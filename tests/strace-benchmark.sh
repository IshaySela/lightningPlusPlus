#!/usr/bin/env bash
# strace-benchmark.sh — benchmark while profiling server syscalls with strace
#
# Usage:
#   ./strace-benchmark.sh <binary> [options]
#
# Options:
#   -p PORT         Port to bind/connect (default: 8080)
#   -t THREADS      wrk threads (default: 4)
#   -c CONNECTIONS  wrk connections (default: 100)
#   -d DURATION     Benchmark duration in seconds (default: 15)
#   -w WARMUP       Warmup duration before benchmark (default: 3)
#   -k              Keep-alive mode (default)
#   -C              Connection: close mode
#   -o FILE         Write strace summary to FILE (default: strace-summary.txt)
#   -l LUA          wrk Lua script (default: inline POST/keep-alive script)
#
# Notes:
#   ptrace_scope=1 blocks attaching strace to a running process without root.
#   This script avoids that restriction by launching the server as a child of
#   strace, which never needs ptrace-attach permission.
#
#   Performance numbers collected under strace are not representative of real
#   throughput — strace intercepts every syscall in every thread.  The syscall
#   distribution (% time) is still meaningful.  For accurate throughput numbers,
#   run wrk against the binary directly (without strace).

set -euo pipefail

# ── defaults ──────────────────────────────────────────────────────────────────
BINARY=""
PORT=8080
WRK_THREADS=4
WRK_CONNECTIONS=100
DURATION=15
WARMUP=3
CONNECTION_CLOSE=0
STRACE_OUT="strace-summary.txt"
LUA_SCRIPT=""

# ── argument parsing ──────────────────────────────────────────────────────────
if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <binary> [options]" >&2
    exit 1
fi

BINARY="$1"; shift

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p) PORT="$2";             shift 2 ;;
        -t) WRK_THREADS="$2";     shift 2 ;;
        -c) WRK_CONNECTIONS="$2"; shift 2 ;;
        -d) DURATION="$2";        shift 2 ;;
        -w) WARMUP="$2";          shift 2 ;;
        -C) CONNECTION_CLOSE=1;   shift   ;;
        -k) CONNECTION_CLOSE=0;   shift   ;;
        -o) STRACE_OUT="$2";      shift 2 ;;
        -l) LUA_SCRIPT="$2";      shift 2 ;;
        *)  echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

if [[ ! -x "$BINARY" ]]; then
    echo "Error: '$BINARY' is not executable." >&2
    exit 1
fi

# ── helpers ───────────────────────────────────────────────────────────────────
URL="http://127.0.0.1:${PORT}/"
STRACE_RAW=$(mktemp /tmp/strace-raw-XXXXXX.txt)
WRK_OUT=$(mktemp /tmp/wrk-out-XXXXXX.txt)
CLEANUP_LUA=0

# If no Lua script was supplied, generate an inline one.
# Avoids the need for an external file and ensures POST + correct
# Connection mode are used regardless of wrk defaults.
if [[ -z "$LUA_SCRIPT" ]]; then
    LUA_SCRIPT=$(mktemp /tmp/wrk-lua-XXXXXX.lua)
    CLEANUP_LUA=1
    if [[ "$CONNECTION_CLOSE" -eq 1 ]]; then
        cat > "$LUA_SCRIPT" <<'EOF'
wrk.method = "POST"
wrk.headers["Content-Length"] = "0"
wrk.headers["Connection"] = "close"
EOF
    else
        cat > "$LUA_SCRIPT" <<'EOF'
wrk.method = "POST"
wrk.headers["Content-Length"] = "0"
wrk.headers["Connection"] = "keep-alive"
EOF
    fi
fi

cleanup() {
    kill "$STRACE_PID" 2>/dev/null || true
    wait "$STRACE_PID" 2>/dev/null || true
    # strace exit orphans the server child — kill it by port
    fuser -k "${PORT}/tcp" &>/dev/null || true
    rm -f "$STRACE_RAW" "$WRK_OUT"
    [[ "$CLEANUP_LUA" -eq 1 ]] && rm -f "$LUA_SCRIPT" || true
}
trap cleanup EXIT

separator() { printf '%0.s─' {1..70}; echo; }

# ── clear the port ────────────────────────────────────────────────────────────
# A previous run may have left an orphaned server on the port.
fuser -k "${PORT}/tcp" &>/dev/null || true
sleep 0.2

