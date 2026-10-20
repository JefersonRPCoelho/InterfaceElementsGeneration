"""Random stress test for IBHMInterfaceElementBuilder.

Usage:
    python3 stress.py <driver> <seed> <cases> <work-dir> [--skip-border-multi]

What each case does
-------------------
1. Builds a random mesh on a grid of 3..7 x 3..7 cells. The kind of mesh cycles through four options:
     quad    only quadrilaterals;
     tri     every cell split into two triangles (by one of its two diagonals, chosen at random);
     hybrid  60% of the cells split into triangles, the others kept as quadrilaterals;
     poly    like hybrid, but 25% of the time two neighbor cells are merged into a hexagon.
2. Selects at random up to half of the interior edges (edges shared by two elements).
3. Inserts that same set four times with the driver: three times one edge per call, in random orders and choosing at
   random which of the two half-edges represents each edge, and once as a single batch (driver --batch).
4. Checks that every run is valid (the driver returns 0 only if the checks V1-V5 pass after every step), that the
   number of new vertices is always the predicted one (V6), and that the four runs give the same topology (compare.py).

The case is marked "border_multi" when its selection contains a border vertex that receives two or more selected
edges. Until report v0.4, this configuration failed (defect I1, Open Hole on a border vertex, fixed after it). With
--skip-border-multi, these selections are discarded and drawn again (round 2 of the report).

When a run aborts, the other runs of the case are skipped and their random draws are not made, so the cases that come
after an abort differ from those of a run without aborts (e.g., seed 21 had 141 border_multi cases before the fix of I1
and has 148 after it).

Output
------
One JSON line with the counters:
    cases, ok, invalid, crash          how each case ended (crash = the driver aborted or timed out);
    border_multi, crash_border_multi   cases with the failing configuration, and how many of them crashed;
    order_dependent, batch_differs     valid cases whose orders, or whose batch run, gave a different topology;
    prediction_mismatch                runs where created != predicted vertices;
    canonical, split, expand, hole     operators applied in the first run of the valid cases;
then the assertion that caused each crash, and (cases, ok) per kind of mesh. Lines starting with INVALID, CRASH,
ORDER DEPENDENT or BATCH DIFFERS describe individual problems.

Exit code: 0 if every case passed (no crash, invalid run, order or batch dependence, or wrong prediction), 1 otherwise.
CTest uses it to run the stress test (tests/CMakeLists.txt).

The random generator is seeded, so the same seed always produces the same meshes, selections and orders (the report
uses seeds 21 and 22 with 400 cases).
"""
import json
import os
import random
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from compare import partition  # noqa: E402

KINDS = ['quad', 'tri', 'hybrid', 'poly']


def mesh(kind, nx, ny, rng):
    """Random mesh of the given kind on an nx x ny grid. Returns (points, elements), elements counterclockwise."""
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
    """Write the mesh in the format read by the driver ('np ne', points, then 'n v0 ... v(n-1)' per element)."""
    with open(fn, 'w') as f:
        f.write(f'{len(pts)} {len(elems)}\n')
        for p in pts:
            f.write(f'{p[0]} {p[1]}\n')
        for e in elems:
            f.write(f'{len(e)} ' + ' '.join(map(str, e)) + '\n')


def interior_edges(elems):
    """Interior edges and border vertices of a mesh.

    Returns (edges, border): edges is a list of (h, ho, a, b), where h is the half-edge a->b, ho is its opposite b->a
    and a < b (half-edges numbered as in the IBHM class); border is the set of vertices on the mesh border.
    """
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


def run(driver, out_dir, mesh_file, order, tag, batch=False):
    """Run the driver and return (exit code, JSON path, console output). A timeout is reported as exit code -999."""
    out = os.path.join(out_dir, tag + '.json')
    cmd = [driver] + (['--batch'] if batch else []) + [mesh_file, out] + [str(h) for h in order]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired:
        return -999, out, 'TIMEOUT'
    return r.returncode, out, r.stdout + r.stderr


def main():
    driver, seed, cases, out_dir = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    skip_border_multi = '--skip-border-multi' in sys.argv
    os.makedirs(out_dir, exist_ok=True)

    rng = random.Random(seed)
    stats = dict(cases=0, ok=0, invalid=0, crash=0, crash_border_multi=0, border_multi=0, order_dependent=0,
                 batch_differs=0, prediction_mismatch=0, split=0, expand=0, hole=0, canonical=0)
    asserts = {}
    per_kind = {k: [0, 0] for k in KINDS}
    for case in range(cases):
        # 1. random mesh
        kind = KINDS[case % len(KINDS)]
        nx, ny = rng.randint(3, 7), rng.randint(3, 7)
        pts, elems = mesh(kind, nx, ny, rng)
        mesh_file = os.path.join(out_dir, f'm{case}.txt')
        write(mesh_file, pts, elems)

        # 2. random selection of interior edges (redrawn if it is not allowed)
        edges, border = interior_edges(elems)
        while True:
            sel = rng.sample(edges, rng.randint(1, max(1, len(edges) // 2)))
            deg = {}
            for h, ho, a, b in sel:
                for v in (a, b):
                    deg[v] = deg.get(v, 0) + 1
            border_multi = any(v in border and d >= 2 for v, d in deg.items())
            if not (skip_border_multi and border_multi):
                break
        stats['cases'] += 1
        stats['border_multi'] += border_multi
        per_kind[kind][0] += 1

        # 3. three orders one edge per call (t = 0, 1, 2) and one batch (t = 3)
        parts, crashed, invalid, log = [], False, False, ''
        for t in range(4):
            order = [h if rng.random() < 0.5 else ho for h, ho, a, b in sel]
            rng.shuffle(order)
            code, out, log = run(driver, out_dir, mesh_file, order, f'c{case}_{t}', batch=(t == 3))
            if code != 0 and code != 2:
                crashed = True
                break
            if code == 2:
                invalid = True
                print('INVALID', case, log[-400:])
                break
            with open(out) as f:
                d = json.load(f)
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

        # 4. classify the case
        if crashed:
            stats['crash'] += 1
            stats['crash_border_multi'] += border_multi
            m = re.search(r'Assertion failed: \((.*?)\), function (\w+)', log)
            key = f'{m.group(2)}: {m.group(1)}' if m else log.strip()[-80:]
            asserts[key] = asserts.get(key, 0) + 1
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
    print('crashes by assertion:', json.dumps(asserts))
    print('per kind (cases, ok):', json.dumps(per_kind))

    problems = ('crash', 'invalid', 'order_dependent', 'batch_differs', 'prediction_mismatch')
    return 1 if any(stats[k] for k in problems) else 0


if __name__ == '__main__':
    sys.exit(main())
