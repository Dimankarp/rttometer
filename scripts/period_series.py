import subprocess
import time

periods_mcu = [50000, 25000, 10000, 5000, 1000, 800, 600, 400, 300, 200, 100, 80, 60, 50]
measure_times = 2000 

output_files = [f"../measures/output_{p}mcu" for p in periods_mcu]
server_ip = "192.168.0.101"
server_port = 5555

rttometer_path = "../build/rttometer"

for p, out_file in zip(periods_mcu, output_files):
    cmd = [rttometer_path, "-p", p, "-n", measure_times, "-o", out_file, server_ip, server_port]
    cmd = list(map(str, cmd))
    print(f"Running: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)
    time.sleep(3)