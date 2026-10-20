// Tests on extreme meshes and inputs: a mesh without interior edges, the smallest mesh with one, every interior edge
// of the base mesh opened, and points with three coordinates. The result is compared with the expected topology
// (InsertionChecks.h).
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "InsertionChecks.h"
#include "MeshValidation.h"
#include "RandomMeshes.h"
#include "TestMeshes.h"

namespace
{
/**
 * @brief Insert the edges in a single call and check the mesh, the builder and the result.
 * @param mesh The mesh, edited by the call.
 * @param halfEdges The half-edges to insert.
 * @return The problems found; an empty vector means that the result is the expected one.
 */
std::vector<std::string> insertAndCheck(IBHM &mesh, const std::vector<unsigned int> &halfEdges)
{
    IBHMInterfaceElementBuilder builder(&mesh);
    const MeshSnapshot input = takeSnapshot(mesh);
    VertexOrigins origins(input.numberOfPoints);

    builder.insertInterfaceElements(halfEdges);
    const MeshSnapshot result = takeSnapshot(mesh);
    origins.update(input, result);

    std::vector<std::string> problems = checkTopology(result);
    for (const std::vector<std::string> &found: {origins.checkCoordinates(mesh),
                                                 checkBuilderState(builder, result,
                                                                   static_cast<unsigned int>(input.elements.size())),
                                                 checkInsertionResult(input, result, openableEdges(input, halfEdges)),
                                                 checkVertexStates(input, result, openableEdges(input, halfEdges),
                                                                   origins.origins())})
    {
        problems.insert(problems.end(), found.begin(), found.end());
    }
    return problems;
}
}


TEST(SpecialMeshes, SingleElementHasNoEdgeToOpen)
{
    // One quadrilateral: its four edges are border edges and are rejected.
    IBHM mesh({0, 0, 1, 0, 1, 1, 0, 1}, {0, 1, 2, 3}, {0, 4}, 2);
    const MeshSnapshot input = takeSnapshot(mesh);
    IBHMInterfaceElementBuilder builder(&mesh);

    EXPECT_EQ(builder.computeNumberOfNewVertices({0, 1, 2, 3}), 0u);
    builder.insertInterfaceElements({0, 1, 2, 3});

    EXPECT_TRUE(takeSnapshot(mesh) == input) << "the mesh must not change";
}

TEST(SpecialMeshes, TwoTrianglesFullyDetached)
{
    // The diagonal of two triangles: both vertices are on the border, so both are duplicated (2 new vertices), the two
    // short sides of the interface element are borders, and the triangles are connected only through it.
    IBHM mesh = twoTrianglesMesh();

    EXPECT_EQ(insertAndCheck(mesh, {2}), std::vector<std::string>{});
    EXPECT_EQ(mesh.numberPoints(), 6u);
    EXPECT_EQ(mesh.heOpposite(7), static_cast<unsigned int>(IBHM::BORDER)) << "short side h1";
    EXPECT_EQ(mesh.heOpposite(9), static_cast<unsigned int>(IBHM::BORDER)) << "short side h3";
}

TEST(SpecialMeshes, EveryInteriorEdgeOfTheBaseMesh)
{
    // Every corner of the base mesh (48) ends with its own vertex, in one call or in one call per edge.
    const std::vector<unsigned int> edges = interiorEdges(baseMesh());

    IBHM batch = baseMesh();
    EXPECT_EQ(insertAndCheck(batch, edges), std::vector<std::string>{});
    EXPECT_EQ(batch.numberPoints(), 48u);

    IBHM incremental = baseMesh();
    IBHMInterfaceElementBuilder builder(&incremental);
    for (const unsigned int edge: edges)
    {
        builder.insertInterfaceElements({edge});
    }
    EXPECT_EQ(cornerPartition(takeSnapshot(incremental), 13), cornerPartition(takeSnapshot(batch), 13));
}

TEST(SpecialMeshes, ThreeCoordinatesPerPoint)
{
    // The base mesh with a third coordinate (a surface in space): the copies get the three coordinates of the original
    // vertex (V7), and the topology does not depend on the number of coordinates.
    IBHM plane = baseMesh();
    std::vector<float> coordinates;
    for (unsigned int point = 0; point < plane.numberPoints(); point++)
    {
        const float x = plane.points()[2 * point];
        const float y = plane.points()[2 * point + 1];
        coordinates.insert(coordinates.end(), {x, y, 0.5f * x - 0.25f * y});
    }
    std::vector<unsigned int> offset;
    for (unsigned int element = 0; element <= plane.numberOfElements(); element++)
    {
        offset.push_back(plane.firstElementHE(element));
    }
    IBHM surface(coordinates, plane.elementsList(), offset, 3);

    EXPECT_EQ(insertAndCheck(surface, baseMeshEdges()), std::vector<std::string>{});
    EXPECT_EQ(surface.points().size(), 3u * (19 + 19));
}
