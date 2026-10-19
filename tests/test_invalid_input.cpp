// Tests of the input that insertInterfaceElements() must reject: indices that are not half-edges of the mesh (I3),
// border edges and edges of interface elements. A rejected edge must not change the mesh, and
// computeNumberOfNewVertices() must count it as 0.
//
// Half-edges of the base mesh used here: 0 = A->B (border edge), 31 = K->Q, 34 = K->L. The mesh has 48 half-edges.
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "MeshValidation.h"
#include "TestMeshes.h"

/**
 * @brief The base mesh and a builder for it.
 */
class InvalidInput : public testing::Test
{
protected:
    IBHM mesh = baseMesh();
    IBHMInterfaceElementBuilder builder{&mesh};

    /**
     * @brief Check that the builder rejects all the edges: count 0 and no change in the mesh.
     * @param edges The edges, one half-edge per edge.
     */
    void expectRejected(const std::vector<unsigned int> &edges)
    {
        const MeshSnapshot before = takeSnapshot(mesh);

        EXPECT_EQ(builder.computeNumberOfNewVertices(edges), 0u);
        builder.insertInterfaceElements(edges);

        EXPECT_TRUE(takeSnapshot(mesh) == before) << "the mesh must not change";
    }
};


TEST_F(InvalidInput, IndexOutsideTheElementList)
{
    // The old code stopped at an assertion in heOpposite().
    expectRejected({999});
}

TEST_F(InvalidInput, IndexInTheReservedSpace)
{
    // A call with only a border edge reserves the positions 48 to 51 and leaves them unused.
    builder.insertInterfaceElements({0});
    ASSERT_EQ(mesh.numberOfHalfEdges(), 48u);
    ASSERT_EQ(mesh.numberReservedElements(), 52u);

    // The old code took 48 as the half-edge of an edge whose opposite is 0 (the initial value of the reserved
    // positions), counted 2 new vertices, and stopped at an assertion in openHole().
    expectRejected({48});
}

TEST_F(InvalidInput, IndexOfAnInterfaceElementCreatedInTheSameCall)
{
    // 999, K-Q and 50. The index 50 is a half-edge of the interface element created by this call for K-Q, but the limit
    // is the number of half-edges before the call (48), so it is rejected as well. K is internal (collapsed, 0 new
    // vertices) and Q is on the border (1).
    EXPECT_EQ(builder.computeNumberOfNewVertices({999, 31, 50}), 1u);
    builder.insertInterfaceElements({999, 31, 50});

    EXPECT_EQ(mesh.numberPoints(), 20u);
    EXPECT_EQ(mesh.numberOfElements(), 14u);
    EXPECT_EQ(checkTopology(takeSnapshot(mesh)), std::vector<std::string>{});
}

TEST_F(InvalidInput, BorderEdge)
{
    expectRejected({0});
}

TEST_F(InvalidInput, EdgeOfAnInterfaceElement)
{
    builder.insertInterfaceElements({34});

    // Half-edge 48: the first half-edge of the interface element just created.
    expectRejected({48});
}

TEST_F(InvalidInput, EdgeOpenedInAPreviousCall)
{
    builder.insertInterfaceElements({34});

    // After the insertion, the opposite of 34 is a half-edge of the interface element, by either half-edge.
    expectRejected({34});
    expectRejected({mesh.heOpposite(34)});
}

TEST_F(InvalidInput, EdgeRepeatedInTheSameCallIsOpenedOnce)
{
    builder.insertInterfaceElements({34, 34, mesh.heOpposite(34)});

    EXPECT_EQ(mesh.numberOfElements(), 14u);
    EXPECT_EQ(checkTopology(takeSnapshot(mesh)), std::vector<std::string>{});
}
