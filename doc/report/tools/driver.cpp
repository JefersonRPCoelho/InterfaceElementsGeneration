// Test driver for IBHMInterfaceElementBuilder: inserts edges, logs the operator chosen for each vertex, validates the
// resulting topology after every insertion, and dumps every intermediate state as JSON.
//
// Usage: driver [--batch] <meshfile> <out.json> <he0> <he1> ...
//   meshfile: "np ne / x y (np lines) / n v0 ... v(n-1) (ne lines)" (the format of malha2Hyb.txt)
//   --batch : insert all edges with a single insertInterfaceElements() call instead of one call per edge.
//
// The original sources are compiled unchanged. To read the builder's private state, this translation unit (and only
// this one) uses "#define private public".
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#define private public
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#undef private

static const unsigned BORDER = 0xFFFFFFFFu, COLLAPSED = 0xFFFFFFFEu;

// ---- Snapshot of the mesh -----------------------------------------------------------------------------------------
struct View
{
    std::vector<std::vector<unsigned>> elems; // vertices of each element
    std::vector<unsigned> first;              // first half-edge of each element
    std::vector<unsigned> opp;                // opposite of each half-edge
    unsigned nPoints = 0;
};

static View view(IBHM *m)
{
    View v;
    v.nPoints = m->numberPoints();
    for (unsigned e = 0; e < m->numberOfElements(); e++)
    {
        v.first.push_back(m->_offset[e]);
        std::vector<unsigned> el;
        for (unsigned h = m->_offset[e]; h < m->_offset[e + 1]; h++) el.push_back(m->heVertexIndex(h));
        v.elems.push_back(el);
    }
    unsigned nh = m->_offset[m->numberOfElements()];
    for (unsigned h = 0; h < nh; h++) v.opp.push_back(m->heOpposite(h));
    return v;
}

// half-edge helpers on a View (independent from the structure being tested)
struct HE
{
    const View &v;
    std::vector<unsigned> elemOf, local;
    explicit HE(const View &vv) : v(vv)
    {
        for (unsigned e = 0; e < v.elems.size(); e++)
            for (unsigned k = 0; k < v.elems[e].size(); k++) { elemOf.push_back(e); local.push_back(k); }
    }
    unsigned next(unsigned h) const { unsigned e = elemOf[h]; return v.first[e] + (local[h] + 1) % v.elems[e].size(); }
    unsigned prev(unsigned h) const { unsigned e = elemOf[h]; unsigned n = v.elems[e].size(); return v.first[e] + (local[h] + n - 1) % n; }
    unsigned vert(unsigned h) const { return v.elems[elemOf[h]][local[h]]; }
};

// ---- Validation (V1-V5 of the report; V6 is checked in run()) ------------------------------------------------------
static int validate(const View &v, std::vector<std::string> &msgs)
{
    HE he(v);
    int errors = 0;
    unsigned nh = v.opp.size();
    auto err = [&](const std::string &s) { msgs.push_back(s); errors++; };
    for (unsigned h = 0; h < nh; h++)
    {
        unsigned o = v.opp[h];
        if (o == BORDER) continue;
        if (o == COLLAPSED)
        {
            if (he.vert(h) != he.vert(he.next(h))) err("collapsed he " + std::to_string(h) + " is not degenerate");
            continue;
        }
        if (o >= nh) { err("opposite out of range at he " + std::to_string(h)); continue; }
        if (v.opp[o] != h) err("asymmetric opposite: O[" + std::to_string(h) + "]=" + std::to_string(o) + " but O[" + std::to_string(o) + "]=" + std::to_string(v.opp[o]));
        if (he.vert(h) != he.vert(he.next(o)) || he.vert(he.next(h)) != he.vert(o))
            err("vertex mismatch across he " + std::to_string(h) + " / " + std::to_string(o));
    }
    // Manifold fan check: the corners of every vertex must be connected through h ~ O[prev(h)].
    std::map<unsigned, std::vector<unsigned>> corners;
    for (unsigned h = 0; h < nh; h++) corners[he.vert(h)].push_back(h);
    for (auto &[w, cs] : corners)
    {
        std::map<unsigned, unsigned> parent;
        for (unsigned c : cs) parent[c] = c;
        std::function<unsigned(unsigned)> find = [&](unsigned x) { return parent[x] == x ? x : parent[x] = find(parent[x]); };
        for (unsigned c : cs)
        {
            unsigned p = he.prev(c);
            unsigned o = v.opp[p];
            if (o == COLLAPSED) { o = p; } // the collapsed half-edge itself starts at w: it links both corners of the IE
            if (o != BORDER && parent.count(o)) parent[find(c)] = find(o);
        }
        std::set<unsigned> roots;
        for (unsigned c : cs) roots.insert(find(c));
        if (roots.size() != 1) err("vertex " + std::to_string(w) + " has " + std::to_string(roots.size()) + " disconnected fans");
    }
    for (unsigned p = 0; p < v.nPoints; p++)
        if (!corners.count(p)) err("vertex " + std::to_string(p) + " is not referenced by any element");
    return errors;
}

static const char *opName(int op)
{
    switch (op)
    {
        case 0: return "CANONICAL";
        case 1: return "SPLIT_ELEMENT";
        case 2: return "EXPAND_EDGE";
        case 3: return "OPEN_HOLE";
        default: return "UNDEFINED";
    }
}

static std::string jsonArr(const std::vector<unsigned> &a, bool signedOpp = false)
{
    std::ostringstream s;
    s << "[";
    for (size_t i = 0; i < a.size(); i++)
    {
        if (i) s << ",";
        if (signedOpp && a[i] == BORDER) s << -1;
        else if (signedOpp && a[i] == COLLAPSED) s << -2;
        else s << a[i];
    }
    s << "]";
    return s.str();
}

