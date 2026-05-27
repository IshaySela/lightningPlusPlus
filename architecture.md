# Lightning++ Architecture

## Overview

Lightning++ is a high-performance, header-heavy C++ HTTP server library built around
three core design pillars: a **non-blocking I/O event loop** (epoll), a **mutex-free
thread pool with a lock-free fast path**, and **pipe-signalled IPC channels** between
the threads that manage those two subsystems. The result is a reactor-style server
that can handle large numbers of concurrent connections with minimal thread contention.

---

## Key Architectural Concepts

### 1 — Accept Thread (main thread)

The calling thread (the one that invokes `HttpServer::start()`) becomes the
**accept thread**. It runs a tight blocking loop around
`ILowLevelSocketServer::accept()`, which maps to the kernel `accept(4)` syscall on a
listening TCP socket. Each accepted file descriptor is:

1. Wrapped in an `IClient` concrete type (`PlainClient` or `SSLClient`).
2. Set to non-blocking mode with `O_NONBLOCK`.
3. **Enqueued** into `NewFdChannel::clients`
   — a `BlockingConcurrentQueue<unique_ptr<IClient>>` (mutex-free; lock-free fast path, futex-based blocking when empty).
4. Signalled to the epoll thread by writing one byte to
   `NewFdChannel::pipeWrite`.

The accept thread never reads request data or writes responses; its only job is to
hand off new sockets as fast as possible.

### 2 — Epoll Thread

A single dedicated `std::thread` runs `NonblockingClientManagerTask::operator()()`.
This thread owns the `epoll(7)` instance and is the **only** thread that calls
`epoll_ctl` or `epoll_wait`. The event loop:

| Step | Action |
|------|--------|
| `epoll_wait()` | Blocks until one of: pipe byte, incoming data, or return-channel notification |
| Pipe read (newFdChannel) | Drains the `NewFdChannel` queue; registers each new fd with `EPOLLIN \| EPOLLONESHOT` |
| `EPOLLIN` on client fd | Peeks up to 64 KiB; scans for `\r\n\r\n`; on match extracts header bytes; dispatches `ClientRequestHandler` task to the worker pool |
| Partial headers | Re-arms fd with `EPOLLONESHOT` and waits for more data |
| Pipe read (returnChannel) | Drains the `ReturnChannel` queue; re-arms fd (`keep-alive`) or closes the connection |

**`EPOLLONESHOT`** is critical: once an fd fires, epoll removes interest in it
automatically, guaranteeing that no two threads ever process the same connection
simultaneously. The epoll thread re-arms the fd only when it is safe to do so.

### 3 — Worker Thread Pool (`TaskExecutor`)

`TaskExecutor<ClientRequestHandler>` is a **mutex-free** MPMC thread pool. It is
*not* fully lock-free in the formal sense — see the note below.

- Backed by `moodycamel::BlockingConcurrentQueue<unique_ptr<ClientRequestHandler>>`.
- Worker threads call `wait_dequeue()`, which has two tiers:
  1. **Lock-free fast path** — spins up to 10 000 iterations on an `atomic<ssize_t>`
     counter with CAS. If an item arrives during the spin, no syscall is made.
  2. **Blocking slow path** — when the queue is genuinely empty the worker falls
     through to `sem_wait()` (POSIX semaphore), which calls `futex(FUTEX_WAIT)` and
     suspends the thread in the kernel. This is **blocking, not lock-free**.
- No `std::mutex` or `std::condition_variable` is used anywhere.
- Shutdown uses a **null-pointer sentinel**: `stop_all()` enqueues one `nullptr`
  per worker; each worker exits when it dequeues `null`. The entry point is guarded
  by `atomic<bool>` with `memory_order_acq_rel` to prevent double-invocation.
- Thread count is configurable at construction time (default: 1).

> **Concurrency terminology note**
> `ConcurrentQueue` (non-blocking) and the enqueue / `try_dequeue` operations of
> `BlockingConcurrentQueue` are **lock-free**: they use only atomic CAS loops and
> never block a thread. `wait_dequeue` is **blocking-with-lock-free-fast-path**: it
> avoids kernel involvement when items are already queued, but a thread waiting on an
> empty queue is parked on a futex and makes no progress — the formal "lock-free"
> guarantee does not hold there.

Each worker executes one `ClientRequestHandler::operator()()` per dequeue:

1. Parse HTTP request line and headers (`HttpRequest::createRequest`).
2. Match URI against registered routes (`UriMapper` regex matching).
3. Extract URI path parameters from regex capture groups.
4. Run pre-middlewares.
5. Call the user-registered `Resolver` function.
6. Run post-middlewares.
7. Serialize `HttpResponse` to raw bytes and write to the client socket stream.
8. Signal completion via `ReturnChannel`.

### 4 — Synchronization Primitives

| Primitive | Location | Purpose |
|-----------|----------|---------|
| `BlockingConcurrentQueue` (moodycamel) | `NewFdChannel::clients`, `TaskExecutor` | Mutex-free hand-off with lock-free fast path; `wait_dequeue` parks on `futex` when queue is empty |
| `ConcurrentQueue` (moodycamel) | `ReturnChannel::connections` | Fully lock-free (CAS-only) non-blocking dequeue; used where the caller never needs to block |
| Unix pipe (`pipeRead` / `pipeWrite`) | `NewFdChannel`, `ReturnChannel` | Wake epoll from `epoll_wait()` when a queue item is available; integrates with epoll itself |
| `EPOLLONESHOT` | epoll registration | Ensures at most one outstanding event per fd; prevents concurrent access to the same connection |
| `std::atomic<bool> kill_threads` | `TaskExecutor` | Idempotent shutdown gate; `exchange(true, acq_rel)` prevents double-shutdown |
| `std::unique_ptr<IClient>` ownership | Everywhere | Single-owner transfer enforces that exactly one entity (epoll thread **or** one worker) holds each connection at any time |

---

## Class Hierarchy

```
ILowLevelSocketServer          IClient              IStream
  ├─ PlainServer                ├─ PlainClient        ├─ PlainStream
  └─ SSLServer                  └─ SSLClient          └─ SSLStream

HttpServer ─uses─► ILowLevelSocketServer
           ─owns─► NewFdChannel
           ─owns─► ReturnChannel
           ─owns─► ResolversMap (method → UriMapper)
           ─owns─► MiddlewareContainer

NonblockingClientManagerTask ─owns─► TaskExecutor<ClientRequestHandler>
                              ─refs─► NewFdChannel, ReturnChannel, HttpServer

ClientRequestHandler ─refs─► HttpServer (read-only routing)
                     ─owns─► unique_ptr<IClient>
                     ─refs─► ReturnChannel
```

---

## Mermaid Diagrams

### Diagram 1 — Thread Architecture & Component Overview

```mermaid
graph TB
    subgraph "Process"
        subgraph AT["Accept Thread (main)"]
            A1[listen socket]
            A2[accept syscall]
            A3[wrap in IClient\nset O_NONBLOCK]
        end

        subgraph CHAN1["NewFdChannel"]
            Q1[BlockingConcurrentQueue\nmutex-free · lock-free fast path\nfutex slow path]
            P1[Unix pipe\npipeRead / pipeWrite]
        end

        subgraph ET["Epoll Thread"]
            E1[epoll_wait]
            E2[drainNewFdChannel\nepoll_ctl ADD]
            E3[handleClientEvent\npeek headers]
            E4[dispatch task]
            E5[drainReturnChannel\nre-arm or close]
        end

        subgraph CHAN2["ReturnChannel"]
            Q2[ConcurrentQueue\nfully lock-free CAS-only MPMC]
            P2[Unix pipe\npipeRead / pipeWrite]
        end

        subgraph TP["Worker Thread Pool (TaskExecutor)"]
            W1[Worker 0\nwait_dequeue]
            W2[Worker 1\nwait_dequeue]
            WN[Worker N\nwait_dequeue]
            TQ[BlockingConcurrentQueue\nfast: atomic CAS spin\nslow: futex sleep]
        end
    end

    A1 -->|blocking accept| A2
    A2 --> A3
    A3 -->|enqueue IClient| Q1
    A3 -->|write 1 byte| P1

    P1 -->|EPOLLIN on pipe fd| E1
    Q1 -->|dequeue| E2
    E2 -->|epoll_ctl ADD\nEPOLLIN\|EPOLLONESHOT| E1
    E1 -->|EPOLLIN on client fd| E3
    E3 -->|complete headers found| E4
    E3 -->|partial headers| E2
    E4 -->|enqueue task| TQ
    TQ --> W1 & W2 & WN

    W1 & W2 & WN -->|enqueue ReturnedConnection| Q2
    W1 & W2 & WN -->|write 1 byte| P2

    P2 -->|EPOLLIN on pipe fd| E1
    Q2 -->|dequeue| E5
    E5 -->|keep-alive:\nepoll_ctl MOD re-arm| E1
    E5 -->|close:\nstream.close| A1
```

