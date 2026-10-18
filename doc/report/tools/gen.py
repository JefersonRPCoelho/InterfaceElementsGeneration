"""Generate the mesh figures of the report (TikZ files in doc/report/figs/).

How it works
------------
Every mesh picture in the report is a state produced by the real C++ code. For each figure this script:

1. builds a small mesh (or reads one of the repository meshes) and writes it in the text format read by the driver;
2. runs the test driver (`driver.cpp`), which inserts the requested edges with IBHMInterfaceElementBuilder and writes
   one JSON snapshot of the mesh after every insertion (see the format below);
3. loads a snapshot into a `State` object and calls `draw()`, which converts it into a `tikzpicture`;
4. saves the picture in `figs/<name>.tex`, which the report includes with `\\input`.

Figures produced (name in figs/ -> what it shows): example_* (the 12-edge example of the slides), canon_* (canonical
operator), split_x1_* / split_x3_* (split element, both hypotheses), expand_* (expand edge), hole_step* (open hole),
border_fail (the failing border case), ibhm_example / ibhm_arrays (IBHM arrays of meshIBHM.txt), insert_* (arrays
before and after one insertion), stress_* (one mesh of each kind used by stress.py).

The figures must not be edited by hand: change this script and run it again.

Usage
-----
    python3 gen.py <driver-binary> <output-figs-dir> <work-dir>

`<work-dir>` receives the meshes written by this script and the JSON files written by the driver.

JSON written by the driver
--------------------------
    {"points": [[x, y], ...],                 original coordinates (new vertices have no geometry in the code)
     "steps": [                               step 0 is the input mesh; step k is the state after the k-th call
        {"step": k, "edge": he, "A": vA, "B": vB,     inserted half-edge and the vertices of its edge
         "opA": "CANONICAL", "opB": "...",           operator chosen for A and B (empty in batch mode)
         "predicted": n, "created": m,               computeNumberOfNewVertices() and vertices actually created
         "errors": e, "msgs": [...],                 result of the validation checks V1-V5
         "nPoints": n,
         "elements": [[v0, v1, ...], ...],           vertices of each element (the IBHM element list)
         "first": [...],                             first half-edge of each element (the IBHM offsets)
         "opp": [...],                               opposite of each half-edge; -1 = BORDER, -2 = COLLAPSED
         "isIE": [0/1, ...],                         1 for interface elements
         "root": [...],                              original vertex from which each vertex was copied
         "collapsed": [v, he, v, he, ...]}           the builder's _collapsed map (vertex, collapsed half-edge)
     ]}

Exploded view
-------------
New vertices are created at the origin by the code, and copies of a vertex are coincident by definition. To make the
topology visible, every copy of a vertex that was split is drawn slightly away from the original position, towards the
continuum elements that use that copy (`State.positions`). Interface element corners on a collapsed edge are also
pushed apart (`corner_positions`), so the zero-length edge is visible. These positions are only for drawing.
"""
import json
import math
import os
import subprocess
import sys

DRIVER, FIGS, WORK = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(FIGS, exist_ok=True)
os.makedirs(WORK, exist_ok=True)
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))


# ----------------------------------------------------------------------------------------------------------------------
# Meshes and driver
# ----------------------------------------------------------------------------------------------------------------------
def grid(nx, ny):
    """Build a grid of nx x ny unit quads.

    Returns (points, elements, vid): the list of (x, y) points, the list of quads (4 vertex indices in counterclockwise
    order) and a dictionary that maps the grid position (i, j) to the vertex index. Element (i, j) has index i + nx * j.
    """
    pts = [(i, j) for j in range(ny + 1) for i in range(nx + 1)]
    vid = {(i, j): j * (nx + 1) + i for j in range(ny + 1) for i in range(nx + 1)}
    elems = [[vid[(i, j)], vid[(i + 1, j)], vid[(i + 1, j + 1)], vid[(i, j + 1)]] for j in range(ny) for i in range(nx)]
    return pts, elems, vid


def write_mesh(fn, pts, elems):
    """Write a mesh in the format read by the driver (the format of malha2Hyb.txt):
    'np ne', then np lines 'x y', then ne lines 'n v0 ... v(n-1)'."""
    with open(fn, 'w') as f:
        f.write(f'{len(pts)} {len(elems)}\n')
        for p in pts:
            f.write(f'{p[0]} {p[1]}\n')
        for e in elems:
            f.write(f'{len(e)} ' + ' '.join(map(str, e)) + '\n')


def he_index(elems, u, w):
    """Index of the half-edge u->w in the input element list.

    The numbering is the one used by the IBHM class: the elements are stored one after the other, so the k-th
    half-edge of an element is (number of half-edges of the previous elements) + k.
    """
    h = 0
    for e in elems:
        for k in range(len(e)):
            if e[k] == u and e[(k + 1) % len(e)] == w:
                return h + k
        h += len(e)
    raise KeyError((u, w))


