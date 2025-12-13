import csv, os, sys
from datetime import datetime

CSV = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'docs', 'Project', 'm48_50_run2.csv')
FMT = '%H:%M:%S.%f'

if not os.path.isfile(CSV):
    print('Missing file:', CSV)
    sys.exit(1)

times = []
with open(CSV, 'r', newline='') as f:
    r = csv.DictReader(f)
    for row in r:
        ts = row.get('timestamp')
        if not ts:
            continue
        # Some timestamps may be like 00:08:17.868
        try:
            t = datetime.strptime(ts, FMT)
        except Exception:
            # ignore malformed
            continue
        times.append(t)

if not times:
    print('No timestamps found')
    sys.exit(1)

# total duration from first to last
start = times[0]
end = times[-1]
delta = (end - start).total_seconds()

# per-point intervals (between successive points)
intervals = []
for a, b in zip(times, times[1:]):
    intervals.append((b - a).total_seconds())

# compute stats
import statistics
count = len(times)
min_i = min(intervals) if intervals else 0
max_i = max(intervals) if intervals else 0
mean_i = statistics.mean(intervals) if intervals else 0
median_i = statistics.median(intervals) if intervals else 0

print(f'Run2 points: {count}')
print(f'Start: {start.time()}  End: {end.time()}')
print(f'Total duration: {delta:.3f} s')
print('Per-point intervals:')
print(f'  count: {len(intervals)}')
print(f'  min: {min_i:.3f} s')
print(f'  median: {median_i:.3f} s')
print(f'  mean: {mean_i:.3f} s')
print(f'  max: {max_i:.3f} s')

# Also total time per 50 points (already delta) and average speed (points/sec)
points_per_sec = count / delta if delta>0 else 0
print(f'Average point rate: {points_per_sec:.3f} points/s ({1/points_per_sec:.3f} s/point)')

# Suggest estimated duration at different per-point times (for planning):
for tps in [2.0, 3.0, 4.0]:
    est_time = count / tps
    print(f'If speed {tps:.1f} points/s => estimated run time: {est_time:.1f} s')
