// Tests of IBHMInterfaceElementBuilder::computeNumberOfNewVertices().
//
// The function must return exactly the number of vertices that insertInterfaceElements() creates for the same edges:
// the edges that the insertion skips must not be counted, and the rule of each vertex must be applied once per vertex.
// Each test compares the count with the expected value and, when the insertion supports the case, with the number of
// vertices really created. The invalid indices (I3) are tested in test_invalid_input.cpp.
//
// Half-edges of the base mesh used here: 31 = K->Q, 34 = K->L, 0 = A->B (border edge), 48 = first half-edge of the
// first interface element created (the mesh has 48 half-edges).
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "TestMeshes.h"

namespace
{
/**
 * @brief The base mesh and a builder for it.
 */
class VertexCount : public testing::Test
{
protected:
    IBHM mesh = baseMesh();
    IBHMInterfaceElementBuilder builder{&mesh};

    /**
     * @brief Count the new vertices for some edges, insert them, and compare both with the expected value.
     * @param edges The edges, one half-edge per edge.
     * @param expected The expected number of new vertices.
     */
    void countAndInsert(const std::vector<unsigned int> &edges, const unsigned int expected)
    {
        const unsigned int predicted = builder.computeNumberOfNewVertices(edges);
        const unsigned int before = mesh.numberPoints();
        builder.insertInterfaceElements(edges);
        const unsigned int created = mesh.numberPoints() - before;

        EXPECT_EQ(predicted, expected) << "count";
        EXPECT_EQ(created, expected) << "vertices created by the insertion";
    }
};
}


TEST_F(VertexCount, BaseMeshExampleInASingleCall)
{
    countAndInsert(baseMeshEdges(), 19);
}

TEST_F(VertexCount, SameEdgeThreeTimesIsOpenedOnce)
{
    // Twice by K->L and once by L->K. K and L are internal, so the edge only collapses them: 0 new vertices. The old
    // count gave 4.
    countAndInsert({34, 34, mesh.heOpposite(34)}, 0);
}

TEST_F(VertexCount, BorderEdgeIsNotCounted)
{
    // The old count gave 2.
    countAndInsert({0}, 0);
}

TEST_F(VertexCount, BorderEdgeRepeatedEdgeAndTwoEdgesAtTheSameVertex)
{
    // K internal with 2 edges -> 1, Q on the border with 1 edge -> 1, L internal with 1 edge -> 0. Total 2.
    countAndInsert({0, 31, 34, 31}, 2);
}

TEST_F(VertexCount, VertexAlreadyInAnInterfaceElement)
{
    // K-L: K and L are internal, so both are only collapsed (0 new vertices).
    countAndInsert({34}, 0);

    // Then an edge of the interface element just created (half-edge 48, which starts at K right after the collapsed
    // edge): it is skipped, 0 new vertices. The old count never returned in this case (infinite loop in isBorder).
    countAndInsert({48}, 0);

    // Then K-Q: K is already in an interface element -> 1, Q on the border -> 1. Total 2.
    countAndInsert({31}, 2);
}

TEST(VertexCountFan, BorderVertexWithTwoEdgesInTheSameCall)
{
    // Q (border) with 2 edges -> 2, R and P (border) with 1 edge each -> 1 + 1. Total 4. The old count gave 5. The
    // insertion of the second edge at Q is an Open Hole on a border vertex (test_open_hole_border.cpp).
    IBHM mesh = fanMesh();
    IBHMInterfaceElementBuilder builder(&mesh);

    EXPECT_EQ(builder.computeNumberOfNewVertices({4, 8}), 4u);
    builder.insertInterfaceElements({4, 8});
    EXPECT_EQ(mesh.numberPoints(), 8u + 4u);
}
