#!/bin/bash

# Hardcoded path from your system
PERF_CMD="/usr/lib/linux-tools/6.8.0-94-generic/perf"
OUTPUT_FILE="data/results.csv"
DURATION=10

mkdir -p data
# Header
echo "Implementation,Threads,MessageSize,Throughput_Gbps,Latency_us,ContextSwitches,PageFaults" > $OUTPUT_FILE

# Check for bc
if ! command -v bc &> /dev/null; then
    echo "Error: bc not installed."
    exit 1
fi

CLIENTS=("client_2copy" "client_1copy" "client_0copy")
THREADS=(1 2 4 8)
SIZES=(64 4096 32768 1048576)

echo "Compiling..."
make > /dev/null

echo "Starting Robust Benchmark..."

for client in "${CLIENTS[@]}"; do
    for t in "${THREADS[@]}"; do
        for s in "${SIZES[@]}"; do
            echo "------------------------------------------------"
            echo "Running $client | Threads: $t | Size: $s"

            # Start Server
            sudo ip netns exec ns_server ./bin/server > /dev/null 2>&1 &
            SERVER_PID=$!
            sleep 2

            # Run Client (Standard Output for perf to avoid CSV issues)
            # We filter for specific events that are supported on WSL
            sudo ip netns exec ns_client $PERF_CMD stat \
                -e context-switches,page-faults,task-clock \
                -o perf.log \
                ./bin/$client 192.168.1.2 8080 $t $s > client.log 2>&1

            sudo kill $SERVER_PID 2>/dev/null
            wait $SERVER_PID 2>/dev/null

            # Parse Application Output
            STATS_LINE=$(grep "Stats:" client.log)
            BYTES=$(echo $STATS_LINE | awk '{print $2}')
            LATENCY=$(echo $STATS_LINE | awk '{print $3}')

            if [ -z "$BYTES" ]; then
                # If failed, try to read the error from client.log
                echo "  [!] Error: Client failed."
                cat client.log
                BYTES=0
                LATENCY=0
                THROUGHPUT=0
            else
                THROUGHPUT=$(echo "scale=4; ($BYTES * 8) / ($DURATION * 1000000000)" | bc)
            fi

            # Parse Perf Output (Standard Format: "   1,234      context-switches")
            CS=$(grep "context-switches" perf.log | awk '{print $1}' | tr -d ',')
            PF=$(grep "page-faults" perf.log | awk '{print $1}' | tr -d ',')
            
            # Defaults
            CS=${CS:-0}
            PF=${PF:-0}

            echo "$client,$t,$s,$THROUGHPUT,$LATENCY,$CS,$PF" >> $OUTPUT_FILE
            echo "  -> Tp: $THROUGHPUT Gbps | Lat: $LATENCY us"
            
            sleep 1
        done
    done
done

rm -f client.log perf.log
echo "Done. Results in $OUTPUT_FILE"