def run(mesh, edges, tag):
    """Run the driver on `mesh`, inserting `edges` (half-edge indices) one per call, and return the loaded JSON.

    The JSON is saved as <work-dir>/<tag>.json. If the driver aborts on an assertion (exit codes 134 / -6, used on
    purpose for the figure of the failing border case), the steps written before the abort are kept: the driver
    flushes the file after each step, so the JSON only needs to be closed.
    """
    out = os.path.join(WORK, tag + '.json')
    r = subprocess.run([DRIVER, mesh, out] + [str(e) for e in edges], capture_output=True, text=True)
    print(f'--- {tag}\n{r.stdout.strip()}')
    if r.returncode not in (0, 134, -6):
        raise RuntimeError(r.stdout + r.stderr)
    with open(out) as f:
        txt = f.read()
    try:
        return json.loads(txt)
    except json.JSONDecodeError:  # driver aborted (assertion): close the JSON with the steps already flushed
        return json.loads(txt.rstrip().rstrip(',') + ']}')


# ----------------------------------------------------------------------------------------------------------------------
# State helpers
# ----------------------------------------------------------------------------------------------------------------------
class State:
    """One snapshot of the mesh (one entry of "steps" in the driver JSON) with the half-edge queries.

    The queries are recomputed here from the snapshot, independently of the C++ code: vert(h) is V[h], nxt/prv are
    next/prev inside the element, and self.opp is the opposite table (-1 = BORDER, -2 = COLLAPSED).
    """

    def __init__(self, data, k):
        st = data['steps'][k]
        self.pts = data['points']
        self.st = st
        self.elems, self.first, self.opp = st['elements'], st['first'], st['opp']
        self.isIE, self.root, self.n = st['isIE'], st['root'], st['nPoints']
        # element and local position of every half-edge
        self.he_elem, self.he_loc = [], []
        for e, el in enumerate(self.elems):
            for j in range(len(el)):
                self.he_elem.append(e)
                self.he_loc.append(j)
        c = st['collapsed']
        self.collapsed = dict(zip(c[0::2], c[1::2]))

    def vert(self, h):
        """Vertex where the half-edge h starts (V[h])."""
        return self.elems[self.he_elem[h]][self.he_loc[h]]

    def nxt(self, h):
        """Next half-edge in the same element."""
        e = self.he_elem[h]
        return self.first[e] + (self.he_loc[h] + 1) % len(self.elems[e])

    def prv(self, h):
        """Previous half-edge in the same element."""
        e = self.he_elem[h]
        n = len(self.elems[e])
        return self.first[e] + (self.he_loc[h] + n - 1) % n

    def in_ie(self, w):
        """True if the vertex w is used by some interface element."""
        return any(self.isIE[e] and w in el for e, el in enumerate(self.elems))

    def positions(self, delta):
        """Drawing position of every vertex (the "exploded view").

        A vertex whose original vertex was not split is drawn at the original position. Otherwise, each copy w is
        moved by `delta` in the direction of the average of the unit vectors from the original position to the
        centroids of the continuum elements that use w. As each copy is used by the elements of one sector around the
        original vertex, the copies move apart, each one into its own sector.
        """
        copies = {}
        used = set(v for el in self.elems for v in el)
        for w in used:
            copies.setdefault(self.root[w], []).append(w)
        P = {}
        for w in used:
            r = self.root[w]
            p = self.pts[r]
            if len(copies[r]) <= 1:
                P[w] = tuple(p)
                continue
            dx = dy = 0.0
            for e, el in enumerate(self.elems):
                if self.isIE[e] or w not in el:
                    continue
                cx = sum(self.pts[self.root[v]][0] for v in el) / len(el)
                cy = sum(self.pts[self.root[v]][1] for v in el) / len(el)
                l = math.hypot(cx - p[0], cy - p[1])
                dx += (cx - p[0]) / l
                dy += (cy - p[1]) / l
            l = math.hypot(dx, dy)
            P[w] = (p[0] + delta * dx / l, p[1] + delta * dy / l) if l > 1e-9 else tuple(p)
        return P

    def border_loops(self):
        """Loops of BORDER half-edges (the outer boundary and the holes).

        Starting at a border half-edge u->w, the next one in the loop is the border half-edge that starts at w. A loop
        whose polygon has negative signed area runs clockwise, i.e., it is a hole (the outer boundary is
        counterclockwise, because the elements are on the left of their half-edges).
        """
        border = [h for h in range(len(self.opp)) if self.opp[h] == -1]
        start = {}
        for h in border:
            start.setdefault(self.vert(h), []).append(h)
        seen, loops = set(), []
        for h in border:
            if h in seen:
                continue
            loop, cur = [], h
            while cur not in seen:
                seen.add(cur)
                loop.append(cur)
                w = self.vert(self.nxt(cur))
                cand = [g for g in start.get(w, []) if g not in seen]
                if not cand:
                    break
                cur = cand[0]
            loops.append(loop)
        return loops


# ----------------------------------------------------------------------------------------------------------------------
# TikZ drawing
# ----------------------------------------------------------------------------------------------------------------------
def fmt(p):
    """TikZ coordinate."""
    return f'({p[0]:.3f},{p[1]:.3f})'


