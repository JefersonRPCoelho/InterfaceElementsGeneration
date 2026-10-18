// Unit test for the point storage of the IBHM class: IBHM::reserveSpaceForNodes() and IBHM::addPoint().
//
// Checks that (1) the reservation only changes the capacity of the coordinate's vector, and exactly to the requested
// amount; (2) addPoint() stores the coordinates at the index it returns; (3) a point can be built from the coordinates
// of a vertex of the same mesh (copy of a vertex); (4) the default point is the origin; (5) no reallocation happens
// inside the reserved space; (6) the size always matches numberPoints(); (7) points can still be added beyond the
// reservation; (8) the number of coordinates is not fixed (a 3D mesh with 3 coordinates).
//
// Build and run (from the repository root):
//     c++ -std=c++17 -Icore/TopologyDataStructures doc/report/tools/test_coordinates.cpp \
//         core/TopologyDataStructures/IBHM.cpp -o test_coordinates && ./test_coordinates
// Prints one line per check and "ALL OK"; the exit code is the number of failed checks.
#include <cassert>
#include <cstdio>
#include <vector>
#include "IBHM.h"

int main()
{
    // Two triangles sharing an edge: 4 points in 2D.
    std::vector<float> c = {0, 0, 1, 0, 1, 1, 0, 1};
    std::vector<unsigned> el = {0, 1, 2, 0, 2, 3}, off = {0, 3, 6};
    IBHM m(c, el, off, 2);
    int fails = 0;
    auto check = [&](bool ok, const char *what) { printf("%-62s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; };

    check(m.points().size() == 8, "size == 4 points * 2 after construction");
    m.reserveSpaceForNodes(3);
    check(m.points().size() == 8, "reserveSpaceForNodes keeps the size");
    check(m.points().capacity() == 14, "capacity is exactly (4 + 3) * 2");
    const float *data = m.points().data();

    std::vector<float> p = {5.0f, 7.0f};
    unsigned a = m.addPoint(p);
    double q[2];
    m.coordinatesFromIndex(a, q);
    check(a == 4 && q[0] == 5.0 && q[1] == 7.0, "addPoint stores the coordinates at the returned index");

    // Copy of an existing vertex, built from the mesh's own coordinates.
    std::vector<float> copy(m.points().begin() + 2 * 2, m.points().begin() + 2 * 2 + 2);
    unsigned b = m.addPoint(copy);
    m.coordinatesFromIndex(b, q);
    check(b == 5 && q[0] == 1.0 && q[1] == 1.0, "a copy of vertex 2 gets vertex 2 coordinates");

    unsigned o = m.addPoint();
    m.coordinatesFromIndex(o, q);
    check(o == 6 && q[0] == 0.0 && q[1] == 0.0, "default point is the origin");
    check(m.points().data() == data, "no reallocation inside the reserved space");
    check(m.points().size() == 14 && m.numberPoints() == 7, "size and numberPoints() agree (7 points)");

    unsigned d = m.addPoint(p);
    check(d == 7 && m.points().size() == 16, "addPoint beyond the reservation still works");
    // Points with another number of coordinates (3D): no fixed maximum.
    std::vector<float> c3 = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    std::vector<unsigned> el3 = {0, 1, 2}, off3 = {0, 3};
    IBHM m3(c3, el3, off3, 3);
    m3.reserveSpaceForNodes(1);
    unsigned e = m3.addPoint({1.5f, 2.5f, 3.5f});
    double r[3];
    m3.coordinatesFromIndex(e, r);
    check(e == 3 && r[0] == 1.5 && r[1] == 2.5 && r[2] == 3.5 && m3.points().capacity() == 12,
          "3D mesh: exact reserve and a point with 3 coordinates");
    printf("%s\n", fails ? "FAILED" : "ALL OK");
    return fails;
}
