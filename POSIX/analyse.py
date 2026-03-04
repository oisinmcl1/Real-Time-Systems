import pandas as pd
import matplotlib.pyplot as plt

files = {
    "nanosleep": "output/nanosleep.csv",
    "signal": "output/signal.csv",
    "timer": "output/timer.csv",
}

for name, filepath in files.items():
    # Read each CSV file
    df = pd.read_csv(filepath)
    values = df["value_ns"]

    # Print statistics
    print(f"\n{name}")
    print(f"Count: {len(values)}")
    print(f"Min: {values.min():.0f} ns")
    print(f"Max: {values.max():.0f} ns")
    print(f"Mean: {values.mean():.0f} ns")
    print(f"StdDev: {values.std():.0f} ns")

    # Plot histogram
    plt.figure()
    plt.hist(values, bins=50)
    plt.title(f"{name} latency distribution")
    plt.xlabel("Latency (ns)")
    plt.ylabel("Count")

    # Fix signal graph
    if name == "signal":
        plt.xlim(0, 2000)

    plt.tight_layout()
    plt.savefig(f"output/{name}_graph.png")
    plt.close()