---

### Diagram 2 — Data Flow: Client Request → Response

```mermaid
sequenceDiagram
    participant C  as Client (TCP)
    participant AT as Accept Thread
    participant NQ as NewFdChannel<br/>(lock-free queue + pipe)
    participant ET as Epoll Thread
    participant TQ as TaskExecutor Queue<br/>(lock-free + semaphore)
    participant W  as Worker Thread
    participant RQ as ReturnChannel<br/>(lock-free queue + pipe)

    C  ->>+  AT: TCP connect (3-way handshake)
    AT ->>   AT: accept() → fd
    AT ->>   AT: set O_NONBLOCK
    AT ->>   NQ: enqueue IClient(fd)
    AT ->>   NQ: write(pipeWrite, 1 byte)
    AT ->>-  AT: loop back to accept()

    NQ -->>  ET: EPOLLIN on pipe fd (wakes epoll_wait)
    ET ->>   NQ: dequeue IClient(fd)
    ET ->>   ET: epoll_ctl(ADD, fd, EPOLLIN|EPOLLONESHOT)

    C  ->>   ET: HTTP request bytes arrive
    ET ->>   ET: epoll_wait returns EPOLLIN on client fd
    ET ->>   ET: stream.peek(64 KiB)
    ET ->>   ET: scan for "\\r\\n\\r\\n"

    alt Headers incomplete
        ET ->> ET: rearmFd(fd) – epoll_ctl MOD EPOLLONESHOT
    else Headers complete
        ET ->> ET: stream.read(headerEnd + 4)
        ET ->> TQ: enqueue ClientRequestHandler(IClient, headerBytes)
        TQ -->> W: wait_dequeue: CAS fast path or futex wakeup
        W  ->> W: HttpRequest::createRequest(headerBytes)
        W  ->> W: parse method, URI, headers
        W  ->> W: run pre-middlewares
        W  ->> W: getResolver(method, uri) → UriMapper regex match
        W  ->> W: resolver(HttpRequest) → HttpResponse
        W  ->> W: run post-middlewares
        W  ->> W: response.toHttpResponse() → bytes
        W  ->> C: stream.write(responseBytes)
        W  ->> RQ: enqueue ReturnedConnection(IClient, keepAlive)
        W  ->> RQ: write(pipeWrite, 1 byte)

        RQ -->> ET: EPOLLIN on pipe fd
        ET ->> RQ: dequeue ReturnedConnection

        alt Connection: keep-alive
            ET ->> ET: epoll_ctl(MOD, fd, EPOLLIN|EPOLLONESHOT)
            note over C,ET: connection reused for next request
        else Connection: close
            ET ->> ET: stream.close()
            ET ->> C: TCP FIN
        end
    end
```

---

### Diagram 3 — Synchronization Primitives & Ownership Transfer

```mermaid
stateDiagram-v2
    [*] --> Listening : server.start()

    state "Accept Thread" as AT {
        Listening --> Accepted : accept() returns fd
        Accepted --> Enqueued : enqueue to NewFdChannel\n+ write pipe
    }

    state "NewFdChannel" as NC {
        Enqueued --> PipeSignal : pipe byte written
    }

    state "Epoll Thread" as ETE {
        PipeSignal --> EpollRegistered : drain queue\nepoll_ctl ADD\nEPOLLIN|EPOLLONESHOT
        EpollRegistered --> ReadReady  : epoll_wait returns EPOLLIN
        ReadReady --> Dispatched       : complete headers → enqueue task
        ReadReady --> EpollRegistered  : partial headers → rearm fd
        Returned  --> EpollRegistered  : keep-alive → epoll_ctl MOD
        Returned  --> Closed           : close → stream.close()
    }

    state "Worker Thread (TaskExecutor)" as WT {
        Dispatched --> Processing   : wait_dequeue\n(CAS spin → futex if empty)
        Processing --> Responding   : parse + route + handler
        Responding --> Signalled    : write response\nenqueue ReturnChannel\nwrite pipe
    }

    state "ReturnChannel" as RC {
        Signalled --> Returned : pipe wakes epoll_wait
    }

    Closed --> [*]

    note right of AT
        Owns socket FD until
        unique_ptr transferred
        to NewFdChannel queue
    end note

    note right of ETE
        EPOLLONESHOT ensures
        no concurrent events
        on same fd
    end note

    note right of WT
        atomic<bool> kill_threads
        guards shutdown;
        nullptr sentinel exits
        each worker loop
    end note
```

