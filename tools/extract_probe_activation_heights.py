import re, csv, os, statistics

LOG = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'm48_raw.log')
RUNCSV = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'docs', 'Project', 'm48_50_run1.csv')

# timestamp format in CSV: HH:MM:SS.mmm
TS_RE = re.compile(r'(?P<h>\d{2}):(?P<m>\d{2}):(?P<s>\d{2})\.(?P<ms>\d{3})')
ACT_RE = re.compile(r"(?P<ts>\d{2}:\d{2}:\d{2}\.\d{3}).*?PROBE_ACTIVATION_SWITCH became ACTIVE at\s*Z:\s*(?P<z>-?[0-9]+\.?[0-9]*)", re.IGNORECASE)
ANSI_RE = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")

# read run start/end from RUNCSV
with open(RUNCSV, newline='') as f:
    r = csv.DictReader(f)
    rows = list(r)
    if not rows:
        print('empty run csv')
        raise SystemExit(1)
    start_ts = rows[0]['timestamp']
    end_ts = rows[-1]['timestamp']

# convert to comparable tuple
def ts_tuple(ts):
    m = TS_RE.match(ts)
    if not m: return None
    return (int(m.group('h')), int(m.group('m')), int(m.group('s')), int(m.group('ms')))

start = ts_tuple(start_ts)
end = ts_tuple(end_ts)
if not start or not end:
    print('bad timestamps', start_ts, end_ts)
    raise SystemExit(1)

# helper compare
def in_range(ts):
    t = ts_tuple(ts)
    return start <= t <= end

heights = []
with open(LOG, 'r', encoding='utf-8', errors='ignore') as f:
    for l in f:
        clean = ANSI_RE.sub('', l)
        m = ACT_RE.search(clean)
        if m:
            ts = m.group('ts')
            if in_range(ts):
                z = float(m.group('z'))
                heights.append((ts, z))

if not heights:
    print('No activation heights found in run window')
    raise SystemExit(1)

vals = [z for ts,z in heights]
print('Found', len(vals), 'activation heights in run window')
print('Sample timestamps and z (first 10):')
for ts,z in heights[:10]:
    print(' ', ts, z)
print('\nStats:')
print('  mean: {:.5f} mm'.format(statistics.mean(vals)))
print('  median: {:.5f} mm'.format(statistics.median(vals)))
print('  stdev: {:.5f} mm'.format(statistics.pstdev(vals)))
print('  min: {:.5f} mm'.format(min(vals)))
print('  max: {:.5f} mm'.format(max(vals)))

# Recommend Z offset: median
print('\nRecommended Z offset (median of trigger heights): {:.5f} mm'.format(statistics.median(vals)))
