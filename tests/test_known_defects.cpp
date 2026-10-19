// Tests of the known defects that are still open (section 7.4 of the report and CLAUDE.md). Each test describes the
// correct behavior and is disabled (prefix DISABLED_) while the defect is open, so it shows up as disabled in every run
// and needs no change to the expectations once the defect is fixed: remove the prefix.
//
// To run them anyway: InterfaceElementsTests --gtest_also_run_disabled_tests --gtest_filter='KnownDefects*'
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "MeshValidation.h"
#include "TestMeshes.h"


// I1: openHole() on a border vertex that is already open. The fan mesh has the border vertex Q = 0 with two interior
// edges, Q-R (half-edge 4) and Q-P (half-edge 8). After the first edge, Q is a border vertex in an interface element
// without a collapsed edge, so the second edge applies Open Hole, which does not stop at the border: assertion in
// debug, write outside the vector in release. Expected: Q ends with 3 copies (2 new vertices), R and P with 2 copies
// each (1 new vertex each), so 4 new vertices, in any order and in a single call.
TEST(KnownDefects, DISABLED_I1BorderVertexWithTwoEdgesOneCallPerEdge)
{
    IBHM mesh = fanMesh();
    IBHMInterfaceElementBuilder builder(&mesh);
    VertexOrigins origins(mesh.numberPoints());
    for (const unsigned int edge: {4u, 8u})
    {
        const MeshSnapshot before = takeSnapshot(mesh);
        builder.insertInterfaceElements({edge});
        origins.update(before, takeSnapshot(mesh));
    }

    EXPECT_EQ(mesh.numberPoints(), 8u + 4u);
    EXPECT_EQ(checkTopology(takeSnapshot(mesh)), std::vector<std::string>{});
    EXPECT_EQ(origins.checkCoordinates(mesh), std::vector<std::string>{});
}

TEST(KnownDefects, DISABLED_I1BorderVertexWithTwoEdgesInOneCall)
{
    IBHM mesh = fanMesh();
    IBHMInterfaceElementBuilder builder(&mesh);

    builder.insertInterfaceElements({4, 8});

    EXPECT_EQ(mesh.numberPoints(), 8u + 4u);
    EXPECT_EQ(checkTopology(takeSnapshot(mesh)), std::vector<std::string>{});
}

// I2 (latent): isBorder() skips the collapsed edge with a next() that can jump over the half-edge where the rotation
// should stop, so it never returns when it starts right after a collapsed edge. The builder no longer reaches this case
// (the edges of interface elements are rejected before the count), so it is tested directly. On the base mesh, after
// opening K-L (half-edge 34), the half-edge 48 starts at K right after the collapsed edge; K is internal.
// While the defect is open, this test does not end: CTest stops it by the timeout.
TEST(KnownDefects, DISABLED_I2IsBorderStartingAfterACollapsedEdge)
{
    IBHM mesh = baseMesh();
    IBHMInterfaceElementBuilder builder(&mesh);
    builder.insertInterfaceElements({34});

    EXPECT_FALSE(mesh.isBorder(48));
}
