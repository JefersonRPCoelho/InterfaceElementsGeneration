"""
Compare the final topology of driver runs.

Two runs that insert the same set of edges in different orders (or in a batch) create the new vertices in a different
order, so the vertex indices differ even when the topology is the same. To compare them independently of the
numbering, we look only at the elements that already existed in the input mesh (they keep their indices) and group
their corners by vertex: two corners are in the same group when they use the same vertex. The set of groups (a
partition of the corners) is the same in two runs if and only if both produced the same topology up to a renumbering of
the vertices. Interface elements are not used in the comparison: they are determined by the continuum elements around
them.

Usage (command line):
    python3 compare.py run1.json run2.json ...

Prints, for each file, the number of points, the number of border half-edges of interface elements (hole and border
sides), the number of collapsed half-edges, and whether its partition is the same as the one of the first file.
"""
import json
import sys


def partition(fn):
    """Load a driver JSON and summarize its final state.

    Returns (partition, number of points, border half-edges of interface elements, collapsed half-edges), where
    partition is a set of frozensets of corners (element, local index) of the original elements, one per vertex.
    """
    with open(fn) as f:
        d = json.load(f)
    s0, sf = d['steps'][0], d['steps'][-1]
    n0 = len(s0['elements'])  # number of elements of the input mesh
    groups = {}
    for e in range(n0):
        for k, v in enumerate(sf['elements'][e]):
            groups.setdefault(v, set()).add((e, k))
    ie_border = sum(1 for e, el in enumerate(sf['elements']) if sf['isIE'][e]
                    for k in range(len(el)) if sf['opp'][sf['first'][e] + k] == -1)
    collapsed = sum(1 for o in sf['opp'] if o == -2)
    return set(frozenset(g) for g in groups.values()), sf['nPoints'], ie_border, collapsed


if __name__ == '__main__':
    reference = None
    for fn in sys.argv[1:]:
        p, n, b, c = partition(fn)
        same = '' if reference is None else ('SAME partition as first' if p == reference else 'DIFFERENT partition')
        print(f'{fn:20s} points={n} IE-border-halfedges={b} collapsed={c} {same}')
        if reference is None:
            reference = p
