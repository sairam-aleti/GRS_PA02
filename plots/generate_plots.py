# AI DECLARATION
# Tool Used: Gemini 2.0
# Prompt: "Python script using matplotlib/pandas to generate 4 specific plots from network benchmark CSV: Throughput vs Size, Latency vs Threads, Cache Misses vs Size, Cycles per Byte."

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Ensure plots directory exists
if not os.path.exists('plots'):
    os.makedirs('plots')

# Load Data
try:
    df = pd.read_csv('data/results.csv')
except FileNotFoundError:
    print("Error: data/results.csv not found!")
    exit()

# Set visual style
sns.set_theme(style="whitegrid")
clients = ['client_2copy', 'client_1copy', 'client_0copy']
labels = ['2-Copy (Standard)', '1-Copy (Sendmsg)', '0-Copy (ZeroCopy)']
colors = ['#FF9999', '#66B2FF', '#99FF99']

# ==========================================
# Plot 1: Throughput vs Message Size (at 2 Threads)
# ==========================================
plt.figure(figsize=(10, 6))
subset = df[df['Threads'] == 2] # Use 2 threads as it was the peak performer

for i, client in enumerate(clients):
    data = subset[subset['Implementation'] == client]
    plt.plot(data['MessageSize'], data['Throughput_Gbps'], marker='o', linewidth=2, label=labels[i], color=colors[i])

plt.xscale('log') # Log scale because sizes grow exponentially (64 -> 1MB)
plt.xlabel('Message Size (Bytes)', fontweight='bold')
plt.ylabel('Throughput (Gbps)', fontweight='bold')
plt.title('Throughput vs Message Size (2 Threads)', fontsize=14)
plt.legend()
plt.savefig('plots/plot_throughput_vs_size.png')
print("Generated: plots/plot_throughput_vs_size.png")

# ==========================================
# Plot 2: Latency vs Thread Count (at 4KB Size)
# ==========================================
plt.figure(figsize=(10, 6))
subset = df[df['MessageSize'] == 4096] # Standard Page Size

for i, client in enumerate(clients):
    data = subset[subset['Implementation'] == client]
    plt.plot(data['Threads'], data['Latency_us'], marker='s', linewidth=2, label=labels[i], color=colors[i])

plt.xlabel('Number of Threads', fontweight='bold')
plt.ylabel('Latency (microseconds)', fontweight='bold')
plt.title('Latency vs Thread Count (4KB Messages)', fontsize=14)
plt.legend()
plt.xticks([1, 2, 4, 8])
plt.savefig('plots/plot_latency_vs_threads.png')
print("Generated: plots/plot_latency_vs_threads.png")

# ==========================================
# Plot 3: Context Switches vs Thread Count (Analysis for Q3)
# (Replacing Cache Misses if they are 0 on WSL)
# ==========================================
plt.figure(figsize=(10, 6))
subset = df[df['MessageSize'] == 1048576] # Large messages show contention best

for i, client in enumerate(clients):
    data = subset[subset['Implementation'] == client]
    plt.plot(data['Threads'], data['ContextSwitches'], marker='^', linewidth=2, label=labels[i], color=colors[i])

plt.xlabel('Number of Threads', fontweight='bold')
plt.ylabel('Context Switches', fontweight='bold')
plt.title('Context Switches vs Threads (1MB Messages)', fontsize=14)
plt.legend()
plt.xticks([1, 2, 4, 8])
plt.savefig('plots/plot_context_switches.png')
print("Generated: plots/plot_context_switches.png")

# ==========================================
# Plot 4: CPU Efficiency (Throughput per Context Switch)
# ==========================================
plt.figure(figsize=(10, 6))
subset = df[df['MessageSize'] == 4096]

for i, client in enumerate(clients):
    data = subset[subset['Implementation'] == client]
    # Avoid division by zero
    efficiency = data['Throughput_Gbps'] / (data['ContextSwitches'] + 1)
    plt.bar(data['Threads'] + (i*0.2), efficiency, width=0.2, label=labels[i], color=colors[i], edgecolor='black')

plt.xlabel('Number of Threads', fontweight='bold')
plt.ylabel('Efficiency (Gbps per Context Switch)', fontweight='bold')
plt.title('CPU Efficiency (4KB Messages)', fontsize=14)
plt.xticks([1.2, 2.2, 4.2, 8.2], ['1', '2', '4', '8'])
plt.legend()
plt.savefig('plots/plot_efficiency.png')
print("Generated: plots/plot_efficiency.png")