---

### Diagram 4 — Worker Thread Pool Internals

```mermaid
graph LR
    subgraph TaskExecutor
        direction TB
        ADD[add_task\nenqueue unique_ptr&ltT&gt]
        BQ["BlockingConcurrentQueue\n(moodycamel MPMC)"]
        SEM["LightweightSemaphore\nfast: atomic CAS spin ≤10k iters\nslow: sem_wait → futex(FUTEX_WAIT)"]

        subgraph Workers["Worker Threads  (0 … N-1)"]
            direction LR
            W0["Thread 0\nwait_dequeue"]
            W1["Thread 1\nwait_dequeue"]
            WN["Thread N-1\nwait_dequeue"]
        end

        STOP["stop_all()\nenqueue N × nullptr\n(atomic exchange guard)"]
    end

    ADD -->|enqueue| BQ
    BQ  --> SEM
    SEM -->|post| W0 & W1 & WN
    W0 & W1 & WN -->|dequeue| BQ
    W0 & W1 & WN -->|nullptr?| EXIT[thread exits]
    STOP -->|enqueue sentinels| BQ

    style TaskExecutor fill:#f5f5f5,stroke:#999
    style Workers fill:#e8f4f8,stroke:#4a90d9
```

---

## Data Structures Summary

| Structure | Type | Owner | Purpose |
|-----------|------|-------|---------|
| `NewFdChannel::clients` | `BlockingConcurrentQueue<unique_ptr<IClient>>` | Shared | Transfer accepted clients accept→epoll |
| `NewFdChannel::pipeRead/Write` | Unix pipe fds | Shared | Wake epoll_wait when queue has items |
| `ReturnChannel::connections` | `ConcurrentQueue<ReturnedConnection>` | Shared | Transfer finished connections worker→epoll |
| `ReturnChannel::pipeRead/Write` | Unix pipe fds | Shared | Wake epoll_wait when return queue has items |
| `NonblockingClientManagerTask::connections` | `unordered_map<int, ConnectionState>` | Epoll thread | Track all active connections by fd |
| `TaskExecutor::tasks` | `BlockingConcurrentQueue<unique_ptr<T>>` | Thread pool | Pending request handler tasks |
| `HttpServer::resolvers` | `unordered_map<string, UriMapper>` | Read-only post-init | Method → URI regex → Resolver mapping |
| `PlainStream::leftoverBuffer` | `vector<char>` | Per-client | Bytes peeked but not yet consumed |

---

## Protocol Stack

```
┌─────────────────────────────────────┐
│          User Code (Resolver)        │   HttpRequest / HttpResponse
├─────────────────────────────────────┤
│         Middleware Chain             │   pre / post hooks
├─────────────────────────────────────┤
│       ClientRequestHandler           │   HTTP parsing, routing
├─────────────────────────────────────┤
│   IStream (PlainStream / SSLStream)  │   read / write / peek / injectBuffer
├─────────────────────────────────────┤
│   IClient (PlainClient / SSLClient)  │   file descriptor wrapper
├─────────────────────────────────────┤
│           TCP Socket (fd)            │   O_NONBLOCK, epoll-managed
└─────────────────────────────────────┘
```

---

## Performance Properties

| Property | Mechanism |
|----------|-----------|
| Scalable connection handling | Single epoll fd multiplexes thousands of sockets |
| No mutex on accept path | `BlockingConcurrentQueue` enqueue is lock-free (CAS); epoll thread drains synchronously after pipe wakeup |
| No mutex in worker pool | `BlockingConcurrentQueue` is mutex-free; workers spin on atomics in the common case, park on `futex` only when queue is genuinely empty |
| No spurious wakeups in workers | `LightweightSemaphore` counts exact item additions; `sem_post` is called only when a waiter needs waking |
| No concurrent fd access | `EPOLLONESHOT` + single-owner `unique_ptr` |
| No extra copies of request data | Header bytes moved (not copied) into worker task |
| Keep-alive connection reuse | Connections re-armed in-place after response sent |
