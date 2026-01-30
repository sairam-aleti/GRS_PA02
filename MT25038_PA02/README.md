# Network Data Transfer Analysis (PA02)

**Name:** Sai Ram  \
**Roll Number:** MT25038  \
**Course:** GRS (Graduate Systems)

## Overview
Benchmarks three Linux data-transfer mechanisms (two-copy `sendto`, one-copy `sendmsg`, and zero-copy `MSG_ZEROCOPY`) to study CPU/context-switch overhead vs. memory bandwidth costs for 4 KB messages.

## Project Layout
- `src/` – Server and three client variants plus shared helpers.
- `scripts/` – Namespace setup and benchmarking automation.
- `plots/` – Plotting script for results.
- `data/` – Collected benchmark CSVs.
- `Makefile` – Build targets for all binaries.

## Prerequisites
- Linux with `bash`, `gcc`, `make`, and `python3` (with `matplotlib`, `pandas`).
- Root privileges for network namespace setup and benchmarking (creates veth pairs, moves interfaces).

## Build
```bash
make
```

## Setup Network Namespaces
Creates two namespaces and a veth pair for isolated client/server testing.
```bash
sudo ./scripts/setup_ns.sh
```

## Run Benchmark
Runs server and all three clients across 1, 2, 4, and 8 threads; writes `data/results.csv`.
```bash
sudo ./scripts/benchmark.sh
```

## Generate Plot
```bash
python3 plots/plot_results.py
```
Outputs graphs to `plots/` and reads `data/results.csv`.

## Key Findings (4 KB messages)
- One-copy (`sendmsg`) is the fastest; skipping the extra user-space memcpy is the best trade-off.
- Zero-copy (`MSG_ZEROCOPY`) is slower here; pin/map overhead dominates for small messages.
- At 8 threads, performance drops sharply due to context-switch pressure on limited cores.

## Package for Submission
From the parent directory of `MT25038_PA02`:
```bash
cd ..
tar -czvf MT25038_PA02.tar.gz MT25038_PA02/
```

## Cleanup
```bash
make clean
```

## Notes
- Scripts assume execution from repo root unless specified.
- Re-run `setup_ns.sh` after a reboot or if namespaces are removed.
