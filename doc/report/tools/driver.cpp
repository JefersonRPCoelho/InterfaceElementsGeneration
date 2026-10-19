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
// The checks V1-V5 and V7 come from tests/support/MeshValidation.{h,cpp}, shared with the automated tests, and the
// builder's private state (operator chosen, interface element flags, collapsed edges) is read through
// tests/support/BuilderInspector.h, a friend of the builder.
//
// Build: target "driver" of the CMake project (tests/CMakeLists.txt), or from the repository root:
//     c++ -std=c++17 -Icore/TopologyDataStructures -Icore/InterfaceElementsBuilder -Itests/support \
//         doc/report/tools/driver.cpp tests/support/MeshValidation.cpp core/TopologyDataStructures/IBHM.cpp \
//         core/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.cpp -o driver
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "BuilderInspector.h"
#include "MeshValidation.h"

static const unsigned BORDER = 0xFFFFFFFFu, COLLAPSED = 0xFFFFFFFEu;

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

    // The original vertex from which each vertex was copied (an original vertex is its own origin), for V7.
    MeshSnapshot v0 = takeSnapshot(*m);
    VertexOrigins origins(v0.numberOfPoints);

    auto dump = [&](const MeshSnapshot &v, int step, unsigned edge, unsigned A, unsigned B, const char *opA,
                    const char *opB, unsigned predicted, unsigned created, int errors,
                    const std::vector<std::string> &msgs) {
        if (step > 0) js << ",";
        js << "{\"step\":" << step << ",\"edge\":" << (int)edge << ",\"A\":" << (int)A << ",\"B\":" << (int)B
           << ",\"opA\":\"" << opA << "\",\"opB\":\"" << opB << "\",\"predicted\":" << predicted
           << ",\"created\":" << created << ",\"errors\":" << errors << ",\"nPoints\":" << v.numberOfPoints;
        js << ",\"elements\":[";
        for (size_t e = 0; e < v.elements.size(); e++) js << (e ? "," : "") << jsonArr(v.elements[e]);
        js << "],\"first\":" << jsonArr(v.firstHalfEdge) << ",\"opp\":" << jsonArr(v.opposite, true);
        js << ",\"isIE\":[";
        for (unsigned e = 0; e < v.elements.size(); e++)
            js << (e ? "," : "") << (BuilderInspector::isInterfaceElement(b, e) ? 1 : 0);
        js << "],\"root\":" << jsonArr(origins.origins());
        std::vector<unsigned> col;
        for (auto &[k, h] : BuilderInspector::collapsed(b)) { col.push_back(k); col.push_back(h); }
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
        MeshSnapshot before = takeSnapshot(*m);
        unsigned a = g[0];
        // An index that is not a half-edge of the mesh is not queried here: it only goes to the builder, which must
        // reject it (I3).
        const bool inMesh = a < m->numberOfHalfEdges();
        unsigned bb = inMesh ? m->heOpposite(a) : BORDER;
        unsigned A = inMesh ? before.vertex(a) : BORDER, B = inMesh ? before.vertex(before.next(a)) : BORDER;
        const char *opA = "", *opB = "";
        // Log the operators only for edges the builder accepts (it rejects border edges and interface element edges).
        if (g.size() == 1 && bb != BORDER && bb != COLLAPSED &&
            !BuilderInspector::isInterfaceElement(b, before.elementOf[a]) &&
            !BuilderInspector::isInterfaceElement(b, before.elementOf[bb]))
        {
            opA = BuilderInspector::operatorName(BuilderInspector::retrieveOperator(b, a));
            opB = BuilderInspector::operatorName(BuilderInspector::retrieveOperator(b, bb));
        }
        unsigned predicted = b.computeNumberOfNewVertices(g);
        b.insertInterfaceElements(g);
        MeshSnapshot after = takeSnapshot(*m);
        unsigned created = after.numberOfPoints - before.numberOfPoints;
        origins.update(before, after);
        std::vector<std::string> msgs = checkTopology(after);
        for (auto &s : origins.checkCoordinates(*m)) msgs.push_back(s);
        int errors = static_cast<int>(msgs.size());
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
