from pathlib import Path
import sys

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pandas.plotting import scatter_matrix
import re

def extract_num_from_filename(filename):
    match = re.search(r'output_(\d+)mcu', filename)
    return int(match.group(1)) if match else None


def read_data(path: Path) -> pd.DataFrame:
    if not path.exists():
        raise FileNotFoundError(f"Input file not found: {path}")
    df = pd.read_csv(
        path,
        sep=r"\s+",
        header=None,
        names=["start_mcs", "reply_mcs", "rtt_mcs"],
        engine="python",
        comment="#",
        dtype={0: np.int64, 1: np.int64, 1: np.int64},
    )

    df["row_no"] = np.arange(1, len(df) + 1, dtype=np.int64)
    df["request_period"] = df['start_mcs'].diff()
    df["reply_period"] = df['reply_mcs'].diff()

    return df

def main():

    folder = Path(sys.argv[1])
    files = sorted(folder.glob("output_*mcu"))

    results = []
    for file in files:
        num = extract_num_from_filename(file.name)
        if num is None:
            continue
        df = read_data(file)
        s = df["rtt_mcs"].dropna().astype(float)
        p90 = np.percentile(s, 90)
        std = s.std()
        start_mcs = df["request_period"].dropna().astype(float)
        reply_mcs = df["reply_period"].dropna().astype(float)
        start_std_coeff = start_mcs.std()/start_mcs.mean()
        reply_std_coeff = reply_mcs.std()/reply_mcs.mean()
        results.append({"num": num, "p90": p90, "std": std, "start_std_coeff":start_std_coeff, "reply_std_coeff":reply_std_coeff})

    stats_df = pd.DataFrame(results).sort_values("num")
    stats_df.to_csv("all_master.csv", index=False)
    # Percentile by row
    plt.figure(figsize=(8, 5))
    plt.plot(stats_df["num"], stats_df["p90"], marker='o')
    plt.xscale("log")  
    plt.xlabel("set period, mcs")
    plt.ylabel("90th RTT, mcs")
    plt.title("90th Percentile RTT by set period")
    plt.show()

    # std rtt deviation
    plt.figure(figsize=(8, 5))
    plt.plot(stats_df["num"], stats_df["std"], marker='o', color='orange')
    plt.xscale("log")
    plt.xlabel("set period, mcs")
    plt.ylabel("RTT std deviation")
    plt.title("RTT std deviation by set period")
    plt.show()

    plt.figure(figsize=(8, 5))
    plt.plot(stats_df["num"], stats_df["start_std_coeff"], marker='o', color='orange')
    plt.xscale("log")
    plt.xlabel("set period, mcs")
    plt.ylabel("Real period std deviation")
    plt.title("Real period std deviation by set period")
    plt.show()

    
    plt.figure(figsize=(8, 5))
    plt.plot(stats_df["num"], stats_df["reply_std_coeff"], marker='o', color='orange')
    plt.xscale("log")
    plt.xlabel("set period, mcs")
    plt.ylabel("Reply period std deviation")
    plt.title("Reply period std deviation by set period")
    plt.show()

if __name__ == "__main__":
    sys.exit(main())