def prime_name(base, k):
    """Name of the k-th copy of a vertex called `base`: base, base_1, base_2, ..."""
    return base if k == 0 else '%s_{%d}' % (base, k)


def vertex_names(s, names):
    """Name every vertex of the state `s`.

    `names` maps original vertices to names. The original vertex keeps its name and its copies get subscripts in the
    order they were created (K, K_1, K_2, ...). A copy can also receive an explicit name if its own index is in `names`.
    """
    out = {}
    count = {}
    for w in range(s.n):
        r = s.root[w]
        if w != r and w in names:  # explicit name for a copy (e.g. to show its index)
            out[w] = names[w]
            continue
        if r not in names:
            continue
        k = count.get(r, 0)
        count[r] = k + 1
        out[w] = prime_name(names[r], k)
    return out


def corner_positions(s, P, vthick):
    """Drawing position of the start corner of every half-edge.

    Usually it is the position of the vertex, P[V[h]]. The exception are the two corners of an interface element on a
    collapsed edge (an edge from a vertex to itself): each one is pushed by `vthick` towards the continuum element
    across the long edge that touches that corner, so the collapsed edge is drawn with a small, purely visual length.
    """
    CP = {}
    for h in range(len(s.opp)):
        w = s.vert(h)
        CP[h] = P[w]
        e = s.he_elem[h]
        if not s.isIE[e] or vthick <= 0:
            continue
        if s.vert(s.nxt(h)) != w and s.vert(s.prv(h)) != w:
            continue  # this corner is not on a collapsed edge
        # the long edge of the interface element at this corner: h itself or the previous half-edge
        long_he = h if s.opp[h] >= 0 and s.vert(s.nxt(h)) != w else s.prv(h)
        o = s.opp[long_he]
        if o < 0:
            continue
        el = s.elems[s.he_elem[o]]
        c = (sum(P[v][0] for v in el) / len(el), sum(P[v][1] for v in el) / len(el))
        d = math.hypot(c[0] - P[w][0], c[1] - P[w][1])
        CP[h] = (P[w][0] + vthick * (c[0] - P[w][0]) / d, P[w][1] + vthick * (c[1] - P[w][1]) / d)
    return CP


