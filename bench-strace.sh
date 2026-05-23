#!/usr/bin/env bash
# bench-strace.sh — run wrk benchmark while profiling server syscalls with strace
#
# Usage:
#   ./bench-strace.sh <binary> [options]
#
# Options:
#   -p PORT         Port to bind/connect (default: 8080)
#   -t THREADS      wrk threads (default: 4)
#   -c CONNECTIONS  wrk connections (default: 100)
#   -d DURATION     Benchmark duration in seconds (default: 15)
#   -w WARMUP       Warmup duration before strace attaches (default: 3)
#   -k              Keep-alive mode (default)
#   -C              Connection: close mode
#   -o FILE         Write strace summary to FILE (default: strace-summary.txt)
#
# Examples:
#   ./bench-strace.sh ./build/http
#   ./bench-strace.sh ./build/http -t 12 -c 400 -d 30
#   ./bench-strace.sh ./build/http -C -o close-strace.txt

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

# ── argument parsing ──────────────────────────────────────────────────────────
if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <binary> [options]" >&2
    exit 1
fi

BINARY="$1"; shift

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p) PORT="$2";            shift 2 ;;
        -t) WRK_THREADS="$2";    shift 2 ;;
        -c) WRK_CONNECTIONS="$2";shift 2 ;;
        -d) DURATION="$2";       shift 2 ;;
        -w) WARMUP="$2";         shift 2 ;;
        -C) CONNECTION_CLOSE=1;  shift   ;;
        -k) CONNECTION_CLOSE=0;  shift   ;;
        -o) STRACE_OUT="$2";     shift 2 ;;
        *)  echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

if [[ ! -x "$BINARY" ]]; then
    echo "Error: '$BINARY' is not executable." >&2
    exit 1
fi

# ── helpers ───────────────────────────────────────────────────────────────────
URL="http://localhost:${PORT}/"
STRACE_RAW=$(mktemp /tmp/strace-raw-XXXXXX.txt)
WRK_OUT=$(mktemp /tmp/wrk-out-XXXXXX.txt)

cleanup() {
    kill "$SERVER_PID" 2>/dev/null || true
    kill "$STRACE_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
    rm -f "$STRACE_RAW" "$WRK_OUT"
}
trap cleanup EXIT

separator() { printf '%0.s─' {1..70}; echo; }

# ── start server ──────────────────────────────────────────────────────────────
echo
echo "▶  Starting server: $BINARY"
"$BINARY" &>/dev/null &
SERVER_PID=$!

# wait for server to listen
for i in {1..20}; do
    if curl -sf -X POST "$URL" -H "Content-Length: 0" -o /dev/null 2>/dev/null; then
        break
    fi
    sleep 0.3
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "Error: server exited before accepting connections." >&2
        exit 1
    fi
done

echo "   PID: $SERVER_PID  port: $PORT"
echo

# ── warmup ────────────────────────────────────────────────────────────────────
echo "⏳  Warming up for ${WARMUP}s …"
wrk -t"$WRK_THREADS" -c"$WRK_CONNECTIONS" -d"${WARMUP}s" \
    -H "Content-Length: 0" \
    ${CONNECTION_CLOSE:+-H "Connection: close"} \
    "$URL" &>/dev/null
echo "   Done."
echo

# ── attach strace ─────────────────────────────────────────────────────────────
# -c  summary mode (counts + time per syscall)
# -f  follow all threads/children
# -S  sort by total time spent
echo "🔬  Attaching strace (following all threads) …"
strace -f -c -S time -p "$SERVER_PID" 2>"$STRACE_RAW" &
STRACE_PID=$!
sleep 0.5   # give strace time to attach before load starts

# ── benchmark ─────────────────────────────────────────────────────────────────
MODE_LABEL="keep-alive"
EXTRA_HEADERS=(-H "Content-Length: 0")
if [[ "$CONNECTION_CLOSE" -eq 1 ]]; then
    MODE_LABEL="Connection: close"
    EXTRA_HEADERS+=(-H "Connection: close")
fi

separator
echo "📊  wrk  threads=${WRK_THREADS}  connections=${WRK_CONNECTIONS}  duration=${DURATION}s  mode=${MODE_LABEL}"
separator

wrk -t"$WRK_THREADS" -c"$WRK_CONNECTIONS" -d"${DURATION}s" --latency \
    "${EXTRA_HEADERS[@]}" \
    "$URL" | tee "$WRK_OUT"

# ── stop strace ───────────────────────────────────────────────────────────────
kill -INT "$STRACE_PID" 2>/dev/null || true
wait "$STRACE_PID" 2>/dev/null || true

# ── strace summary ────────────────────────────────────────────────────────────
separator
echo "🔬  Syscall profile (strace -f -c, sorted by time)"
separator

# strace -c output goes to stderr, which we redirected to $STRACE_RAW
# It prints a header + table; strip the "strace: Process ... attached" lines
grep -v "^strace:" "$STRACE_RAW" | head -80

# save clean copy
grep -v "^strace:" "$STRACE_RAW" > "$STRACE_OUT"
echo
echo "   Full strace summary saved to: $STRACE_OUT"

# ── extract headline numbers for the report ───────────────────────────────────
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
echo "   Requests/sec: ${RPS}"
echo "   Latency avg : ${LAT_AVG}   p99: ${LAT_P99}"
echo "   Top syscall : ${TOP_SYSCALL} (${TOP_PCT}% of syscall time)"
separator
