import csv, math, statistics, sys, os

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

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: compare_two_runs.py runA.csv runB.csv')
        sys.exit(1)
    a = sys.argv[1]
    b = sys.argv[2]
    runA = read_ranges(a)
    runB = read_ranges(b)
    if not runA or not runB:
        print('Missing data in one of the files')
        sys.exit(1)
    sA = summarize(runA)
    sB = summarize(runB)
    print('Run A:', a)
    for k,v in sA.items(): print(f'  {k}: {v}')
    print('\nRun B:', b)
    for k,v in sB.items(): print(f'  {k}: {v}')
    for T in [0.03, 0.04, 0.06]:
        rejA = sum(1 for x in runA if x > T)
        rejB = sum(1 for x in runB if x > T)
        print(f"\nT={T:.3f}: A_rejects={rejA}/{len(runA)}, B_rejects={rejB}/{len(runB)}")
