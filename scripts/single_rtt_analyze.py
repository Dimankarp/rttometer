from pathlib import Path
import sys

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pandas.plotting import scatter_matrix


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


def rtt_stat(rtt_mcs: pd.Series) -> dict:
    s = rtt_mcs.dropna().astype(float)
    stats = {
        "count": int(s.count()),
        "median_mcs": float(s.median()),
        "std_mcs": float(s.std()),
        "p90_mcs": float(np.percentile(s, 90)),
        "p95_mcs": float(np.percentile(s, 95)),
        "p99_mcs": float(np.percentile(s, 99)),
        "max_mcs": float(s.max()),
    }
    return stats


def main():
    
    input_path = Path(sys.argv[1])
    outdir = Path(f"out_{input_path}")
    outdir.mkdir(parents=True, exist_ok=True)

    df = read_data(input_path)

    
    df.to_csv(f"{outdir}/master.csv", index=False)
    stats = rtt_stat(df["rtt_mcs"])

    for i in stats:
        print(f"{i}: {stats[i]}");
    
    scatter_matrix(df, figsize=(8, 8), diagonal='kde', alpha=0.8)
    plt.show()
    
    plt.scatter(df["row_no"], df["request_period"], label='Dataset 1', color='blue', marker='o')
    plt.scatter(df["row_no"], df["reply_period"], label='Dataset 2', color='red', marker='x')
    plt.scatter(df["row_no"], df["rtt_mcs"], label='Dataset 3', color='green', marker='s')
    plt.show()



if __name__ == "__main__":
    sys.exit(main())
