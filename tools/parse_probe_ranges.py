#!/usr/bin/env python3
"""
parse_probe_ranges.py

Parse Marlin M1128/M48 logs dumped to a text file (or piped stdin) and compute
statistics for per-attempt pair ranges. The script looks for occurrences of
"range=<float>" (e.g. "range=0.05") and also supports lines that contain
"heights=" followed by comma-separated heights.

Usage:
  python tools\parse_probe_ranges.py /path/to/log.txt
  cat log.txt | python tools\parse_probe_ranges.py

Outputs: count, min, max, mean, median, stddev, 90th and 95th percentiles,
and suggested T values (median, median+std, 95th percentile).
"""
import sys
import re
import math
from statistics import mean, median, stdev

def extract_ranges(text):
    # Match 'range=NUMBER'
    r1 = re.findall(r"range=\s*([+-]?\d*\.?\d+(?:[eE][+-]?\d+)?)", text)
    ranges = [float(x) for x in r1]

    # If no explicit ranges found, try to compute from 'heights=' or 'heights:' patterns
    if not ranges:
        # Look for lines like 'heights=-0.22,-0.17' or 'heights= -0.22, -0.17'
        for m in re.findall(r"heights\s*=\s*([\-0-9.,\s]+)", text):
            try:
                vals = [float(v) for v in re.split(r"\s*,\s*", m.strip()) if v != '']
                if len(vals) >= 2:
                    rng = max(vals) - min(vals)
                    ranges.append(rng)
            except Exception:
                continue
    return ranges

def percentile(data, p):
    if not data:
        return None
    data = sorted(data)
    k = (len(data)-1) * (p/100.0)
    f = math.floor(k)
    c = math.ceil(k)
    if f == c:
        return data[int(k)]
    d0 = data[int(f)] * (c-k)
    d1 = data[int(c)] * (k-f)
    return d0 + d1

def summarize(ranges):
    if not ranges:
        print("No ranges found in input.")
        return
    n = len(ranges)
    mn = min(ranges)
    mx = max(ranges)
    mu = mean(ranges)
    med = median(ranges)
    sd = stdev(ranges) if n > 1 else 0.0
    p90 = percentile(ranges, 90)
    p95 = percentile(ranges, 95)
    print(f"Found {n} range samples")
    print(f"Min: {mn:.5f}")
    print(f"Max: {mx:.5f}")
    print(f"Mean: {mu:.5f}")
    print(f"Median: {med:.5f}")
    print(f"Stddev: {sd:.5f}")
    print(f"90th percentile: {p90:.5f}")
    print(f"95th percentile: {p95:.5f}")
    print("")
    print("Suggested T values:")
    print(f" - Median: {med:.5f}")
    print(f" - Median + 1·std: {med + sd:.5f}")
    print(f" - Median + 2·std: {med + 2*sd:.5f}")
    print(f" - 95th percentile: {p95:.5f}")

if __name__ == '__main__':
    if len(sys.argv) > 1:
        path = sys.argv[1]
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            txt = f.read()
    else:
        txt = sys.stdin.read()
    ranges = extract_ranges(txt)
    summarize(ranges)
