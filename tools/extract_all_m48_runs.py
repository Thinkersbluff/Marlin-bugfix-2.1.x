import re, csv, os

LOG = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'm48_raw.log')
OUTDIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'docs', 'Project')

M48_HEADER_RE = re.compile(r"M48 Z-Probe Repeatability Test", re.IGNORECASE)
LINE_RE = re.compile(r"(?P<ts>\d{2}:\d{2}:\d{2}\.\d{3}).*?\b(?P<idx>\d+) of 50:\s*z:\s*(?P<z>[0-9.]+)\s+Mean:\s*(?P<mean>[0-9.]+)\s+Sigma:\s*(?P<sigma>[0-9.]+)\s+Min:\s*(?P<min>[0-9.]+)\s+Max:\s*(?P<max>[0-9.]+)\s+Range:\s*(?P<range>[0-9.]+)", re.IGNORECASE)
ANSI_RE = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")

if not os.path.isfile(LOG):
    print('Log not found:', LOG)
    raise SystemExit(1)

runs = []
with open(LOG, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

i = 0
n = len(lines)
while i < n:
    line = lines[i]
    if M48_HEADER_RE.search(line):
        # Found start of a run, scan forward to collect 50 points
        run_points = []
        j = i+1
        while j < n and len(run_points) < 50:
            l = ANSI_RE.sub('', lines[j])
            m = LINE_RE.search(l)
            if m:
                d = m.groupdict()
                try:
                    run_points.append({
                        'index': int(d['idx']),
                        'timestamp': d['ts'],
                        'z': float(d['z']),
                        'mean': float(d['mean']),
                        'sigma': float(d['sigma']),
                        'min': float(d['min']),
                        'max': float(d['max']),
                        'range': float(d['range']),
                    })
                except Exception:
                    pass
            j += 1
        if run_points:
            runs.append(run_points)
        i = j
    else:
        i += 1

if not runs:
    print('No M48 runs found in', LOG)
    raise SystemExit(1)

os.makedirs(OUTDIR, exist_ok=True)
for idx, run in enumerate(runs, start=1):
    out = os.path.join(OUTDIR, f'm48_50_run{idx}.csv')
    with open(out, 'w', newline='') as csvf:
        w = csv.DictWriter(csvf, fieldnames=['index','timestamp','z','mean','sigma','min','max','range'])
        w.writeheader()
        for row in run:
            w.writerow(row)
    print('Wrote', out, 'points:', len(run))

print('Extracted', len(runs), 'runs')
