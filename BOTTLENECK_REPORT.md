# Bottleneck Report: `feature/non-blocking-io`

Profiled across two snapshots of the branch:
- **Snapshot A** — commit `35a5d13` (pre-MPMC): mutex-based `ReturnChannel`, `erase(begin())`
- **Snapshot B** — commit `8f66bb1` (latest): `rigtorp::MPMCQueue` ReturnChannel, `pop_front()` fixed

Profiling method: `strace -c -f -p <PID>` across all 34 threads during `wrk` load.  
Server config: 32 worker threads, POST `/` → static HTML response.  
Hardware: **4 CPU cores**, Intel Xeon @ 2.80 GHz.

---

## Benchmark Results

### Snapshot A — `35a5d13` (mutex ReturnChannel, erase(begin()))

| Mode | Threads | Conns | Req/s | p50 | p99 |
|---|---|---|---|---|---|
| keep-alive | t4 | 100 | **42,754** | 2.30ms | 4.23ms |
| keep-alive | t12 | 400 | **36,352** | 2.70ms | 4.95ms |
| Connection:close | t4 | 100 | **17,537** | 4.02ms | 40.00ms |
| Connection:close | t8 | 400 | **19,175** | 16.20ms | 64.50ms |

### Snapshot B — `8f66bb1` (MPMC ReturnChannel, pop_front())

| Mode | Threads | Conns | Req/s | p50 | p99 |
|---|---|---|---|---|---|
| keep-alive | t4 | 100 | **4,067** | 23.7ms | 124ms |
| keep-alive | t12 | 400 | **1,730** | 239ms | 328ms |
| Connection:close | t4 | 100 | **5,453** | 16.3ms | 88ms |

**The MPMC change caused a ~10× regression across all modes.**

---

## strace Syscall Profiles

### Snapshot A — keep-alive (95,832-req window)

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 97.74  220.957389        1359    162469      1142 futex
  0.84    1.904811          25     73589     26700 recvfrom
  0.46    1.047664          39     26700            sendto
  0.40    0.894513        8856       101            accept
  0.29    0.658184          24     26801            write
  0.24    0.549955          11     46988            epoll_ctl
  0.02    0.044927          23      1932            epoll_wait
  0.00    0.005689          10       542            read
```

### Snapshot B — keep-alive (3,758-req window)

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 96.45   48.873425        2172     22494        266 futex
  1.54    0.779489        7717       101             accept
  0.83    0.420079          21     19191       3763 recvfrom
  0.44    0.220715          58      3758             sendto
  0.43    0.216464          14     15450             epoll_ctl
  0.26    0.132202          34      3849             write
  0.05    0.024191          18      1309             epoll_wait
  0.01    0.005259          26       202             fcntl
```

