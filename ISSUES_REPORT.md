# Open Issues vs. Recent Commits — Status Report

> **Generated:** 2026-05-26  
> **Repository:** `ishaysela/lightningplusplus`  
> **Commits analysed:** 2026-05-16 → 2026-05-25 (latest: `c9dec70`)

---

## Summary

| Issue | Title | Status |
|-------|-------|--------|
| [#18](#issue-18--taskexecutor-replace-single-mutex-vector-queue-with-lock-free-mpmc-queue) | TaskExecutor: replace single-mutex vector queue with lock-free MPMC queue | ✅ **Solved in recent commits** |
| [#17](#issue-17--replace-deprecated-err_get_error_line_data-with-openssl-3x-api) | Replace deprecated `ERR_get_error_line_data` with OpenSSL 3.x API | ❌ **Not yet solved** |
| [#16](#issue-16--implement-http-request-size-restrictions-414-431) | Implement HTTP request size restrictions (414, 431) | ⚠️ **Partially addressed** |

---

## Issue #18 — TaskExecutor: replace single-mutex vector queue with lock-free MPMC queue

**Status: ✅ SOLVED**

### What the issue asked for
Replace the `TaskExecutor`'s single shared `std::mutex` + `std::vector` task queue (which had O(n) `erase(begin())` dequeuing and thundering-herd contention at high thread counts) with either:
- a per-thread deque with work-stealing, or
- a lock-free MPMC ring-buffer (e.g. `boost::lockfree::queue`)

The issue also noted a "minimal first step": swap `std::vector` for `std::deque` to eliminate the O(n) erase.

### How recent commits solved it

The fix was delivered in two stages:

**Stage 1 — Minimal step (2026-05-21)**
| Commit | Description |
|--------|-------------|
| `655112b` | Changed task executor queue to `std::deque`, using `pop_front()` — O(1) instead of O(n) |
| `2586039` | Confirmed the deque change, boosted perf from 20 k → 27 k req/sec |

**Stage 2 — Full lock-free MPMC replacement (2026-05-22 → 2026-05-25)**
| Commit | Description |
|--------|-------------|
| `9e66ca7` | Added moodycamel `BlockingConcurrentQueue` library |
| `5ae19da` | Wired the library into `CMakeLists.txt` |
| `8f66bb1` | Updated `NonblockingClientManager` to use the MPMC queue |
| `06620fb` | Replaced the rigtorp lock-free queue with moodycamel (supports move semantics) |
| `e76ccaa` | Removed the old rigtorp lock-free queue |
| `9d183be` | **Key commit** — created `MPMCTaskExecutor.hpp` backed by `moodycamel::BlockingConcurrentQueue`; switched `NonblockingClientManager` to it; perf jumped from ~21 k → **~40 k req/sec** (avg latency 2.46 ms, p99 4.36 ms vs. 8.7 ms) |
| `be4b035` | Removed the now-unused old `TaskExecutor.hpp` |
| `9fffb1f` | Updated comments on `MPMCTaskExecutor` |
| `e5601d9` | Moved moodycamel files to `lightning/moodycamel/` |
| `47d5a4f` | Switched accept-thread → epoll-thread channel to use moodycamel's blocking queue |

### Current state
`lightning/MPMCTaskExecutor.hpp` is the sole task executor. It uses `moodycamel::BlockingConcurrentQueue<std::unique_ptr<T>>` — lock-free, supports multiple producers and consumers, and avoids all mutex contention. The old `TaskExecutor.hpp` file no longer exists.

**Recommendation:** Close issue #18. The code fully satisfies all requirements stated in the issue.

---

## Issue #17 — Replace deprecated `ERR_get_error_line_data` with OpenSSL 3.x API

**Status: ❌ NOT YET SOLVED**

### What the issue asked for
`OpensslErrorQueueException.cpp:12` calls `ERR_get_error_line_data`, deprecated since OpenSSL 3.0. The fix is to replace it with `ERR_get_error_all`.

### Current code (unchanged)
```cpp
// OpensslErrorQueueException.cpp:12
while ((currentError = ERR_get_error_line_data(&filename, &line, &errorData, &flags)) != 0)
{
    std::cout << "Error code: " << currentError << " with text:\t" << errorData << "\n";
}
```

### No matching commits found
None of the recent commits (2026-05-16 → 2026-05-25) touch `OpensslErrorQueueException.cpp` or the `ERR_get_error_*` family of calls. The deprecated call is still present in the codebase and will continue to produce `-Wdeprecated-declarations` warnings on OpenSSL 3.x builds.

**Recommendation:** Issue #17 remains open. The required change is small (swap one function call, add `funcname` parameter) and should be straightforward to apply.

---

## Issue #16 — Implement HTTP request size restrictions (414, 431)

**Status: ⚠️ PARTIALLY ADDRESSED**

### What the issue asked for
Return standard HTTP error responses for oversized input:
- **414 URI Too Long** — request-target exceeds a configured maximum
- **431 Request Header Fields Too Large** — a single header or the total header block exceeds limits

### What was found in recent commits
No commit explicitly mentions 414, 431, or header-size enforcement. However, the recent non-blocking I/O refactor added a size constant:

```cpp
// lightning/httpServer/NonblockingClientManagerTask.hpp:32
static constexpr int MAX_HEADER_BYTES = 65536;  // 64 KB
```

This constant exists but the codebase does **not** yet return proper `414` or `431` responses when limits are breached — connections are dropped or errors are handled silently.

### Gap
The acceptance criteria from the issue are not met:
- [ ] `414 URI Too Long` response on oversized request-line
- [ ] `431 Request Header Fields Too Large` response on oversized header
- [ ] `Connection: close` header included in rejection responses
- [ ] Limits configurable via `ServerBuilder`

**Recommendation:** Issue #16 remains open. `MAX_HEADER_BYTES` is a good foundation; the remaining work is to hook size checks into the parsing stage and return the correct 4xx responses.

---

## Commit Timeline (Issues Period: 2026-05-21 → 2026-05-25)

```
2026-05-21  655112b  deque + pop_front O(1)                     → partial fix for #18
2026-05-21  2586039  confirmed deque improvement (20k→27k r/s)  → partial fix for #18
2026-05-22  9e66ca7  add moodycamel library                      → towards #18
2026-05-22  5ae19da  wire moodycamel into CMakeLists             → towards #18
2026-05-22  8f66bb1  NonblockingClientManager uses MPMC          → towards #18
2026-05-23  06620fb  replace rigtorp with moodycamel             → towards #18
2026-05-23  e76ccaa  remove old rigtorp queue                    → towards #18
2026-05-23  9d183be  MPMCTaskExecutor + 21k→40k r/s perf jump   → COMPLETES #18
2026-05-25  be4b035  remove unused TaskExecutor.hpp              → cleanup for #18
2026-05-25  9fffb1f  update MPMCTaskExecutor comments            → cleanup for #18
2026-05-25  e5601d9  move moodycamel to lightning/moodycamel/    → cleanup for #18
2026-05-25  47d5a4f  accept→epoll channel on moodycamel          → cleanup for #18
```

---

*Issues #17 and #16 have no corresponding commits in the analysed window.*