def draw(s, *, delta=0.12, scale=1.0, names=None, label_all=True, label_set=None, pending=(), current=None,
         arrows=(), he_labels=(), vsize=0.15, font=r'\scriptsize', arrow_shift=0.14, show_numbers=True,
         extra='', clip=None, mark_vertices=None, hole_label=None, elem_labels=None, vthick=None, vlabel_font=None):
    """Convert the state `s` into a tikzpicture (returned as a string).

    Drawing order: holes (hatched), continuum elements (blue), interface elements (orange), collapsed edges (red),
    element labels, edges still to be inserted (red), half-edge arrows, vertices.

    Main parameters:
      delta        distance used to separate the copies of a split vertex (exploded view);
      vthick       visual half-length of collapsed edges (default 0.4 * delta);
      scale, font  TikZ scale and default font;
      names        names of the original vertices (see vertex_names); label_all / label_set choose which are labeled;
      pending      list of (number, (u, w)) original edges still to be inserted, drawn in red with their number;
      current      number of the next edge, drawn thicker;
      arrows       list of (he, label, color) half-edge arrows; he_labels is the same list, used for labeled arrows.
                   An optional 4th item is a dict: 'shift' (distance from the edge towards the element centroid),
                   'shrink' (fraction cut at each end), 'normal' (explicit offset vector), 'lab' (label distance
                   factor), 'at' (label position along the arrow) and 'labdir' (explicit label offset direction);
      clip         ((x0, y0), (x1, y1)) rectangle used to zoom on part of the mesh;
      elem_labels  {element index: text} drawn at the element centroid;
      hole_label   text written inside holes (None for no text);
      extra        raw TikZ code appended at the end.
    The TikZ styles (cefill, iefill, holefill, seledge, curedge, hearrow, vreg, vie, vcol, ...) are defined in
    doc/report/styles.tex.
    """
    P = s.positions(delta)
    CP = corner_positions(s, P, 0.4 * delta if vthick is None else vthick)
    names = names or {}
    vn = vertex_names(s, names)
    L = [r'\begin{tikzpicture}[scale=%.3f, every node/.style={font=%s}]' % (scale, font)]
    if clip:
        L.append(r'\clip %s rectangle %s;' % (fmt(clip[0]), fmt(clip[1])))

    def centroid(e):
        el = range(s.first[e], s.first[e] + len(s.elems[e]))
        return (sum(CP[h][0] for h in el) / len(el), sum(CP[h][1] for h in el) / len(el))

    # holes: loops of border half-edges with negative signed area
    for loop in s.border_loops():
        poly = [CP[h] for h in loop]
        area = sum(poly[i][0] * poly[(i + 1) % len(poly)][1] - poly[(i + 1) % len(poly)][0] * poly[i][1]
                   for i in range(len(poly))) / 2
        if area < -1e-6:
            L.append(r'\fill[holefill] ' + ' -- '.join(fmt(p) for p in poly) + ' -- cycle;')
            if hole_label:
                c = (sum(p[0] for p in poly) / len(poly), sum(p[1] for p in poly) / len(poly))
                L.append(r'\node[font=\tiny\itshape] at %s {%s};' % (fmt(c), hole_label))
    # elements: continuum first, interface elements on top
    for ie_pass in (False, True):
        for e, el in enumerate(s.elems):
            if bool(s.isIE[e]) != ie_pass:
                continue
            style = 'iefill' if s.isIE[e] else 'cefill'
            hs = range(s.first[e], s.first[e] + len(el))
            L.append(r'\filldraw[%s] ' % style + ' -- '.join(fmt(CP[h]) for h in hs) + ' -- cycle;')
    # collapsed edges: short red segment between the two (visually separated) corners
    for h in range(len(s.opp)):
        if s.opp[h] == -2:
            L.append(r'\draw[red, line width=1.1pt] %s -- %s;' % (fmt(CP[h]), fmt(CP[s.nxt(h)])))
    if elem_labels:
        for e, txt in elem_labels.items():
            L.append(r'\node[elemlabel] at %s {%s};' % (fmt(centroid(e)), txt))
    # edges still to be inserted: found by their original vertices in a continuum element
    for num, (u, w) in pending:
        for h in range(len(s.opp)):
            if s.isIE[s.he_elem[h]]:
                continue
            if s.root[s.vert(h)] == u and s.root[s.vert(s.nxt(h))] == w:
                p, q = CP[h], CP[s.nxt(h)]
                is_cur = current is not None and num == current
                L.append(r'\draw[%s] %s -- %s;' % ('curedge' if is_cur else 'seledge', fmt(p), fmt(q)))
                if show_numbers:
                    m = ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2)
                    d = (q[0] - p[0], q[1] - p[1])
                    l = math.hypot(*d)
                    n = (-d[1] / l * 0.16, d[0] / l * 0.16)
                    L.append(r'\node[edgenum] at %s {%s};' % (fmt((m[0] + n[0], m[1] + n[1])), num))
                break
    # half-edge arrows: drawn parallel to the edge, shifted towards the centroid of their element
    for item in list(arrows) + list(he_labels):
        h, lab, col = item[0], item[1], item[2]
        opts = item[3] if len(item) > 3 else {}
        p, q = CP[h], CP[s.nxt(h)]
        if math.hypot(q[0] - p[0], q[1] - p[1]) < 1e-6:
            continue
        c = centroid(s.he_elem[h])
        m = ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2)
        dm = math.hypot(c[0] - m[0], c[1] - m[1])
        sh = min(opts.get('shift', arrow_shift), 0.45 * dm)
        u = ((c[0] - m[0]) / dm * sh, (c[1] - m[1]) / dm * sh) if dm > 1e-9 else (0, 0)
        if 'normal' in opts:
            u = opts['normal']
        sk = opts.get('shrink', 0.22)
        a = (p[0] + (q[0] - p[0]) * sk + u[0], p[1] + (q[1] - p[1]) * sk + u[1])
        b = (q[0] - (q[0] - p[0]) * sk + u[0], q[1] - (q[1] - p[1]) * sk + u[1])
        L.append(r'\draw[hearrow, %s] %s -- %s;' % (col, fmt(a), fmt(b)))
        if lab:
            ls = opts.get('lab', 1.9)
            pos = opts.get('at', 0.5)
            lu = opts.get('labdir', u)
            lp = (a[0] + (b[0] - a[0]) * pos + lu[0] * (ls - 1), a[1] + (b[1] - a[1]) * pos + lu[1] * (ls - 1))
            L.append(r'\node[helabel, text=%s] at %s {$%s$};' % (col, fmt(lp), lab))
    # vertices: red ring = has a collapsed edge; orange label = belongs to an interface element
    for w in sorted(P):
        p = P[w]
        r = s.root[w]
        col = s.collapsed.get(w) is not None
        ie = s.in_ie(w)
        style = 'vcol' if col else ('vie' if ie else 'vreg')
        show = label_all if label_set is None else (w in label_set or r in label_set)
        if mark_vertices and w in mark_vertices:
            style += ', ' + mark_vertices[w]
        if vlabel_font:
            style += ', font=' + vlabel_font
        if show and w in vn:
            L.append(r'\node[%s, minimum size=%.2fcm] at %s {$%s$};' % (style, vsize * 2.4, fmt(p), vn[w]))
        else:
            L.append(r'\node[%s, dot] at %s {};' % (style, fmt(p)))
    L.append(extra)
    L.append(r'\end{tikzpicture}')
    return '\n'.join(L)


def save(name, txt):
    """Write figs/<name>.tex."""
    with open(os.path.join(FIGS, name + '.tex'), 'w') as f:
        f.write(txt + '\n')


