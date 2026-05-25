# Benchmark Plan: Lightning++ (epoll) vs .NET Core

## Objective
Compare throughput and latency of:
1. **Lightning++ (epoll architecture)** — built in Release mode
2. **.NET Core minimal API** — single POST `/` endpoint returning "Hello World", also built in Release

Both benchmarks use identical `wrk` parameters and `tests/benchmark.lua` (POST, keep-alive, Content-Length: 0).

---

## Steps

### 0. Branch setup
- Checkout / create branch `claude/epoll-architecture-benchmark-eF2M5`

---

### 1. Build Lightning++ in Release mode
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PROFILING=OFF
cmake --build build -j$(nproc)
```
- Ensures `-O3` optimizations, no debug symbols, no profiling overhead.

---

### 2. Check tooling
- Confirm `wrk` is installed (`wrk --version`)
- Confirm `dotnet` SDK is installed (`dotnet --version`)
- If either is missing → attempt install via apt/snap, or note the gap and skip that leg

---

### 3. Benchmark Lightning++
```bash
# Start server (port 8080, 7 worker threads as in source.cpp)
./build/http &
SERVER_PID=$!
sleep 2   # allow server to start

# Warm-up (5 s, discarded)
wrk -t4 -c100 -d5s --script=tests/benchmark.lua http://localhost:8080/

# Real benchmark (12 threads, 400 connections, 30 s)
wrk -t12 -c400 -d30s --latency --script=tests/benchmark.lua http://localhost:8080/

kill $SERVER_PID
```
- Capture: **Requests/sec**, **Latency avg / p50 / p99 / p999**, **Transfer/sec**

---

### 4. Create .NET Core minimal API comparison app
Create `dotnet-benchmark/` directory with a minimal ASP.NET Core app:
```csharp
// Program.cs
var builder = WebApplication.CreateBuilder(args);
builder.WebHost.ConfigureKestrel(o => {
    o.ListenLocalhost(8081);
    o.Limits.MaxConcurrentConnections = 10000;
    o.Limits.MaxConcurrentUpgradedConnections = 10000;
});
var app = builder.Build();
app.MapPost("/", () => "Hello World");
app.Run();
```
- Uses Kestrel (built-in, battle-tested .NET HTTP server)
- Listens on **port 8081** to avoid conflict

Build in Release:
```bash
cd dotnet-benchmark
dotnet publish -c Release -o ./publish
```

---

### 5. Benchmark .NET Core
```bash
cd dotnet-benchmark/publish
./dotnet-benchmark &   # or: dotnet dotnet-benchmark.dll
DOTNET_PID=$!
sleep 3

# Warm-up
wrk -t4 -c100 -d5s --script=../tests/benchmark.lua http://localhost:8081/

# Real benchmark (same params)
wrk -t12 -c400 -d30s --latency --script=../tests/benchmark.lua http://localhost:8081/

kill $DOTNET_PID
```

---

### 6. Record results
Write a `BENCHMARK_RESULTS.md` file with:
- System info (CPU, cores, RAM)
- Build configurations
- Full wrk output for both runs
- Summary comparison table:

| Metric | Lightning++ (epoll) | .NET Core Kestrel |
|---|---|---|
| Requests/sec | | |
| Avg latency | | |
| p99 latency | | |
| p999 latency | | |
| Transfer/sec | | |

---

### 7. Commit and push
```bash
git add BENCHMARK_RESULTS.md dotnet-benchmark/
git commit -m "feat: add benchmark results — Lightning++ epoll vs .NET Core Kestrel"
git push -u origin claude/epoll-architecture-benchmark-eF2M5
```

---

## Key decisions / trade-offs

| Decision | Rationale |
|---|---|
| 30s benchmark (not 3s from wrk-benchmark.sh) | Longer run gives stable, variance-reduced numbers |
| 5s warm-up | JIT (.NET) and connection pool (both) need time to reach steady state |
| Same `benchmark.lua` for .NET | Fair comparison — POST, keep-alive, zero body |
| Port 8081 for .NET | Avoids conflict with Lightning++ on 8080 |
| 7 worker threads for Lightning++ | Matches source.cpp |
| Kestrel default thread pool for .NET | Default is `Environment.ProcessorCount` — fair, no manual tuning |
| Release builds for both | Eliminates debug overhead from comparison |
