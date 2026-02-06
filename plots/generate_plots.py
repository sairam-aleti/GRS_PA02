import pandas as pd
import matplotlib.pyplot as plt
import os

# Ensure plots directory exists
if not os.path.exists('plots'):
    os.makedirs('plots')

# Load Data
try:
    df = pd.read_csv('data/results.csv')
except FileNotFoundError:
    print("Error: data/results.csv not found! Run the benchmark first.")
    exit()

# Setup Styling (Mimics Seaborn Whitegrid using standard Matplotlib)
plt.style.use('seaborn-v0_8-whitegrid') 
clients = ['client_2copy', 'client_1copy', 'client_0copy']
labels = ['2-Copy (Standard)', '1-Copy (Sendmsg)', '0-Copy (ZeroCopy)']
colors = ['#FF9999', '#66B2FF', '#99FF99'] 
markers = ['o', 's', '^']

# ---------------------------------------------------------
# Plot 1: Throughput vs Message Size
# ---------------------------------------------------------
plt.figure(figsize=(10, 6))
subset = df[df['Threads'] == 2] 

for i, client in enumerate(clients):
    data = subset[subset['Implementation'] == client]
    plt.plot(data['MessageSize'], data['Throughput_Gbps'], 
             marker=markers[i], linewidth=2, label=labels[i], color=colors[i])

plt.xscale('log')
plt.xlabel('Message Size (Bytes) [Log Scale]', fontweight='bold')
plt.ylabel('Throughput (Gbps)', fontweight='bold')
plt.title('Throughput vs Message Size (2 Threads)', fontsize=14)
plt.legend()
plt.tight_layout()
plt.savefig('plots/plot_throughput_vs_size.png')
print("Generated: plots/plot_throughput_vs_size.png")

# ---------------------------------------------------------
# Plot 2: Latency vs Thread Count
# ---------------------------------------------------------
plt.figure(figsize=(10, 6))
subset = df[df['MessageSize'] == 4096] # Standard Page Size (4KB)

for i, client in enumerate(clients):
    data = subset[subset['Implementation'] == client]
    plt.plot(data['Threads'], data['Latency_us'], 
             marker=markers[i], linewidth=2, label=labels[i], color=colors[i])

plt.xlabel('Number of Threads', fontweight='bold')
plt.ylabel('Latency (microseconds)', fontweight='bold')
plt.title('Latency vs Thread Count (4KB Messages)', fontsize=14)
plt.legend()
plt.xticks([1, 2, 4, 8])
plt.tight_layout()
plt.savefig('plots/plot_latency_vs_threads.png')
print("Generated: plots/plot_latency_vs_threads.png")

# ---------------------------------------------------------
# Plot 3: Cache Misses vs Message Size (L1 & LLC)
# ---------------------------------------------------------
plt.figure(figsize=(10, 6))
subset = df[df['Threads'] == 2]

for i, client in enumerate(clients):
    data = subset[subset['Implementation'] == client]
    
    # Plot L1 (Solid Line)
    plt.plot(data['MessageSize'], data['L1_Misses'], 
             marker=markers[i], linewidth=2, label=f"{labels[i]} (L1)", 
             color=colors[i])
    
    # Plot LLC (Dashed Line) - Shows the Zeros!
    plt.plot(data['MessageSize'], data['LLC_Misses'], 
             marker='x', linewidth=1, label=None, # No separate label to keep legend clean
             color=colors[i], linestyle='--', alpha=0.5)

plt.xscale('log')
plt.yscale('symlog', linthresh=10) # Symlog handles 0s correctly!
plt.xlabel('Message Size (Bytes) [Log Scale]', fontweight='bold')
plt.ylabel('Cache Misses (Symlog Scale)', fontweight='bold')
plt.title('Cache Misses: L1 (Solid) vs LLC (Dashed)', fontsize=14)
plt.legend()
plt.tight_layout()
plt.savefig('plots/plot_cache_misses.png')
print("Generated: plots/plot_cache_misses.png")

# ---------------------------------------------------------
# Plot 4: CPU Cycles per Byte Transferred
# ---------------------------------------------------------
plt.figure(figsize=(10, 6))
# We use a large message size to stabilize the metric
subset = df[df['MessageSize'] == 1048576] 

for i, client in enumerate(clients):
    data = subset[subset['Implementation'] == client]
    
    # Avoid division by zero
    tput = data['Throughput_Gbps'].replace(0, 0.0001) 
    total_bytes = tput * 1e9 / 8.0 * 10.0
    cycles_per_byte = data['Cycles'] / total_bytes
    
    plt.plot(data['Threads'], cycles_per_byte, 
             marker=markers[i], linewidth=2, label=labels[i], color=colors[i])

plt.xlabel('Number of Threads', fontweight='bold')
plt.ylabel('CPU Cycles per Byte', fontweight='bold')
plt.title('CPU Cost (Cycles per Byte) vs Thread Count', fontsize=14)
plt.legend()
plt.xticks([1, 2, 4, 8])
plt.tight_layout()
plt.savefig('plots/plot_cycles_per_byte.png')
print("Generated: plots/plot_cycles_per_byte.png")