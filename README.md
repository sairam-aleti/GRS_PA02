# Network I/O Primitives Analysis (PA02)

**Name:** Sai Ram  \
**Roll Number:** MT25038  \
**Course:** GRS (Graduate Systems)

## Overview
Benchmarks three network data-transfer mechanisms—2-copy, 1-copy, and 0-copy—to study CPU/context-switch overhead versus memory bandwidth costs.

## Project Structure
- `src/`: Server and three client variants (2-copy, 1-copy, 0-copy) plus shared helpers.
- `scripts/`: Automation for namespaces and full benchmark runs.
- `plots/`: Generated performance graphs and plotting script.
- `data/`: Raw CSV results from benchmarking.
- `Makefile`: Build targets for all binaries.

## How to Run
1) Build
```bash
make
```

2) Setup Network Namespaces (requires sudo for veth creation)
```bash
sudo ./scripts/setup_ns.sh
```

3) Run Full Benchmark (48 tests: 3 clients × 4 thread counts × 4 message sizes)
```bash
sudo ./scripts/benchmark.sh
```

4) Generate Plots
```bash
python3 plots/generate_plots.py
```

## Key Findings & Analysis
1. Throughput hierarchy
	- Winner: 1-copy (`sendmsg`) peaked around ~83 Gbps by skipping the extra user-space memcpy without incurring zero-copy setup costs.
	- Runner up: 2-copy (standard) stays competitive at small messages thanks to CPU cache effects.
	- Loser: 0-copy (`MSG_ZEROCOPY`) underperforms for <32 KB due to page pinning and completion overheads.

2. Thrashing effect
	- At 8 threads, throughput drops across all clients from context-switch pressure; see `plots/plot_context_switches.png` for the spike in switches.

3. Latency
	- Sub-10 µs for small messages; spikes at 1 MB, especially for 0-copy, due to memory management overhead.

## Push to GitHub
```bash
git add .
git commit -m "Final Submission: Completed Benchmark, Plots, and Analysis"
git push origin main
```

## Create the Submission Zip
```bash
mkdir _PA02
cp -r src scripts data plots Makefile README.md _PA02/
zip -r _PA02.zip _PA02
```
