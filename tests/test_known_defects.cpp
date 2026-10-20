// Tests of the known defects that are still open (section 7.4 of the report and CLAUDE.md). Each test describes the
// correct behavior and is disabled (prefix DISABLED_) while the defect is open, so it shows up as disabled in every run
// and needs no change to the expectations once the defect is fixed: remove the prefix.
//
// To run them anyway: InterfaceElementsTests --gtest_also_run_disabled_tests --gtest_filter='KnownDefects*'
//
// Fixed defects leave this file and get their own tests: I1 (Open Hole on a border vertex) is in
// test_open_hole_border.cpp.
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "TestMeshes.h"


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
