from matplotlib import pyplot as plt
import pandas as pd
import numpy as np
input_path = "perf_sched"
f =  open(input_path, 'r');
df = pd.DataFrame(list(map(lambda x: float(x.strip().split()[3][:-1]), f.readlines()[10:])))
df["diff"] = df[0].diff() * 1000000
df["row_no"] = np.arange(1, len(df) + 1, dtype=np.int64)
print(f"90perc: {df['diff'].quantile(0.9) * 1000000} mcs, std: {df['diff'].std()}")
df.to_csv("perf.csv")
plt.figure(figsize=(8, 5))
plt.plot(df["row_no"], df["diff"], marker='o')
plt.show()