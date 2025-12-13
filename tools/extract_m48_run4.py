import re, csv, os

LOG = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'm48_raw.log')
OUT = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'docs', 'Project', 'm48_50_run4.csv')

# Regex to match lines like: "01:15:56.422 > 1 of 50: z: 0.038 Mean: 0.037500 Sigma: 0.000000 Min: 0.038 Max: 0.038 Range: 0.000"
LINE_RE = re.compile(
    r"(?P<ts>\d{2}:\d{2}:\d{2}\.\d{3}).*?(?P<idx>\d+)\s+of\s+50:.*?z:\s*(?P<z>[0-9.]+).*?Mean:\s*(?P<mean>[0-9.]+).*?Sigma:\s*(?P<sigma>[0-9.]+).*?Min:\s*(?P<min>[0-9.]+).*?Max:\s*(?P<max>[0-9.]+).*?Range:\s*(?P<range>[0-9.]+)",
    re.IGNORECASE
)

ANSI_RE = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")

lines = []
with open(LOG, 'r', encoding='utf-8', errors='ignore') as f:
    for l in f:
        # Strip ANSI escape sequences which appear in the log
        clean = ANSI_RE.sub('', l)
        m = LINE_RE.search(clean)
        if m:
            d = m.groupdict()
            try:
                lines.append({
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
                # Skip malformed captures
                continue

if not lines:
    print('No M48 lines found in', LOG)
    raise SystemExit(1)

# Sort by index
lines.sort(key=lambda x: x['index'])

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, 'w', newline='') as csvf:
    w = csv.DictWriter(csvf, fieldnames=['index','timestamp','z','mean','sigma','min','max','range'])
    w.writeheader()
    for row in lines:
        w.writerow(row)

print('Wrote', OUT, 'with', len(lines), 'points')
