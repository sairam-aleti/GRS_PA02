#!/bin/bash

# AI DECLARATION
# Tool Used: Gemini 2.0
# Prompt: "Write a bash script to benchmark 3 network clients, varying thread counts. 
# Run each inside a network namespace with perf, parse throughput/context-switches, and save to CSV."

# --- CONFIGURATION ---
# We use the specific binary path to bypass the WSL version mismatch error
PERF_CMD="/usr/lib/linux-tools/6.8.0-94-generic/perf"
OUTPUT_FILE="data/results.csv"
DURATION=10

# Ensure data directory exists
mkdir -p data

# Write the CSV Header
echo "Implementation,Threads,Throughput_Gbps,ContextSwitches,PageFaults" > $OUTPUT_FILE

# Define the tests
CLIENTS=("client_2copy" "client_1copy" "client_0copy")
THREADS=(1 2 4 8)

echo "=========================================================="
echo "Starting Benchmark Automation"
echo "Results will be saved to: $OUTPUT_FILE"
echo "=========================================================="

# Check if bc is installed (for floating point math)
if ! command -v bc &> /dev/null; then
    echo "Error: 'bc' is not installed. Please run: sudo apt-get install bc"
    exit 1
fi

for client in "${CLIENTS[@]}"; do
    for t in "${THREADS[@]}"; do
        echo "------------------------------------------------"
        echo "Benchmarking $client with $t threads..."

        # 1. Start the Server in ns_server (in background)
        # We assume the server listens on 8080.
        sudo ip netns exec ns_server ./bin/server > /dev/null 2>&1 &
        SERVER_PID=$!
        
        # Give server a moment to spin up
        sleep 2

        # 2. Run the Client in ns_client (wrapped in PERF)
        # We capture Client Output (throughput) to client.log
        # We capture Perf Output (events) to perf.log
        # We measure: task-clock (cpu usage), context-switches (overhead), page-faults (memory ops)
        sudo ip netns exec ns_client $PERF_CMD stat \
            -e task-clock,context-switches,page-faults \
            -o perf.log \
            ./bin/$client 192.168.1.2 8080 $t > client.log 2>&1

        # 3. Cleanup: Kill the server
        sudo kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null

        # 4. PARSING RESULTS
        
        # Extract Bytes Sent: Look for line "Total: X bytes"
        # Example output: "2-Copy Total: 41384837120 bytes sent..."
        BYTES=$(grep "Total:" client.log | awk '{print $3}')

        # Calculate Gbps: (Bytes * 8) / (Duration * 10^9)
        if [ -z "$BYTES" ]; then
            THROUGHPUT=0
            echo "  [!] Error: No data sent?"
        else
            THROUGHPUT=$(echo "scale=4; ($BYTES * 8) / ($DURATION * 1000000000)" | bc)
        fi

        # Extract Perf Metrics (removing commas)
        CS=$(grep "context-switches" perf.log | awk '{print $1}' | tr -d ',')
        PF=$(grep "page-faults" perf.log | awk '{print $1}' | tr -d ',')

        # Handle empty perf data
        CS=${CS:-0}
        PF=${PF:-0}

        # 5. Save to CSV
        echo "$client,$t,$THROUGHPUT,$CS,$PF" >> $OUTPUT_FILE
        
        echo "  -> Result: $THROUGHPUT Gbps | CS: $CS | PF: $PF"
        
        # Short cooldown to let sockets close
        sleep 1
    done
done

# Cleanup temp files
rm -f client.log perf.log

echo "=========================================================="
echo "Benchmark Complete."
echo "Data stored in $OUTPUT_FILE"