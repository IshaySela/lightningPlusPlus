# Benchmark Results: Lightning++ (epoll) vs .NET Core 8 Kestrel

## System Info
| | |
|---|---|
| CPU | Intel Xeon @ 2.10 GHz |
| Physical cores | 4 |
| Threads/core | 1 |
| RAM | 15 GiB |
| OS | Linux 6.18.5 |
| wrk version | 4.1.0 (epoll backend) |

## Configurations

| Parameter | Lightning++ | .NET Core 8 Kestrel |
|---|---|---|
| Build | `cmake -DCMAKE_BUILD_TYPE=Release` | `dotnet publish -c Release` |
| Port | 8080 | 8081 |
| HTTP protocol | HTTP/1.1 | HTTP/1.1 (`HttpProtocols.Http1`, explicit) |
| Worker threads | 7 (`.withThreads(7)`) | Default (`ProcessorCount` = 4) |
| Handler | `POST /` → `<h1>Hello!</h1>` (HTML body) | `POST /` → `"Hello World"` (plain text) |

> **Both benchmarks used identical wrk settings:**
> `wrk -t12 -c400 -d30s --latency --script=tests/benchmark.lua`
> (POST, `Connection: keep-alive`, `Content-Length: 0`)

---

## Results

### Lightning++ (epoll, Release)

```
Running 30s test @ http://localhost:8080/
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     4.77ms    1.93ms  32.49ms   74.19%
    Req/Sec     6.99k     1.14k   30.84k    75.80%
  Latency Distribution
     50%    4.18ms
     75%    5.75ms
     90%    7.53ms
     99%   10.73ms
  2507144 requests in 30.10s, 239.10MB read
Requests/sec:  83299.68
Transfer/sec:      7.94MB
```

### .NET Core 8 Kestrel (HTTP/1.1 only, Release)

```
Running 30s test @ http://localhost:8081/
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     5.83ms    2.27ms  51.97ms   79.54%
    Req/Sec     5.74k   787.40    42.00k    92.64%
  Latency Distribution
     50%    5.58ms
     75%    6.84ms
     90%    8.20ms
     99%   12.43ms
  2057374 requests in 30.10s, 319.82MB read
Requests/sec:  68359.05
Transfer/sec:     10.63MB
```

---

## Summary Comparison

| Metric | Lightning++ (epoll) | .NET Core 8 Kestrel | Delta |
|---|---|---|---|
| **Requests/sec** | **83,300** | 68,359 | **+21.8%** |
| Avg latency | 4.77 ms | 5.83 ms | −18.2% |
| p50 latency | 4.18 ms | 5.58 ms | −25.1% |
| p75 latency | 5.75 ms | 6.84 ms | −15.9% |
| p90 latency | 7.53 ms | 8.20 ms | −8.2% |
| p99 latency | **10.73 ms** | 12.43 ms | −13.7% |
| Max latency | 32.49 ms | 51.97 ms | −37.5% |
| Transfer/sec | 7.94 MB | 10.63 MB | — (response size differs) |
| Total requests (30s) | 2,507,144 | 2,057,374 | +449,770 |

---

## Notes

- **Throughput:** Lightning++ delivered **~21.8% more requests/sec** than .NET Core Kestrel under identical load.
- **Latency:** Lightning++ was faster at every percentile — most notably p50 (−25%) and max (−37%).
- **Worker threads:** Lightning++ used 7 workers vs .NET's default 4 (`ProcessorCount`); this partially accounts for the delta given the 4-core machine. .NET's thread pool is dynamic and will burst beyond this under load.
- **Response body:** Lightning++ returns HTML (`<h1>Hello!</h1>` + headers, ~104 bytes); .NET returns plain text (`Hello World`, ~143 bytes with headers). Slightly larger .NET responses inflate its transfer/sec number but add marginal serialization cost.
- **Protocol parity:** Both servers ran HTTP/1.1 with `Connection: keep-alive`. Kestrel was explicitly configured with `HttpProtocols.Http1` to prevent HTTP/2 negotiation.
- **Same host:** Both servers ran on the same machine as wrk — results reflect pure server throughput, not network RTT.