# ----------------------------------------------------------------------------------------------------------------------
# 1. Full example (the mesh of the slides, edges in the slide order)
#    Figures: example_initial, example_final, example_final_small, example_step00..12; table: example_table.
# ----------------------------------------------------------------------------------------------------------------------
EX_NAMES = dict(enumerate('A B C D E F G H I J K L M N P Q R S T'.split()))
ex_mesh = os.path.join(REPO, 'malha2Hyb.txt')
# vertex 18 (T) is stored as (3,4) in malha2Hyb.txt, which makes the triangle M-T-S degenerate; the slides place
# T at (4,3), so we use that position for drawing only.
ex_slide_order = [31, 34, 11, 26, 47, 38, 23, 19, 30, 42, 29, 45]
ex = run(ex_mesh, ex_slide_order, 'example')
ex['points'][18] = [4, 3]
ex_edges = []  # (number, (u, w)) in original vertices
s0 = State(ex, 0)
for i, h in enumerate(ex_slide_order):
    ex_edges.append((i + 1, (s0.vert(h), s0.vert(s0.nxt(h)))))

save('example_initial', draw(State(ex, 0), names=EX_NAMES, pending=ex_edges, scale=1.25, vsize=0.17,
                             arrows=[(h, None, 'black!55') for h in range(48)], arrow_shift=0.1))
save('example_final', draw(State(ex, 12), names=EX_NAMES, scale=3.2, delta=0.17, vsize=0.12, vthick=0.07,
                           vlabel_font=r'\tiny\bfseries', hole_label=''))
save('example_final_small', draw(State(ex, 12), names=EX_NAMES, scale=1.9, delta=0.17, label_all=False, vsize=0.08,
                                 vthick=0.07))
OPS = {'CANONICAL': 'Canônico', 'SPLIT_ELEMENT': 'Split Element', 'EXPAND_EDGE': 'Expand Edge', 'OPEN_HOLE': 'Open Hole'}
steps_tbl = []
for k in range(0, 13):
    s = State(ex, k)
    pend = [(n, e) for n, e in ex_edges if n > k]
    cur = k + 1 if k < 12 else None
    save(f'example_step{k:02d}', draw(s, names=EX_NAMES, pending=pend, current=cur, scale=1.05, delta=0.14,
                                      label_all=False, vsize=0.09, show_numbers=True, font=r'\tiny'))
    if k > 0:
        st = ex['steps'][k]
        vn = vertex_names(State(ex, k), EX_NAMES)
        steps_tbl.append((k, vn.get(st['A'], st['A']), vn.get(st['B'], st['B']), OPS[st['opA']], OPS[st['opB']],
                          st['created']))
with open(os.path.join(FIGS, 'example_table.tex'), 'w') as f:
    f.write('\\begin{tabular}{clllc}\n\\toprule\n'
            'Aresta & $A$--$B$ (no código) & Operador em $A$ & Operador em $B$ & vértices novos\\\\\n\\midrule\n')
    for k, A, B, oa, ob, c in steps_tbl:
        f.write(f'{k} & ${A}$--${B}$ & {oa} & {ob} & {c} \\\\\n')
    f.write('\\midrule\n\\multicolumn{4}{r}{total} & %d\\\\\n\\bottomrule\n\\end{tabular}\n' % sum(t[-1] for t in steps_tbl))

# ----------------------------------------------------------------------------------------------------------------------
# 2. Canonical operator: internal edge (3x2 grid) and edge that reaches the border (2x2 grid)
#    Figures: canon_int_before/after, canon_border_before/after.
# ----------------------------------------------------------------------------------------------------------------------
pts, el, vid = grid(3, 2)
mesh = os.path.join(WORK, 'g32.txt')
write_mesh(mesh, pts, el)
A, B = vid[(1, 1)], vid[(2, 1)]
a = he_index(el, A, B)
d = run(mesh, [a], 'canon_int')
names = {A: 'A', B: 'B'}
s0, s1 = State(d, 0), State(d, 1)
b = s0.opp[a]
ie = len(s1.elems) - 1
h0 = s1.first[ie]
save('canon_int_before', draw(s0, names=names, scale=1.4, delta=0.0, label_set={A, B}, pending=[(1, (A, B))],
                              show_numbers=False, he_labels=[(a, 'a', 'red!70!black'), (b, 'b', 'blue!70!black')],
                              vsize=0.13, elem_labels={4: 'U', 1: 'L'}))
save('canon_int_after', draw(s1, names=names, scale=1.4, delta=0.22, label_set={A, B}, vthick=0.12,
                             he_labels=[(a, 'a', 'red!70!black', {'shift': 0.22, 'lab': 1.6, 'at': 0.1}),
                                        (b, 'b', 'blue!70!black', {'shift': 0.22, 'lab': 1.6, 'at': 0.1}),
                                        (h0 + 3, 'h_3', 'black', {'normal': (0.2, 0), 'lab': 2.0, 'labdir': (-0.42, 0.2), 'shrink': 0.05}),
                                        (h0 + 1, 'h_1', 'black', {'normal': (-0.2, 0), 'lab': 2.0, 'labdir': (0.42, 0.2), 'shrink': 0.05})],
                             vsize=0.13, elem_labels={4: 'U', 1: 'L'}))