static int run(IBHM *m, const std::vector<double> &coords, const std::vector<unsigned> &edges, const std::string &out,
               bool batch)
{
    IBHMInterfaceElementBuilder b(m);
    std::ofstream js(out);
    js << "{\"points\":[";
    for (size_t i = 0; i < coords.size() / 2; i++) js << (i ? "," : "") << "[" << coords[2 * i] << "," << coords[2 * i + 1] << "]";
    js << "],\"steps\":[";

    View v0 = view(m);
    std::vector<unsigned> root(v0.nPoints);
    for (unsigned i = 0; i < v0.nPoints; i++) root[i] = i;

    auto dump = [&](const View &v, int step, unsigned edge, unsigned A, unsigned B, const char *opA, const char *opB,
                    unsigned predicted, unsigned created, int errors, const std::vector<std::string> &msgs) {
        if (step > 0) js << ",";
        js << "{\"step\":" << step << ",\"edge\":" << (int)edge << ",\"A\":" << (int)A << ",\"B\":" << (int)B
           << ",\"opA\":\"" << opA << "\",\"opB\":\"" << opB << "\",\"predicted\":" << predicted
           << ",\"created\":" << created << ",\"errors\":" << errors << ",\"nPoints\":" << v.nPoints;
        js << ",\"elements\":[";
        for (size_t e = 0; e < v.elems.size(); e++) js << (e ? "," : "") << jsonArr(v.elems[e]);
        js << "],\"first\":" << jsonArr(v.first) << ",\"opp\":" << jsonArr(v.opp, true);
        js << ",\"isIE\":[";
        for (size_t e = 0; e < v.elems.size(); e++) js << (e ? "," : "") << (b._isInterfaceElement[e] ? 1 : 0);
        js << "],\"root\":" << jsonArr(root);
        std::vector<unsigned> col;
        for (auto &[k, h] : b._collapsed) { col.push_back(k); col.push_back(h); }
        js << ",\"collapsed\":" << jsonArr(col);
        js << ",\"msgs\":[";
        for (size_t i = 0; i < msgs.size(); i++) js << (i ? "," : "") << "\"" << msgs[i] << "\"";
        js << "]}";
        js.flush(); // keep the states already computed if a later step aborts
    };
    dump(v0, 0, BORDER, BORDER, BORDER, "", "", 0, 0, 0, {});

    int total = 0;
    std::vector<std::vector<unsigned>> groups;
    if (batch) groups.push_back(edges);
    else for (unsigned e : edges) groups.push_back({e});

    int step = 0;
    for (auto &g : groups)
    {
        step++;
        View before = view(m);
        HE hb(before);
        unsigned a = g[0];
        unsigned bb = m->heOpposite(a);
        unsigned A = hb.vert(a), B = hb.vert(hb.next(a));
        const char *opA = "", *opB = "";
        // Log the operators only for edges the builder accepts (it rejects border edges and interface element edges).
        if (g.size() == 1 && bb != BORDER && bb != COLLAPSED && !b._isInterfaceElement[hb.elemOf[a]] &&
            !b._isInterfaceElement[hb.elemOf[bb]])
        {
            unsigned x, y;
            opA = opName((int)b.retrieveOperator(a, x, y));
            opB = opName((int)b.retrieveOperator(bb, x, y));
        }
        unsigned predicted = b.computeNumberOfNewVertices(g);
        b.insertInterfaceElements(g);
        View after = view(m);
        unsigned created = after.nPoints - before.nPoints;
        // Track the original vertex of each new vertex: a new vertex always replaces an older one in some old corner.
        root.resize(after.nPoints, BORDER);
        for (size_t e = 0; e < before.elems.size(); e++)
            for (size_t k = 0; k < before.elems[e].size(); k++)
            {
                unsigned nv = after.elems[e][k], ov = before.elems[e][k];
                if (nv >= before.nPoints) root[nv] = root[ov];
            }
        std::vector<std::string> msgs;
        int errors = validate(after, msgs);
        total += errors;
        printf("step %2d edge he=%u (%u-%u)  A:%-13s B:%-13s new=%u predicted=%u %s\n", step, a, A, B, opA, opB,
               created, predicted, errors ? "INVALID" : "ok");
        for (auto &s : msgs) printf("      ! %s\n", s.c_str());
        dump(after, step, a, A, B, opA, opB, predicted, created, errors, msgs);
    }
    js << "]}\n";
    return total;
}

int main(int argc, char **argv)
{
    int arg = 1;
    bool batch = false;
    if (argc > 1 && std::string(argv[1]) == "--batch") { batch = true; arg++; }
    if (argc - arg < 2) { fprintf(stderr, "usage: driver [--batch] <meshfile> <out.json> <he...>\n"); return 1; }
    std::ifstream in(argv[arg]);
    unsigned np, ne;
    in >> np >> ne;
    std::vector<double> c(2 * np);
    for (unsigned i = 0; i < np; i++) in >> c[2 * i] >> c[2 * i + 1];
    std::vector<unsigned> el, off{0};
    for (unsigned i = 0; i < ne; i++)
    {
        unsigned n;
        in >> n;
        for (unsigned k = 0; k < n; k++) { unsigned x; in >> x; el.push_back(x); }
        off.push_back(off.back() + n);
    }
    std::vector<unsigned> edges;
    for (int i = arg + 2; i < argc; i++) edges.push_back(std::stoul(argv[i]));
    std::vector<float> cf(c.begin(), c.end());
    IBHM m(cf, el, off, 2);
    int errors = run(&m, c, edges, argv[arg + 1], batch);
    printf("TOTAL ERRORS: %d\n", errors);
    return errors ? 2 : 0;
}
