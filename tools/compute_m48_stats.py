import csv, math, statistics, sys, os

ROOT = os.path.dirname(os.path.dirname(__file__))
R2 = os.path.join(ROOT, 'docs', 'Project', 'm48_50_run2.csv')
R3 = os.path.join(ROOT, 'docs', 'Project', 'm48_50_run3.csv')


def read_ranges(path):
    ranges = []
    with open(path, 'r', newline='') as f:
        r = csv.DictReader(f)
        for row in r:
            val = row.get('range')
            if val is None:
                continue
            try:
                ranges.append(float(val))
            except:
                pass
    return ranges


def percentile(sorted_data, p):
    if not sorted_data:
        return None
    n = len(sorted_data)
    if n == 1:
        return sorted_data[0]
    k = (n - 1) * (p / 100.0)
    f = math.floor(k)
    c = math.ceil(k)
    if f == c:
        return sorted_data[int(k)]
    d0 = sorted_data[int(f)] * (c - k)
    d1 = sorted_data[int(c)] * (k - f)
    return d0 + d1


def summarize(data):
    s = sorted(data)
    return {
        'count': len(data),
        'mean': statistics.mean(data),
        'stdev': statistics.pstdev(data),
        'min': min(data),
        'max': max(data),
        'p50': percentile(s, 50),
        'p90': percentile(s, 90),
        'p95': percentile(s, 95),
    }


def main():
    if not os.path.isfile(R2) or not os.path.isfile(R3):
        print('Missing CSV files. Expected:', R2, R3)
        sys.exit(1)
    run2 = read_ranges(R2)
    run3 = read_ranges(R3)
    s2 = summarize(run2)
    s3 = summarize(run3)
    print('Run2 (bed ON, nozzle OFF)')
    for k,v in s2.items():
        print(f'  {k}: {v}')
    print('\nRun3 (bed ON, nozzle ON)')
    for k,v in s3.items():
        print(f'  {k}: {v}')

    for T in [0.03, 0.04, 0.06]:
        rej2 = sum(1 for x in run2 if x > T)
        rej3 = sum(1 for x in run3 if x > T)
        print(f"\nT={T:.3f}: run2_rejects={rej2}/{len(run2)}, run3_rejects={rej3}/{len(run3)}")

if __name__ == '__main__':
    main()