pts, el, vid = grid(2, 2)
mesh = os.path.join(WORK, 'g22.txt')
write_mesh(mesh, pts, el)
A, B = vid[(1, 1)], vid[(1, 2)]
a = he_index(el, A, B)
d = run(mesh, [a], 'canon_border')
names = {A: 'A', B: 'B'}
s0, s1 = State(d, 0), State(d, 1)
b = s0.opp[a]
save('canon_border_before', draw(s0, names=names, scale=1.4, delta=0.0, label_set={A, B}, pending=[(1, (A, B))],
                                 show_numbers=False, he_labels=[(a, 'a', 'red!70!black'), (b, 'b', 'blue!70!black')],
                                 vsize=0.13))
save('canon_border_after', draw(s1, names=names, scale=1.4, delta=0.22, label_set={A, B},
                                he_labels=[(a, 'a', 'red!70!black'), (b, 'b', 'blue!70!black')],
                                vsize=0.13))

# ----------------------------------------------------------------------------------------------------------------------
# 3. Split element (both hypotheses, 3x4 grid) and expand edge (4x2 grid)
#    The first edge A-B is opened (both ends collapsed); the second edge leaves B going up (hypothesis x1) or down
#    (hypothesis x3). Figures: split_x1_before/after, split_x3_before/after, expand_before/rotation/after.
# ----------------------------------------------------------------------------------------------------------------------
pts, el, vid = grid(3, 4)
mesh = os.path.join(WORK, 'g34.txt')
write_mesh(mesh, pts, el)
A, B, C, D = vid[(1, 2)], vid[(2, 2)], vid[(2, 3)], vid[(2, 1)]
e1 = he_index(el, A, B)
names = {A: 'A', B: 'B', C: 'C', D: 'D'}
for tag, (u, w), S_el in (('split_x1', (B, C), 7), ('split_x3', (B, D), 4)):
    he = he_index(el, u, w)               # the half-edge that starts at B (the one retrieveOperator receives)
    d = run(mesh, [e1, he], tag)
    s1, s2 = State(d, 1), State(d, 2)
    c = s1.collapsed[B]
    b = s1.opp[he]
    if tag == 'split_x1':
        x1 = s1.opp[s1.prv(he)]
        lab = [(he, r'he', 'red!70!black', {'lab': 2.2}), (b, 'b', 'blue!70!black'),
               (s1.prv(he), r'\mathrm{prev}(he)', 'violet', {'lab': 2.6}),
               (x1, 'x_1', 'green!45!black', {'shift': 0.05, 'lab': 3.6})]
    else:
        x3 = s1.opp[s1.nxt(b)]
        lab = [(he, r'he', 'red!70!black', {'lab': 2.2}), (b, 'b', 'blue!70!black'),
               (s1.nxt(b), r'\mathrm{next}(b)', 'violet', {'lab': 2.6}),
               (x3, 'x_3', 'green!45!black', {'shift': 0.05, 'lab': 3.6})]
    save(tag + '_before', draw(s1, names=names, scale=2.0, delta=0.22, label_set={A, B, C, D}, clip=((0.45, 0.55), (2.8, 3.45)),
                               pending=[(2, (u, w))], show_numbers=False, he_labels=lab, vsize=0.12,
                               elem_labels={S_el: 'S'}))
    s2_ie = len(s2.elems) - 1
    save(tag + '_after', draw(s2, names=names, scale=2.0, delta=0.22, label_set={A, B, C, D}, vsize=0.12, clip=((0.45, 0.55), (2.8, 3.45)),
                              elem_labels={S_el: 'S', s2_ie: ''}))

pts, el, vid = grid(4, 2)
mesh = os.path.join(WORK, 'g42.txt')
write_mesh(mesh, pts, el)
A, B, C = vid[(1, 1)], vid[(2, 1)], vid[(3, 1)]
names = {A: 'A', B: 'B', C: 'C'}
e1, e2 = he_index(el, A, B), he_index(el, B, C)
d = run(mesh, [e1, e2], 'expand')
s1, s2 = State(d, 1), State(d, 2)
c = s1.collapsed[B]
save('expand_before', draw(s1, names=names, scale=1.7, delta=0.2, clip=((0.45, 0.3), (3.55, 1.7)), label_set={A, B, C}, pending=[(2, (B, C))],
                           show_numbers=False, vsize=0.12,
                           he_labels=[(e2, 'he', 'red!70!black'), (c, 'c', 'orange!80!black'),
                                      (s1.opp[e2], 'b', 'blue!70!black')]))
# corners visited by reindexElementsCCW(c, he, B_1): c, then opposite(prev(.)) until he
visit, h = [], c
while h != e2:
    visit.append(h)
    h = s1.opp[s1.prv(h)]
save('expand_rotation', draw(s1, names=names, scale=1.7, delta=0.2, clip=((0.45, 0.3), (3.55, 1.7)), label_set={A, B, C}, pending=[(2, (B, C))],
                             show_numbers=False, vsize=0.12,
                             he_labels=[(g, str(i + 1), 'orange!80!black', {'lab': 2.4} if i else {'lab': 2.0, 'labdir': (-0.22, 0.2)}) for i, g in enumerate(visit)] +
                                       [(e2, r'he=\mathrm{stop}', 'red!70!black', {'lab': 2.3})]))
