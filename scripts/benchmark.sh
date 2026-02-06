#!/bin/bash

# Hardcoded path fallback
PERF_CMD=$(which perf 2>/dev/null)
if [ -z "$PERF_CMD" ]; then
    PERF_CMD="/usr/bin/perf"
fi

OUTPUT_FILE="data/results.csv"
DURATION=10

mkdir -p data

# [cite_start]Header matches Assignment Part B Metrics [cite: 58-68]
echo "Implementation,Threads,MessageSize,Throughput_Gbps,Latency_us,ContextSwitches,L1_Misses,LLC_Misses,Cycles" > $OUTPUT_FILE

if ! command -v bc &> /dev/null; then
    echo "Error: 'bc' is not installed. Run: sudo apt install bc"
    exit 1
fi

CLIENTS=("client_2copy" "client_1copy" "client_0copy")
THREADS=(1 2 4 8)
SIZES=(64 4096 32768 1048576)

# --- CLEANUP TRAP ---
cleanup() {
    sudo killall server 2>/dev/null
    rm -f client.log perf.log
}
trap cleanup EXIT INT TERM

echo "Compiling..."
make > /dev/null

echo "=========================================================="
echo "Starting Benchmark on Native Linux"
echo "Capturing: Throughput, Latency, L1-ICACHE-Misses, LLC-Misses, Cycles"
echo "=========================================================="

for client in "${CLIENTS[@]}"; do
    for t in "${THREADS[@]}"; do
        for s in "${SIZES[@]}"; do
            echo "------------------------------------------------"
            echo "Running $client | Threads: $t | Size: $s"

            # 1. Start Server
            sudo ip netns exec ns_server ./bin/server > /dev/null 2>&1 &
            SERVER_PID=$!
            sleep 2

            # 2. Run Client with YOUR SPECIFIC EVENTS
            # L1: Using 'L1-icache-load-misses' because 'dcache-misses' is missing on your CPU
            # LLC: Using 'LLC-load-misses' (confirmed present in your list)
            sudo ip netns exec ns_client $PERF_CMD stat \
                -e context-switches,L1-icache-load-misses,LLC-load-misses,cycles \
                -o perf.log \
                ./bin/$client 192.168.1.2 8080 $t $s > client.log 2>&1

            # 3. Cleanup Server
            sudo kill $SERVER_PID 2>/dev/null
            wait $SERVER_PID 2>/dev/null

            # 4. Parse Application Stats
            STATS_LINE=$(grep "Stats:" client.log)
            BYTES=$(echo $STATS_LINE | awk '{print $2}')
            LATENCY=$(echo $STATS_LINE | awk '{print $3}')

            if [ -z "$BYTES" ]; then
                echo "  [!] Error: Client produced no output."
                BYTES=0; LATENCY=0; THROUGHPUT=0
            else
                THROUGHPUT=$(echo "scale=4; ($BYTES * 8) / ($DURATION * 1000000000)" | bc)
            fi

            # 5. Parse Perf Hardware Counters
            # We use 'head -n 1' to fix the double-line bug
            CS=$(grep "context-switches" perf.log | head -n 1 | awk '{print $1}' | tr -d ',')
            
            # Match strictly against the events we used above
            L1=$(grep "L1-icache-load-misses" perf.log | head -n 1 | awk '{print $1}' | tr -d ',')
            LLC=$(grep "LLC-load-misses" perf.log | head -n 1 | awk '{print $1}' | tr -d ',')
            CYC=$(grep "cycles" perf.log | head -n 1 | awk '{print $1}' | tr -d ',')

            # Handle failures/zeros
            CS=${CS:-0}; L1=${L1:-0}; LLC=${LLC:-0}; CYC=${CYC:-0}
            if [[ "$L1" == *"<"* ]]; then L1=0; fi
            if [[ "$LLC" == *"<"* ]]; then LLC=0; fi

            # 6. Save Data
            echo "$client,$t,$s,$THROUGHPUT,$LATENCY,$CS,$L1,$LLC,$CYC" >> $OUTPUT_FILE
            
            echo "  -> Tp: $THROUGHPUT Gbps | L1 Miss: $L1 | LLC Miss: $LLC"
            sleep 1
        done
    done
done

echo "Benchmark Complete. Data saved to $OUTPUT_FILE"