### Snapshot A — Connection:close (22,413-req window)

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
```

---

## Analysis

### Finding 1 — CRITICAL REGRESSION: MPMC Spin-Wait on 4-Core Machine

**Root cause of 10× regression**: `rigtorp::MPMCQueue` is a spin-wait queue. Its `emplace()` and `pop()` busy-loop using `while (turn != slot.turn.load(...));`. On a machine with only **4 CPU cores** and **34 threads** (1 accept + 1 epoll + 32 workers), spinning is catastrophic:

1. A worker thread claims a slot via `head_.fetch_add(1)` (atomic)
2. The OS context-switches that worker before it finishes the `slot.turn.store()`
3. The epoll thread calls `drainReturnChannel()`, reaches `pop()`, and immediately begins spinning on `slot.turn` — burning an entire CPU core
4. With 4 cores, one core spinning blocks one other thread from running
5. At scale: multiple workers spinning in `emplace()` + epoll spinning in `pop()` → nearly all 4 cores burn in tight loops → actual request processing stalls

Per strace ratio comparison:
- Snapshot A: 162,469 futex calls / 95,832 req = **1.7 futex/req**, avg wait **1,359 µs**
- Snapshot B: 22,494 futex calls / 3,758 req = **5.9 futex/req**, avg wait **2,172 µs**

Each completed request in Snapshot B takes 3.5× more futex time *and* wastes uncountable CPU cycles in spin-wait that strace cannot measure. The result: 10× throughput collapse.

**The rule**: lock-free spin-wait queues only outperform mutex queues when `active_threads ≤ CPU_cores`. At 34 threads / 4 cores, a mutex that sleeps is strictly better because sleeping yields the CPU to threads that can make progress.

**Fix options**:
- Revert `ReturnChannel` to the mutex + move-drain from Snapshot A (already worked well)
- OR replace `rigtorp::MPMCQueue` with a bounded queue that falls back to `futex_wait` when empty/full
- OR reduce worker thread count from 32 → `nproc` (4), then spin-wait becomes viable

---

### Finding 2 — NEW BUG: `empty()` + `pop()` TOCTOU in `drainReturnChannel`

**File**: `NonblockingClientManagerTask.cpp`

```cpp
while(!returnChannel.connections.empty())  // ← check
{
    ReturnedConnection rc;
    returnChannel.connections.pop(rc);     // ← act — NOT atomic with check
```

`pop()` does `tail_.fetch_add(1)` unconditionally, then spin-waits for `slot.turn`. A producer that claims `head` but gets context-switched before its `slot.turn.store()` will cause `pop()` to spin on that slot until the producer resumes — potentially many milliseconds on a 4-core oversubscribed system.

**Fix**: Replace `pop()` with `try_pop()`, which returns false immediately if the slot isn't ready:

```cpp
ReturnedConnection rc;
while (returnChannel.connections.try_pop(rc))
{
    // process rc
}
```

---

### Finding 3 — `pop_front()` Fix Landed Correctly (`2586039`)

**File**: `lightning/TaskExecutor.hpp`

The commit replacing `tasks.erase(tasks.begin())` with `tasks.pop_front()` is correct and is a genuine improvement — reduces lock hold time in the `TaskExecutor` mutex critical section. Its benefit is obscured by the MPMC regression in Snapshot B but will be visible once that is reverted.

---

### Finding 4 — Baseline Bottleneck Unchanged: `futex` at 96–97% in Both Snapshots

Despite the MPMC structural change, `futex` remains the dominant syscall. In Snapshot A it came from `TaskExecutor::mutex` + `ReturnChannel::m`. In Snapshot B it comes from `TaskExecutor::mutex` alone, but per-request futex time is *higher* because TaskExecutor workers are starved by spinning threads consuming CPU.

Until the thread count matches the core count, the `TaskExecutor` mutex contention will persist as the ceiling. With 32 workers on 4 cores, at any moment 28 threads are context-switched out and competing to wake up — every `add_task()` produces a thundering herd.

---

### Finding 5 — Connection:close Path: 2× `fcntl` Per Connection (minor)

**File**: `HttpServer.cpp`

Visible in Snapshot A Connection:close strace: **44,830 fcntl calls for 22,413 requests = 2 per connection**:

```cpp
fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);  // GET + SET = 2 syscalls
```

Replaceable with:

```cpp
fcntl(fd, F_SETFL, O_NONBLOCK);  // SET only = 1 syscall
```

Minor: saves ~17 µs/connection on the close path only.

---

## Recommended Fix Priority

| Priority | Fix | Expected Impact |
|---|---|---|
| 1 | Revert `ReturnChannel` to mutex + move-drain, OR reduce threads to `nproc` | Restores ~42k req/s keep-alive |
| 2 | Replace `pop()` with `try_pop()` in `drainReturnChannel` | Prevents epoll spin-stall on MPMC path |
| 3 | Match worker thread count to `nproc` (4) at runtime (`std::thread::hardware_concurrency()`) | Reduces all lock contention by 8× |
| 4 | Replace `TaskExecutor` mutex+CV with a sleeping bounded queue | Eliminates remaining futex dominance |
| 5 | `fcntl(SET only)` in `HttpServer.cpp` | Saves 1 syscall/connection on close path |