save('expand_after', draw(s2, names=names, scale=1.7, delta=0.2, clip=((0.45, 0.3), (3.55, 1.7)), label_set={A, B, C}, vsize=0.12))

# ----------------------------------------------------------------------------------------------------------------------
# 4. Open hole: four edges arriving at the center V of a 4x4 grid. Figures: hole_step0..4 (zoom on the center).
# ----------------------------------------------------------------------------------------------------------------------
pts, el, vid = grid(4, 4)
mesh = os.path.join(WORK, 'g44.txt')
write_mesh(mesh, pts, el)
V, W, E, N, S = vid[(2, 2)], vid[(1, 2)], vid[(3, 2)], vid[(2, 3)], vid[(2, 1)]
names = {V: 'V', W: 'W', E: 'E', N: 'N', S: 'S'}
seq = [he_index(el, W, V), he_index(el, V, E), he_index(el, V, N), he_index(el, S, V)]
d = run(mesh, seq, 'hole')
for k in range(5):
    s = State(d, k)
    pend = [(i + 1, (s.root[State(d, 0).vert(h)], s.root[State(d, 0).vert(State(d, 0).nxt(h))]))
            for i, h in enumerate(seq) if i + 1 > k]
    save(f'hole_step{k}', draw(s, names=names, scale=1.9, delta=0.26, label_set={V}, pending=pend,
                               current=k + 1 if k < 4 else None, vsize=0.1, clip=((0.85, 0.85), (3.15, 3.15)),
                               vlabel_font=r'\tiny\bfseries'))

# ----------------------------------------------------------------------------------------------------------------------
# 5. Border vertex with two selected edges (the case that fails, defect I1). The driver aborts on the second edge, so
#    the figure shows the state after the first one. Figure: border_fail.
# ----------------------------------------------------------------------------------------------------------------------
fan = os.path.join(WORK, 'fan.txt')
fan_pts = [(0, 0), (1, 0), (0.5, 0.866), (-0.5, 0.866), (-1, 0), (1.2, 0.7), (0, 1.4), (-1.2, 0.7)]
fan_el = [[0, 1, 5, 2], [0, 2, 6, 3], [0, 3, 7, 4]]
write_mesh(fan, fan_pts, fan_el)
d = run(fan, [he_index(fan_el, 0, 2), he_index(fan_el, 0, 3)], 'fan')
s1 = State(d, 1)
save('border_fail', draw(s1, names={0: 'Q', 2: 'R', 3: 'P'}, scale=2.2, delta=0.15, label_set={0, 2, 3},
                         pending=[(2, (0, 3))], show_numbers=False, vsize=0.12,
                         he_labels=[(he_index(fan_el, 0, 3), 'a', 'red!70!black')]))

# ----------------------------------------------------------------------------------------------------------------------
# 6. IBHM arrays of meshIBHM.txt (no insertion: the arrays built by the IBHM class). Figure: ibhm_example; table:
#    ibhm_arrays.
# ----------------------------------------------------------------------------------------------------------------------
ib_mesh = os.path.join(REPO, 'meshIBHM.txt')
d = run(ib_mesh, [], 'ibhm_arrays')
s = State(d, 0)
V = [s.vert(h) for h in range(len(s.opp))]
O = s.opp
first = s.first + [len(V)]
np_ = s.n
ib_el = s.elems
save('ibhm_example', draw(s, names={i: str(i) for i in range(np_)}, scale=2.5, delta=0.0, vsize=0.12,
                          he_labels=[(h, str(h), 'black!70', {'lab': 1.9, 'shift': 0.13, 'shrink': 0.25} if h < first[-2] else {'lab': 2.3, 'shift': 0.07, 'shrink': 0.3}) for h in range(len(V))],
                          elem_labels={e: r'$c_{%d}$' % e for e in range(len(ib_el))}, font=r'\scriptsize'))
with open(os.path.join(FIGS, 'ibhm_arrays.tex'), 'w') as f:
    f.write(r'\begin{tabular}{l' + 'c' * len(V) + '}\n')
    f.write(r'\textit{he} & ' + ' & '.join(str(h) for h in range(len(V))) + r' \\ \hline' + '\n')
    f.write(r'$V[\,]$ & ' + ' & '.join(str(v) for v in V) + r' \\' + '\n')
    f.write(r'$O[\,]$ & ' + ' & '.join(('--' if o < 0 else str(o)) for o in O) + r' \\' + '\n')
    f.write(r'\end{tabular}' + '\n')
    f.write(r'\par\medskip $Of[\,] = [' + ',\\,'.join(str(x) for x in first) + ']$\n')

