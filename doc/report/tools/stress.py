"""Random stress test for IBHMInterfaceElementBuilder.

Usage: python3 stress.py <driver> <seed> <cases> <work-dir> [--skip-border-multi]

Each case builds a random mesh (quads, triangles, quads+triangles, or quads+triangles+hexagons), selects a random subset
of interior edges, and inserts them three times in random orders (choosing randomly which half-edge represents each
edge) plus once as a single batch. Every run must be valid (driver checks V1-V6) and all four runs must produce the
same topology. With --skip-border-multi, selections where a border vertex receives two or more edges (a configuration
known to fail, defect I1) are discarded and redrawn.
"""
import json
import os
import random
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from compare import partition  # noqa: E402

DRIVER = sys.argv[1]
SEED = int(sys.argv[2])
CASES = int(sys.argv[3])
OUT = sys.argv[4]
SKIP_BORDER_MULTI = '--skip-border-multi' in sys.argv
os.makedirs(OUT, exist_ok=True)
KINDS = ['quad', 'tri', 'hybrid', 'poly']


def mesh(kind, nx, ny, rng):
    pts = [(i, j) for j in range(ny + 1) for i in range(nx + 1)]
    vid = lambda i, j: j * (nx + 1) + i
    elems = []
    j = 0
    while j < ny:
        i = 0
        while i < nx:
            a, b, c, d = vid(i, j), vid(i + 1, j), vid(i + 1, j + 1), vid(i, j + 1)
            r = rng.random()
            if kind == 'poly' and i + 1 < nx and r < 0.25:
                # merge two quads into a hexagon (6 vertices, two of them on straight sides)
                elems.append([a, b, vid(i + 2, j), vid(i + 2, j + 1), c, d])
                i += 2
                continue
            if kind == 'tri' or (kind in ('hybrid', 'poly') and r < 0.6):
                elems += [[a, b, c], [a, c, d]] if rng.random() < 0.5 else [[a, b, d], [b, c, d]]
            else:
                elems.append([a, b, c, d])
            i += 1
        j += 1
    return pts, elems


def write(fn, pts, elems):
    with open(fn, 'w') as f:
        f.write(f'{len(pts)} {len(elems)}\n')
        for p in pts:
            f.write(f'{p[0]} {p[1]}\n')
        for e in elems:
            f.write(f'{len(e)} ' + ' '.join(map(str, e)) + '\n')


def interior_edges(elems):
    he, h = {}, 0
    for e in elems:
        for k in range(len(e)):
            he[(e[k], e[(k + 1) % len(e)])] = h
            h += 1
    edges = [(h, he[(b, a)], a, b) for (a, b), h in he.items() if (b, a) in he and a < b]
    border = set()
    for (a, b) in he:
        if (b, a) not in he:
            border |= {a, b}
    return edges, border


def run(mesh_file, order, tag, batch=False):
    out = os.path.join(OUT, tag + '.json')
    cmd = [DRIVER] + (['--batch'] if batch else []) + [mesh_file, out] + [str(h) for h in order]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired:
        return -999, out, 'TIMEOUT'
    return r.returncode, out, r.stdout + r.stderr


rng = random.Random(SEED)
stats = dict(cases=0, ok=0, invalid=0, crash=0, crash_border_multi=0, border_multi=0, order_dependent=0,
             batch_differs=0, prediction_mismatch=0, split=0, expand=0, hole=0, canonical=0)
per_kind = {k: [0, 0] for k in KINDS}
for case in range(CASES):
    kind = KINDS[case % len(KINDS)]
    nx, ny = rng.randint(3, 7), rng.randint(3, 7)
    pts, elems = mesh(kind, nx, ny, rng)
    mesh_file = os.path.join(OUT, f'm{case}.txt')
    write(mesh_file, pts, elems)
    edges, border = interior_edges(elems)
    while True:
        sel = rng.sample(edges, rng.randint(1, max(1, len(edges) // 2)))
        deg = {}
        for h, ho, a, b in sel:
            for v in (a, b):
                deg[v] = deg.get(v, 0) + 1
        border_multi = any(v in border and d >= 2 for v, d in deg.items())
        if not (SKIP_BORDER_MULTI and border_multi):
            break
    stats['cases'] += 1
    stats['border_multi'] += border_multi
    per_kind[kind][0] += 1
    parts, crashed, invalid = [], False, False
    for t in range(4):
        order = [h if rng.random() < 0.5 else ho for h, ho, a, b in sel]
        rng.shuffle(order)
        code, out, log = run(mesh_file, order, f'c{case}_{t}', batch=(t == 3))
        if code != 0 and code != 2:
            crashed = True
            break
        if code == 2:
            invalid = True
            print('INVALID', case, log[-400:])
            break
        d = json.load(open(out))
        if any(s['predicted'] != s['created'] for s in d['steps'][1:]):
            stats['prediction_mismatch'] += 1
        if t == 0:
            for s in d['steps'][1:]:
                for op in (s['opA'], s['opB']):
                    key = {'SPLIT_ELEMENT': 'split', 'EXPAND_EDGE': 'expand', 'OPEN_HOLE': 'hole',
                           'CANONICAL': 'canonical'}.get(op)
                    if key:
                        stats[key] += 1
        parts.append(partition(out)[0])
    if crashed:
        stats['crash'] += 1
        stats['crash_border_multi'] += border_multi
        if not border_multi:
            print('CRASH without a border vertex with 2+ edges:', case, kind, log[-300:])
    elif invalid:
        stats['invalid'] += 1
    else:
        stats['ok'] += 1
        per_kind[kind][1] += 1
        if any(p != parts[0] for p in parts[:3]):
            stats['order_dependent'] += 1
            print('ORDER DEPENDENT', case)
        if parts[3] != parts[0]:
            stats['batch_differs'] += 1
            print('BATCH DIFFERS', case)
print(json.dumps(stats))
print('per kind (cases, ok):', json.dumps(per_kind))
