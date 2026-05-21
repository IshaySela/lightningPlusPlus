# Bottleneck Report: `feature/non-blocking-io`

## Environment
- Branch: `feature/non-blocking-io`
- Binary: built with `-O2 -g -fno-omit-frame-pointer` (`ENABLE_PROFILING=ON`)
- Profiler: `strace -c -f -p <PID>` (syscall accounting across all 34 threads)
- Load generator: `wrk -t4 -c100 -d12s`
- Server config: 32 worker threads, POST `/` handler returning static HTML

---

## Benchmark Results

| Mode | Connections | Req/s | p50 | p99 |
|---|---|---|---|---|
| `Connection: keep-alive` | 100 | **42,754** | 2.30ms | 4.23ms |
| `Connection: keep-alive` | 400 | **42,393** | 9.37ms | 12.26ms |
| `Connection: close` | 100 | **17,537** | 4.02ms | 40.00ms |
| `Connection: close` | 400 | **19,175** | 16.20ms | 64.50ms |

The ~15k cap reported in production corresponds to the **`Connection: close` path**. Keep-alive saturates at ~42k. The ceiling in both cases is mutex contention, but close mode adds per-connection syscall overhead on top.

---

## strace Syscall Profile (keep-alive, during 95,832 req window)

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 97.74  220.957389        1359    162469       1142 futex
  0.84    1.904811          25     73589      26700 recvfrom
  0.46    1.047664          39     26700            sendto
  0.40    0.894513        8856       101            accept
  0.29    0.658184          24     26801            write
  0.24    0.549955          11     46988            epoll_ctl
  0.02    0.044927          23      1932            epoll_wait
  0.00    0.005689          10       542            read
```

## strace Syscall Profile (Connection:close, during 22,314 req window)

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 96.20  284.565488        2089    136164        910 futex
  1.03    3.044345         135     22415             accept
  0.74    2.195826          31     69573      22327 recvfrom
  0.57    1.699457          37     44828             write
  0.57    1.693701          37     44830             fcntl
  0.41    1.226012          54     22413             sendto
  0.25    0.725013          15     47163             epoll_ctl
  0.13    0.377309          16     22421        105 shutdown
  0.08    0.223422           9     22421             close
  0.01    0.037320          25      1490             epoll_wait
```

---

## Findings

### 1. CRITICAL — Mutex Contention (`futex`): 97% of All Syscall Time

**The dominant bottleneck in both modes is `futex`** — the Linux primitive underlying `std::mutex` and `std::condition_variable`.

- Keep-alive: **1,359 µs average futex wait** (162,469 calls = 97.74% of total syscall time)
- Close: **2,089 µs average futex wait** (136,164 calls = 96.20% of total syscall time)

An uncontended futex returns in < 1 µs. A 1,359 µs average means threads are genuinely sleeping, not spinning — they are blocked waiting for another thread to release a lock.

Three mutexes are in the hot path on every request:

**a) `TaskExecutor::mutex`** (`lightning/TaskExecutor.hpp`)  
All 32 worker threads plus the epoll thread contend on this single lock. The worker threads hold it while waiting on the condition variable (`cv.wait(lock, ...)`), so each `add_task()` call from the epoll thread must fight 32 waiting threads.

```cpp
// Every worker thread (32 of them) — all holding this lock simultaneously:
std::unique_lock<std::mutex> lock(executor.mutex);
executor.cv.wait(lock, [...] { return executor.tasks.size() > 0 || ...; });
```

**b) `ReturnChannel::m`** (`lightning/httpServer/FdChannels.hpp` + `ClientRequestHandler.cpp`)  
All 32 workers converge on this single mutex when signalling completion. At peak, 32 threads simultaneously try to push into `returnChannel.connections`:

```cpp
// ClientRequestHandler.cpp — every worker, end of every request:
std::lock_guard lock(returnChannel.get().m);  // 32 threads fight here
returnChannel.get().connections.push_back({std::move(client), keepAlive});
```

**c) `NewFdChannel::m`** (`FdChannels.hpp`)  
Lower severity — only the accept thread and epoll thread contend here. But under Connection:close this fires once per request.

---

### 2. HIGH — `TaskExecutor::tasks.erase(begin())` Not Fixed

**File**: `lightning/TaskExecutor.hpp`

The commit `655112b` claims O(1) dequeue by switching `vector` → `deque`. However the implementation still calls `.erase(begin())` instead of `.pop_front()`:

```cpp
// Current (still O(n) — shifts all remaining elements):
auto task = executor.tasks.front();
executor.tasks.erase(executor.tasks.begin());

// Should be O(1):
auto task = std::move(executor.tasks.front());
executor.tasks.pop_front();
```

This fires inside the lock on every dequeue, slowing the critical section and increasing lock hold time.

---

### 3. HIGH — `EPOLLONESHOT` Generates ~2 `epoll_ctl` Syscalls Per Request

Keep-alive: **46,988 epoll_ctl** for 26,700 requests = **1.76 calls/request**  
Close: **47,163 epoll_ctl** for 22,413 requests = **2.10 calls/request**

Each `EPOLLONESHOT` event auto-disables the fd after firing, requiring an explicit `epoll_ctl(EPOLL_CTL_MOD)` re-arm in `rearmFd()`. For keep-alive this fires per request. For close it fires when a partial header arrives (triggering re-arm before the request is dispatched).

`epoll_ctl` is a syscall; these are all happening on the single epoll thread — adding to its serialisation bottleneck.

---

### 4. MEDIUM — Connection:close Adds 5 Syscalls Per Request

Close mode adds per-connection overhead absent from keep-alive:

| Syscall | Count | Cost (µs/call) | Purpose |
|---|---|---|---|
| `accept()` | 1× | 135 µs | New TCP connection |
| `fcntl()` ×2 | 2× | 37 µs | Set `O_NONBLOCK` (GET + SET) |
| `shutdown()` | 1× | 16 µs | Half-close before close |
| `close()` | 1× | 9 µs | Socket teardown |

Total: ~234 µs in syscalls per request just for connection lifecycle, vs ~0 for keep-alive. At 15k req/s this is 234 µs × 15k = 3.5 seconds of syscall time per second — i.e. the system is spending 3.5 CPU-seconds per wall-clock second just on connection setup/teardown.

The `fcntl(GET) + fcntl(SET)` pattern (`HttpServer.cpp`) runs per-connection. It can be folded into one `fcntl(F_SETFL, O_NONBLOCK)` without the GET.

---

### 5. LOW — `recvfrom` Error Rate (EAGAIN on non-blocking sockets)

Keep-alive: 26,700 errors out of 73,589 `recvfrom` calls = **36% error rate**  
Close: 22,327 errors out of 69,573 `recvfrom` calls = **32% error rate**

These are `EAGAIN`/`EWOULDBLOCK` returns from `peek()` when the socket has no more data. The stream's `peek()` loop keeps calling `recv()` until it gets a partial read, then stops — one extra failed syscall per peek operation. This is unavoidable with the current peek-until-header design, but is a minor cost relative to futex.

---

## Root Cause Summary

The 15k req/s cap on Connection:close is caused by:

1. **97% of syscall time in futex** — all 32 worker threads + epoll thread fighting over two mutexes (`TaskExecutor::mutex` and `ReturnChannel::m`) on every single request
2. **234 µs of extra connection-lifecycle syscalls per request** in close mode (accept + 2×fcntl + shutdown + close) that don't exist in keep-alive mode
3. **Incomplete `pop_front()` optimization** in `TaskExecutor` increases lock hold time under the already-contended mutex

The keep-alive cap at ~42k req/s is set almost entirely by (1) alone.

---

## Recommended Fixes (Ordered by Impact)

### Fix 1 — Replace `ReturnChannel` mutex with a lock-free MPSC queue
All 32 worker threads are the producers; the epoll thread is the sole consumer. This is the textbook case for a multi-producer single-consumer (MPSC) queue. An atomic linked list or ring buffer eliminates the mutex entirely, removing the primary futex contention source.

### Fix 2 — Replace `TaskExecutor` with a work-stealing or MPMC queue
`std::mutex` + `std::condition_variable` across 32 threads is the second contention source. A lock-free MPMC queue (e.g. a bounded ring buffer with CAS) would eliminate the futex traffic on `add_task` / dequeue.

### Fix 3 — Fix `pop_front()` (trivial, immediate win)
`lightning/TaskExecutor.hpp`: replace `tasks.erase(tasks.begin())` with `tasks.pop_front()` on both the copy and move branches.

### Fix 4 — Replace `EPOLLONESHOT` with level-triggered + explicit state
Removes one `epoll_ctl()` syscall per request from the epoll thread's hot path.

### Fix 5 — Fold `fcntl` GET+SET into single SET
`HttpServer.cpp`: replace `fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)` with `fcntl(fd, F_SETFL, O_NONBLOCK)` — saves one syscall per connection on the close path.
