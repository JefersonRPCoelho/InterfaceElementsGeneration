// Test driver for IBHMInterfaceElementBuilder.
//
// It reads a mesh, inserts interface elements in the given edges, prints the operator chosen for each vertex, checks
// the topology after every insertion, and writes every intermediate state of the mesh to a JSON file (used by gen.py
// to draw the figures and by stress.py / compare.py to compare runs).
//
// Usage:
//     driver [--batch] <meshfile> <out.json> <he0> <he1> ...
//   meshfile  "np ne", then np lines "x y", then ne lines "n v0 ... v(n-1)" (the format of malha2Hyb.txt).
//   out.json  output file (format below).
//   he0 ...   one half-edge index per edge to open (either half-edge of the edge). An index that is not a half-edge of
//             the mesh is passed to the builder as it is, to test its validation (its vertices are printed as BORDER).
//   --batch   insert all edges with a single insertInterfaceElements() call instead of one call per edge.
//
// Console output: one line per call, e.g.
//     step  2 edge he=34 (10-11)  A:SPLIT_ELEMENT B:CANONICAL     new=1 predicted=1 ok
// with the half-edge, its two vertices, the operator chosen for each vertex, the number of vertices created and the
// number predicted by computeNumberOfNewVertices(), and "ok" or "INVALID" (followed by the failed checks).
//
// Checks after every call (numbered as in the report):
//   V1  opposites are symmetric: O[O[h]] = h for every valid opposite;
//   V2  opposites agree on the vertices: V[h] = V[next(O[h])] and V[next(h)] = V[O[h]];
//   V3  a COLLAPSED half-edge has zero length: V[h] = V[next(h)];
//   V4  manifold: the corners of every vertex form a single fan, connected through h ~ O[prev(h)];
//   V5  every vertex is used by some element;
//   V6  the number of created vertices equals the prediction (printed and stored in the JSON; checked by stress.py);
//   V7  every copy of a vertex has the same coordinates as its original vertex.
//
// JSON: {"points": [[x, y], ...], "steps": [step 0 = input mesh, step k = after the k-th call]}, where each step has
//   edge, A, B, opA, opB           the inserted half-edge, its vertices and the operators (empty in batch mode);
//   predicted, created, errors, msgs (V1-V5 and V7), nPoints;
//   elements, first, opp, isIE     the mesh: vertices of each element, first half-edge of each element (offsets),
//                                  opposite of each half-edge (-1 = BORDER, -2 = COLLAPSED), interface element flags;
//   root                           for each vertex, the original vertex it was copied from;
//   collapsed                      the builder's _collapsed map flattened as [vertex, half-edge, vertex, ...].
// The file is flushed after each step, so the steps already computed survive if a later step aborts.
//
// Exit code: 0 if every check passed, 2 if some check failed; an assertion in the library aborts the program.
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
// A copy of the mesh arrays taken through the IBHM interface, so the checks below do not depend on the code under test.
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

// Half-edge queries on a View, recomputed here (independent from the IBHM class): element and local position of each
// half-edge, next, previous and vertex.
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
// Returns the number of problems found and appends one message per problem to msgs.
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
    // V4: the corners of every vertex must be connected through h ~ O[prev(h)] (union-find over the corners). A
    // collapsed half-edge starts at the same vertex as the corner after it, so it links the two corners of the
    // interface element at that vertex.
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

// ---- V7: coordinates of the copies --------------------------------------------------------------------------------
// Every vertex must have exactly the coordinates of its original vertex (root). Returns the number of problems.
static int checkCoordinates(const IBHM *m, const std::vector<unsigned> &root, std::vector<std::string> &msgs)
{
    int errors = 0;
    const std::vector<float> &points = m->points();
    const unsigned dim = m->numberCoordinatesByVertex();
    for (unsigned w = 0; w < m->numberPoints(); w++)
    {
        for (unsigned i = 0; i < dim; i++)
        {
            if (points[w * dim + i] != points[root[w] * dim + i])
            {
                msgs.push_back("vertex " + std::to_string(w) + " does not have the coordinates of its original vertex " +
                               std::to_string(root[w]));
                errors++;
                break;
            }
        }
    }
    return errors;
}

// Name of an IBHMInterfaceElementBuilder::OperatorType value.
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

// JSON array of unsigned values. With signedOpp, BORDER and COLLAPSED are written as -1 and -2.
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

// Insert the edges (one call per edge, or a single call in batch mode), checking and dumping the mesh after each call.
// Returns the total number of problems found by the checks.
static int run(IBHM *m, const std::vector<double> &coords, const std::vector<unsigned> &edges, const std::string &out,
               bool batch)
{
    IBHMInterfaceElementBuilder b(m);
    std::ofstream js(out);
    js << "{\"points\":[";
    for (size_t i = 0; i < coords.size() / 2; i++) js << (i ? "," : "") << "[" << coords[2 * i] << "," << coords[2 * i + 1] << "]";
    js << "],\"steps\":[";

    // root[w] is the original vertex from which w was copied (an original vertex is its own root).
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
        // An index that is not a half-edge of the mesh is not queried here: it only goes to the builder, which must
        // reject it (I3).
        const bool inMesh = a < m->numberOfHalfEdges();
        unsigned bb = inMesh ? m->heOpposite(a) : BORDER;
        unsigned A = inMesh ? hb.vert(a) : BORDER, B = inMesh ? hb.vert(hb.next(a)) : BORDER;
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
        errors += checkCoordinates(m, root, msgs);
        total += errors;
        printf("step %2d edge he=%u (%u-%u)  A:%-13s B:%-13s new=%u predicted=%u %s\n", step, a, A, B, opA, opB,
               created, predicted, errors ? "INVALID" : "ok");
        for (auto &s : msgs) printf("      ! %s\n", s.c_str());
        dump(after, step, a, A, B, opA, opB, predicted, created, errors, msgs);
    }
    js << "]}\n";
    return total;
}

// Parse the arguments, read the mesh (2D, any element sizes), build the IBHM and run the insertions.
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
