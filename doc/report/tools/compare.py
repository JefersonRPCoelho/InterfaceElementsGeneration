import json, sys
def partition(fn):
    d = json.load(open(fn))
    s0, sf = d['steps'][0], d['steps'][-1]
    n0 = len(s0['elements'])
    groups = {}
    for e in range(n0):
        for k, v in enumerate(sf['elements'][e]):
            groups.setdefault(v, set()).add((e, k))
    # holes: count border half-edges belonging to interface elements
    ie_border = sum(1 for e,el in enumerate(sf['elements']) if sf['isIE'][e]
                    for k in range(len(el)) if sf['opp'][sf['first'][e]+k] == -1)
    col = sum(1 for o in sf['opp'] if o == -2)
    return set(frozenset(g) for g in groups.values()), sf['nPoints'], ie_border, col
if __name__ == '__main__':
  ref = None
  for fn in sys.argv[1:]:
      p, n, b, c = partition(fn)
      same = '' if ref is None else ('SAME partition as first' if p == ref else 'DIFFERENT partition')
      print(f'{fn:20s} points={n} IE-border-halfedges={b} collapsed={c} {same}')
      if ref is None: ref = p
