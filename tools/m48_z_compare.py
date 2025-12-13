import csv,statistics,sys

fnA='docs/Project/m48_50_run4.csv'
fnB='docs/Project/m48_50_run1.csv'

def zstats(fn):
    zs=[]
    with open(fn,newline='') as f:
        r=csv.DictReader(f)
        for row in r:
            try:
                zs.append(float(row['z']))
            except:
                pass
    return {'count':len(zs),'mean':statistics.mean(zs) if zs else None,'median':statistics.median(zs) if zs else None,'min':min(zs) if zs else None,'max':max(zs) if zs else None}

sA=zstats(fnA)
sB=zstats(fnB)
print('Run4 z stats:',sA)
print('Run5 z stats:',sB)
if sA['mean'] is not None and sB['mean'] is not None:
    d = sB['mean']-sA['mean']
    print('Delta (run5 - run4): {:.6f} mm'.format(d))
else:
    print('Could not compute delta')