# ── start server under strace ─────────────────────────────────────────────────
# Launching the binary as a strace child bypasses ptrace_scope=1: a process
# may always ptrace its own children without elevated permissions.
# strace -c writes the syscall summary to stderr; redirect to $STRACE_RAW.
echo
echo "▶  Starting server under strace: $BINARY"
strace -f -c -S time "$BINARY" 2>"$STRACE_RAW" &
STRACE_PID=$!

# wait for server to listen
for i in {1..20}; do
    if curl -sf -X POST "$URL" -H "Content-Length: 0" -o /dev/null 2>/dev/null; then
        break
    fi
    sleep 0.3
    if ! kill -0 "$STRACE_PID" 2>/dev/null; then
        echo "Error: server exited before accepting connections." >&2
        exit 1
    fi
done

SERVER_PID=$(pgrep -P "$STRACE_PID" -x "$(basename "$BINARY")" 2>/dev/null || echo "?")
echo "   strace PID: $STRACE_PID  server PID: $SERVER_PID  port: $PORT"
echo

# ── warmup ────────────────────────────────────────────────────────────────────
echo "⏳  Warming up for ${WARMUP}s …"
wrk -t"$WRK_THREADS" -c"$WRK_CONNECTIONS" -d"${WARMUP}s" \
    --script="$LUA_SCRIPT" \
    "$URL" &>/dev/null
echo "   Done."
echo

# ── benchmark ─────────────────────────────────────────────────────────────────
MODE_LABEL=$([[ "$CONNECTION_CLOSE" -eq 1 ]] && echo "Connection: close" || echo "keep-alive")

separator
echo "📊  wrk  threads=${WRK_THREADS}  connections=${WRK_CONNECTIONS}  duration=${DURATION}s  mode=${MODE_LABEL}"
echo "   ⚠  Running under strace — throughput numbers reflect strace overhead only"
separator

wrk -t"$WRK_THREADS" -c"$WRK_CONNECTIONS" -d"${DURATION}s" --latency \
    --script="$LUA_SCRIPT" \
    "$URL" | tee "$WRK_OUT"

# ── stop strace ───────────────────────────────────────────────────────────────
# SIGINT causes strace to detach and flush the -c summary to $STRACE_RAW.
# The server child becomes an orphan; the cleanup trap kills it via fuser.
kill -INT "$STRACE_PID" 2>/dev/null || true
wait "$STRACE_PID" 2>/dev/null || true

# ── strace summary ────────────────────────────────────────────────────────────
separator
echo "🔬  Syscall profile (strace -f -c, sorted by time)"
separator

grep -v "^strace:" "$STRACE_RAW" | head -80
grep -v "^strace:" "$STRACE_RAW" > "$STRACE_OUT"
echo
echo "   Full strace summary saved to: $STRACE_OUT"

# ── summary ───────────────────────────────────────────────────────────────────
separator
echo "📋  Summary"
separator

RPS=$(grep "Requests/sec" "$WRK_OUT" | awk '{printf "%.0f", $2}')
LAT_AVG=$(grep "Latency" "$WRK_OUT" | awk 'NR==1{print $2}')
LAT_P99=$(grep "99%" "$WRK_OUT" | awk '{print $2}')
TOP_SYSCALL=$(grep -v "^strace:" "$STRACE_RAW" \
             | awk '$1~/^[0-9]/ {print $NF; exit}')
TOP_PCT=$(grep -v "^strace:" "$STRACE_RAW" \
         | awk '$1~/^[0-9]/ {print $1; exit}')

echo "   Binary      : $BINARY"
echo "   Mode        : $MODE_LABEL"
echo "   Threads/conn: ${WRK_THREADS}t / ${WRK_CONNECTIONS}c"
echo "   Requests/sec: ${RPS}  (strace overhead — not representative)"
echo "   Latency avg : ${LAT_AVG}   p99: ${LAT_P99}"
echo "   Top syscall : ${TOP_SYSCALL} (${TOP_PCT}% of syscall time)"
separator
echo "   ℹ  For real throughput, run wrk directly:"
echo "      wrk -t${WRK_THREADS} -c${WRK_CONNECTIONS} -d${DURATION}s --latency --script=<lua> ${URL}"
separator