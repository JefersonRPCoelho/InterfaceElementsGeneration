// Unit test for IBHMInterfaceElementBuilder::computeNumberOfNewVertices().
//
// The function must return exactly the number of vertices that insertInterfaceElements() creates for the same edges:
// edges that the insertion skips (indices that are not half-edges of the mesh, border edges, edges of interface
// elements, repeated edges) must not be counted, and the rule of each vertex must be applied once per vertex. For each
// case, the test compares the count with the expected value and, when the insertion supports the case, with the number
// of vertices really created.
//
// Build and run (from the repository root):
//     c++ -std=c++17 -Icore/TopologyDataStructures -Icore/InterfaceElementsBuilder \
//         doc/report/tools/test_vertex_count.cpp core/TopologyDataStructures/IBHM.cpp \
//         core/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.cpp -o test_vertex_count && ./test_vertex_count
// Prints one line per check and "ALL OK"; the exit code is the number of failed checks.
#include <cstdio>
#include <vector>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"

namespace
{
int fails = 0;

void check(const bool ok, const char *what)
{
    printf("%-74s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}

// The mesh of the slides (malha2Hyb.txt): quadrilaterals and triangles. Vertex names used in the comments:
//   y=0: A B C D      y=1: E F G H I      y=2: J K L M N      y=3: P Q R S      and T = (4, 3).
IBHM slideMesh()
{
    const std::vector<float> c = {0, 0, 1, 0, 2, 0, 3, 0,
                                  0, 1, 1, 1, 2, 1, 3, 1, 4, 1,
                                  0, 2, 1, 2, 2, 2, 3, 2, 4, 2,
                                  0, 3, 1, 3, 2, 3, 3, 3,
                                  4, 3};
    const std::vector<unsigned int> el = {0, 1, 5, 4, 1, 2, 6, 5, 2, 3, 7, 6, 4, 5, 10, 9, 5, 6, 11, 10, 6, 7, 12, 11,
                                          7, 8, 12, 8, 13, 12, 9, 10, 15, 14, 10, 11, 16, 15, 11, 12, 17, 16,
                                          12, 13, 18, 12, 18, 17};
    const std::vector<unsigned int> off = {0, 4, 8, 12, 16, 20, 24, 27, 30, 34, 38, 42, 45, 48};
    return IBHM(c, el, off, 2);
}

// Three quadrilaterals around the border vertex Q = 0 (a half disk), so Q has two interior edges: Q-R and Q-P.
IBHM fanMesh()
{
    const std::vector<float> c = {0, 0, 1, 0, 0.5f, 0.866f, -0.5f, 0.866f, -1, 0, 1.2f, 0.7f, 0, 1.4f, -1.2f, 0.7f};
    const std::vector<unsigned int> el = {0, 1, 5, 2, 0, 2, 6, 3, 0, 3, 7, 4};
    const std::vector<unsigned int> off = {0, 4, 8, 12};
    return IBHM(c, el, off, 2);
}

// Count the new vertices for 'edges', insert them, and compare the count with 'expected' and with the vertices created.
void countAndInsert(IBHM &mesh, IBHMInterfaceElementBuilder &builder, const std::vector<unsigned int> &edges,
                    const unsigned int expected, const char *what)
{
    const unsigned int predicted = builder.computeNumberOfNewVertices(edges);
    const unsigned int before = mesh.numberPoints();
    builder.insertInterfaceElements(edges);
    const unsigned int created = mesh.numberPoints() - before;
    char text[256];
    snprintf(text, sizeof(text), "%s: predicted %u, expected %u, created %u", what, predicted, expected, created);
    check(predicted == expected && created == expected, text);
}
}



int main()
{
    // Half-edges of malha2Hyb.txt: 31 = K->Q, 34 = K->L, 0 = A->B (border edge), 48 = first half-edge of the first
    // interface element created (the mesh has 48 half-edges).

    // 1. The 12 edges of the slides in a single call: 19 new vertices.
    {
        IBHM mesh = slideMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        countAndInsert(mesh, builder, {31, 34, 11, 26, 47, 38, 23, 19, 30, 42, 29, 45}, 19, "slide example, one call");
    }

    // 2. The same edge three times (twice by K->L and once by L->K): it is opened once. K and L are internal, so the
    // first edge only collapses them: 0 new vertices. The old count gave 4.
    {
        IBHM mesh = slideMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        countAndInsert(mesh, builder, {34, 34, mesh.heOpposite(34)}, 0, "same edge three times");
    }

    // 3. A border edge only: it is skipped, 0 new vertices. The old count gave 2.
    {
        IBHM mesh = slideMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        countAndInsert(mesh, builder, {0}, 0, "border edge");
    }

    // 4. A border edge, K-Q twice and K-L: K internal with 2 edges -> 1, Q on the border with 1 edge -> 1, L internal
    // with 1 edge -> 0. Total 2.
    {
        IBHM mesh = slideMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        countAndInsert(mesh, builder, {0, 31, 34, 31}, 2, "border edge, repeated edge and two edges at K");
    }

    // 5. First K-L: K and L are internal, so both are only collapsed (0 new vertices).
    // 6. Then an edge of the interface element just created (half-edge 48, which starts at K right after the collapsed
    // edge): it is skipped, 0 new vertices. The old count never returned in this case (infinite loop in isBorder).
    // 7. Then K-Q: K is already in an interface element -> 1, Q on the border -> 1. Total 2.
    {
        IBHM mesh = slideMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        countAndInsert(mesh, builder, {34}, 0, "K-L, both internal");
        countAndInsert(mesh, builder, {48}, 0, "then an edge of that interface element");
        countAndInsert(mesh, builder, {31}, 2, "then K-Q, K already in an interface element");
    }

    // 8. A border vertex with two edges in the same call (count only: the insertion does not support this case yet).
    // Q (border) with 2 edges -> 2, R and P (border) with 1 edge each -> 1 + 1. Total 4. The old count gave 5.
    {
        IBHM mesh = fanMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        const unsigned int predicted = builder.computeNumberOfNewVertices({4, 8});
        char text[256];
        snprintf(text, sizeof(text), "border vertex with two edges (count only): predicted %u, expected 4", predicted);
        check(predicted == 4, text);
    }

    // 9. An index outside the element's list: it is skipped, 0 new vertices. The old code stopped at an assertion in
    // heOpposite().
    {
        IBHM mesh = slideMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        countAndInsert(mesh, builder, {999}, 0, "index outside the element's list");
    }

    // 10. An index in the reserved space. A call with only a border edge reserves the positions 48 to 51 and leaves
    // them unused. The reserved opposites must start as BORDER, and the index 48 must be skipped in the next call
    // (0 new vertices, no new element). The old code took 48 as the half-edge of an edge whose opposite is 0 (the
    // initial value of the reserved positions), counted 2 new vertices, and stopped at an assertion in openHole().
    {
        IBHM mesh = slideMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        builder.insertInterfaceElements({0});
        check(mesh.numberOfHalfEdges() == 48 && mesh.numberReservedElements() == 52,
              "border edge alone: positions 48 to 51 reserved, but not part of the mesh");
        check(mesh.heOpposite(48) == static_cast<unsigned int>(IBHM::BORDER),
              "the reserved opposites start as BORDER");
        countAndInsert(mesh, builder, {48}, 0, "index in the reserved space");
        check(mesh.numberOfElements() == 13 && mesh.numberOfHalfEdges() == 48, "no element inserted");
    }

    // 11. Invalid indices mixed with a valid edge in the same call: 999, K-Q and 50. K is internal (collapsed, 0) and Q
    // is on the border (1): total 1. The index 50 is a half-edge of the interface element created by this call for
    // K-Q, but the limit is the number of half-edges before the call (48), so it is skipped as well.
    {
        IBHM mesh = slideMesh();
        IBHMInterfaceElementBuilder builder(&mesh);
        countAndInsert(mesh, builder, {999, 31, 50}, 1, "invalid indices mixed with a valid edge");
        check(mesh.numberOfElements() == 14 && mesh.numberOfHalfEdges() == 52, "only one element inserted");
    }

    printf("%s\n", fails ? "FAILED" : "ALL OK");
    return fails;
}