# ----------------------------------------------------------------------------------------------------------------------
# 7. The IBHM arrays before and after one insertion (small hybrid mesh, edge 4-7).
#    Figures: insert_before, insert_after; table: insert_arrays (changed values highlighted).
# ----------------------------------------------------------------------------------------------------------------------
sm_pts = [(i, j) for j in range(3) for i in range(3)]
sm_el = [[0, 1, 4, 3], [1, 2, 5, 4], [3, 4, 7], [3, 7, 6], [4, 5, 8], [4, 8, 7]]
sm = os.path.join(WORK, 'small.txt')
write_mesh(sm, sm_pts, sm_el)
a = he_index(sm_el, 4, 7)
d = run(sm, [a], 'small')
s0, s1 = State(d, 0), State(d, 1)
sm_names = {i: str(i) for i in range(9)}
save('insert_before', draw(s0, names=sm_names, scale=2.1, delta=0.0, vsize=0.11, pending=[(1, (4, 7))],
                           show_numbers=False,
                           he_labels=[(h, str(h), 'red!70!black' if h in (a, s0.opp[a]) else 'black!70',
                                       {'lab': 1.8, 'shift': 0.11, 'shrink': 0.25}) for h in range(len(s0.opp))]))
ie = len(s1.elems) - 1
ie_he = list(range(s1.first[ie], s1.first[ie] + 4))
save('insert_after', draw(s1, names={**sm_names, 9: '9'}, scale=2.1, delta=0.2, vsize=0.11, vthick=0.1,
                          he_labels=[(h, str(h), 'black!70', {'lab': 1.8, 'shift': 0.11, 'shrink': 0.25})
                                     for h in range(len(s1.opp)) if h not in ie_he] +
                                    [(ie_he[0], str(ie_he[0]), 'orange!50!black', {'normal': (0.06, 0), 'lab': 2.0, 'labdir': (0.18, -0.25)}),
                                     (ie_he[2], str(ie_he[2]), 'orange!50!black', {'normal': (-0.06, 0), 'lab': 2.0, 'labdir': (-0.2, 0.25)})]))


def arrays_table(s_before, s_after):
    """LaTeX table with V[] and O[] before and after an insertion, plus the offsets.

    Old positions whose value changed are shown in red; the positions of the new element, in orange. In O[], '--' is
    BORDER and 'C' (small caps) is COLLAPSED.
    """
    nb, na = len(s_before.opp), len(s_after.opp)
    fmt_o = lambda o: '--' if o == -1 else (r'\textsc{c}' if o == -2 else str(o))
    rows = []
    vb = [s_before.vert(h) for h in range(nb)]
    va = [s_after.vert(h) for h in range(na)]

    def cell(new, old):
        return new if old is None or new == old else r'\textcolor{red!75!black}{\textbf{%s}}' % new
    rows.append(r'\textit{he} & ' + ' & '.join(str(h) for h in range(na)) + r' \\ \hline')
    rows.append(r'$\V$ antes & ' + ' & '.join(str(vb[h]) if h < nb else '' for h in range(na)) + r' \\')
    rows.append(r'$\V$ depois & ' + ' & '.join(cell(str(va[h]), str(vb[h]) if h < nb else None) if h < nb
                                                 else r'\textcolor{orange!50!black}{\textbf{%d}}' % va[h] for h in range(na)) + r' \\ \hline')
    rows.append(r'$\Op$ antes & ' + ' & '.join(fmt_o(s_before.opp[h]) if h < nb else '' for h in range(na)) + r' \\')
    rows.append(r'$\Op$ depois & ' + ' & '.join(cell(fmt_o(s_after.opp[h]), fmt_o(s_before.opp[h])) if h < nb
                                                  else r'\textcolor{orange!50!black}{\textbf{%s}}' % fmt_o(s_after.opp[h]) for h in range(na)) + r' \\')
    txt = r'\begin{tabular}{l' + 'c' * na + '}\n' + '\n'.join(rows) + '\n' + r'\end{tabular}' + '\n'
    txt += r'\par\smallskip $Of$ antes $= [' + r',\,'.join(str(x) for x in s_before.first + [nb]) + r']$\qquad '
    txt += r'$Of$ depois $= [' + r',\,'.join(str(x) for x in s_after.first + [na]) + r']$' + '\n'
    return txt


with open(os.path.join(FIGS, 'insert_arrays.tex'), 'w') as f:
    f.write(arrays_table(s0, s1))

# ----------------------------------------------------------------------------------------------------------------------
# 8. One mesh of each kind generated by stress.py (same generator, fixed seed), with the selected edges in red.
#    Figures: stress_quad, stress_tri, stress_hybrid, stress_poly.
# ----------------------------------------------------------------------------------------------------------------------
import random  # noqa: E402
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import stress  # noqa: E402  (only its functions are used; its main() is not executed on import)

for kind in stress.KINDS:
    rng = random.Random(3)
    pts, elems = stress.mesh(kind, 6, 4, rng)
    mesh = os.path.join(WORK, f'stress_{kind}.txt')
    write_mesh(mesh, pts, elems)
    edges, border = stress.interior_edges(elems)
    sel = rng.sample(edges, len(edges) // 4)
    d = run(mesh, [], f'stress_{kind}')
    save(f'stress_{kind}', draw(State(d, 0), scale=0.62, label_all=False, vsize=0.05,
                                pending=[(i, (a, b)) for i, (h, ho, a, b) in enumerate(sel)], show_numbers=False))
print('figures written to', FIGS)
