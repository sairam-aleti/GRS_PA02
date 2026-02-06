# Network I/O Primitives Analysis (PA02)

**Name:** Sai Ram Reddy Aleti
**Roll Number:** MT25038  
**Course:** GRS (Graduate Systems)

## Problem Statement
Evaluate how different data-path designs affect throughput and latency for a TCP workload. Three client implementations are tested, ranging from traditional data copying to kernel-assisted zero-copy. All clients send data to the same multi-threaded server. Each client must serialize an 8-field string structure before sending it, as required by the assignment.

## Code Architecture
- `src/common.h` / `src/common.c`: Shared socket helpers and the `Message` abstraction (8 dynamically allocated string fields plus lengths). Includes utilities for randomized payload generation and reusable server/client socket setup.
- `src/server.c`: Thread-per-connection TCP sink. Allocates the complex message locally (to mirror client allocation cost), drains incoming bytes, and detaches threads to avoid join overhead.
- `src/client_2copy.c`: Baseline `send()` client; flattens the 8-field message into one contiguous buffer each iteration. Captures average per-send latency using `CLOCK_MONOTONIC`.
- `src/client_1copy.c`: `sendmsg()`/iov variant; avoids the extra user-space memcpy while preserving a copy inside the kernel.
- `src/client_0copy.c`: Enables `SO_ZEROCOPY` and cleans the error queue with `MSG_ERRQUEUE` to reap completions; falls back on completion draining when ENOBUFS is encountered.
- `scripts/setup_ns.sh`: Creates `ns_server` and `ns_client` namespaces with a veth pair (`192.168.1.2/24` ↔ `192.168.1.1/24`) for clean, isolated runs.
- `scripts/benchmark.sh`: End-to-end harness (48 runs = 3 clients × 4 thread counts × 4 message sizes). Collects application stats plus `perf` counters (context switches, L1-icache misses, LLC misses, cycles) into `data/results.csv`.
- `plots/`: `generate_plots.py` and generated figures from the CSV; useful for throughput vs. message size/thread-count inspection.

## Prerequisites
- Linux kernel with `MSG_ZEROCOPY` support and `perf` installed (`sudo apt install linux-tools-common linux-tools-$(uname -r)`).
- `bc` for simple throughput arithmetic inside the benchmark script.
- Passwordless `sudo` recommended for namespace creation and `perf` collection.
- Python 3 with `matplotlib`/`pandas` (for plot generation).

## Build
```bash
make
```
Artifacts land in `bin/` (`server`, `client_0copy`, `client_1copy`, `client_2copy`). Objects are cached in `obj/`.

## Reproducibility Steps
1) **Create network namespaces** (fresh, isolated path):
```bash
sudo ./scripts/setup_ns.sh
```

2) **Run the full benchmark sweep** (10 s per point, writes `data/results.csv`):
```bash
sudo ./scripts/benchmark.sh
```

3) **Generate plots** from the CSV:
```bash
python3 plots/generate_plots.py
```

4) **Manual spot checks** (example: 2 threads, 32 KB message using 1-copy):
```bash
sudo ip netns exec ns_server ./bin/server &
sudo ip netns exec ns_client ./bin/client_1copy 192.168.1.2 8080 2 32768
```

## Methodology
- Duration: Each client thread runs a 10 s tight loop, reporting total bytes and average per-send latency.
- Message format: 8 dynamic string fields; size budget evenly divided across fields to meet the assignment’s structured payload requirement.
- Metrics: Throughput computed as $(\text{bytes} \times 8) / (10 \times 10^9)$ Gbps via `bc`; hardware counters harvested by `perf stat` with namespace isolation to limit host noise.
- Concurrency model: Server is thread-per-connection with detached threads; clients spawn `num_threads` workers to stress the stack symmetrically.

## Highlights from Results (data/results.csv)
- **Throughput leadership:** The `sendmsg` (1-copy) approach achieves the highest throughput, reaching 413 Gbps with 8 threads and 1 MB messages. It slightly outperforms the two-copy design and surpasses the zero-copy approach once message sizes are larger than what fits in cache.
- **Zero-copy tradeoff:** `MSG_ZEROCOPY` performs poorly for messages smaller than 32 KB because of completion tracking overhead and retries caused by ENOBUFS. Performance only catches up when using the largest message sizes.
- **Small-message regime:** With 64 B messages, all implementations are limited by context switching, and throughput remains under 6 Gbps even when using 8 threads.
- **Cache behavior:** In the two-copy design, L1 instruction cache misses increase significantly as thread count grows. This matches the observed leveling off in throughput when scaling from 4 to 8 threads.

## Notes
- Scripts expect to run from repository root. Clean old namespaces with `sudo ip netns del ns_server ns_client` if needed.
- `MSG_ZEROCOPY` requires root and a kernel with the feature enabled; falls back with ENOBUFS handling inside the client.
