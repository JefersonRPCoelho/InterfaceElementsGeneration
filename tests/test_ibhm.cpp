// Tests of the IBHM class alone: point storage (reserveSpaceForNodes, addPoint, duplicatePoint) and the half-edge
// space (numberOfHalfEdges, reserveSpaceForElements, and the half-edge queries reject the sentinels).
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "MeshValidation.h"
#include "TestMeshes.h"

namespace
{
/**
 * @brief Get the coordinates of a point.
 * @param mesh The mesh.
 * @param point The point index.
 * @return The coordinates.
 */
std::vector<double> coordinatesOf(const IBHM &mesh, const unsigned int point)
{
    std::vector<double> coordinates(mesh.numberCoordinatesByVertex());
    mesh.coordinatesFromIndex(point, coordinates.data());
    return coordinates;
}
}


// ---- Points ---------------------------------------------------------------------------------------------------------

TEST(IBHMPoints, ReserveChangesOnlyTheCapacityAndExactly)
{
    IBHM mesh = twoTrianglesMesh();
    ASSERT_EQ(mesh.points().size(), 8u);

    mesh.reserveSpaceForNodes(3);

    EXPECT_EQ(mesh.points().size(), 8u);
    EXPECT_EQ(mesh.numberPoints(), 4u);
    EXPECT_EQ(mesh.points().capacity(), 14u) << "the capacity must be exactly (4 + 3) points * 2 coordinates";
}

TEST(IBHMPoints, AddPointStoresTheCoordinatesAtTheReturnedIndex)
{
    IBHM mesh = twoTrianglesMesh();

    const unsigned int point = mesh.addPoint({5.0f, 7.0f});

    EXPECT_EQ(point, 4u);
    EXPECT_EQ(coordinatesOf(mesh, point), (std::vector<double>{5.0, 7.0}));
}

TEST(IBHMPoints, AddPointAcceptsTheCoordinatesOfAVertexOfTheSameMesh)
{
    IBHM mesh = twoTrianglesMesh();
    const std::vector<float> copy(mesh.points().begin() + 4, mesh.points().begin() + 6);

    const unsigned int point = mesh.addPoint(copy);

    EXPECT_EQ(coordinatesOf(mesh, point), coordinatesOf(mesh, 2));
}

TEST(IBHMPoints, AddPointWithoutCoordinatesIsTheOrigin)
{
    IBHM mesh = twoTrianglesMesh();

    const unsigned int point = mesh.addPoint();

    EXPECT_EQ(coordinatesOf(mesh, point), (std::vector<double>{0.0, 0.0}));
}

TEST(IBHMPoints, NoReallocationInsideTheReservedSpace)
{
    IBHM mesh = twoTrianglesMesh();
    mesh.reserveSpaceForNodes(3);
    const float *data = mesh.points().data();

    mesh.addPoint({5.0f, 7.0f});
    mesh.duplicatePoint(2);
    mesh.addPoint();

    EXPECT_EQ(mesh.points().data(), data);
    EXPECT_EQ(mesh.numberPoints(), 7u);
    EXPECT_EQ(mesh.points().size(), 14u) << "the size must always match numberPoints()";
}

TEST(IBHMPoints, AddPointBeyondTheReservation)
{
    IBHM mesh = twoTrianglesMesh();
    mesh.reserveSpaceForNodes(1);
    mesh.addPoint();

    const unsigned int point = mesh.addPoint({5.0f, 7.0f});

    EXPECT_EQ(point, 5u);
    EXPECT_EQ(mesh.points().size(), 12u);
    EXPECT_EQ(coordinatesOf(mesh, point), (std::vector<double>{5.0, 7.0}));
}

TEST(IBHMPoints, DuplicatePointCopiesTheVertex)
{
    IBHM mesh = twoTrianglesMesh();
    mesh.reserveSpaceForNodes(1);
    const float *data = mesh.points().data();

    const unsigned int copy = mesh.duplicatePoint(2);

    EXPECT_EQ(copy, 4u);
    EXPECT_EQ(coordinatesOf(mesh, copy), coordinatesOf(mesh, 2));
    EXPECT_EQ(mesh.points().data(), data);
}

TEST(IBHMPoints, DuplicatePointIsRightWhenTheVectorIsReallocated)
{
    IBHM mesh = twoTrianglesMesh();
    while (mesh.points().size() < mesh.points().capacity())
    {
        mesh.addPoint();
    }
    const float *full = mesh.points().data();

    // The source is read by index, so a reallocation during the copy must not change the copied values.
    const unsigned int copy = mesh.duplicatePoint(2);

    EXPECT_NE(mesh.points().data(), full) << "the test needs a reallocation during the copy";
    EXPECT_EQ(coordinatesOf(mesh, copy), (std::vector<double>{1.0, 1.0}));
}

TEST(IBHMPoints, ThreeCoordinatesPerPoint)
{
    // The number of coordinates is not fixed: a 3D mesh with one triangle.
    IBHM mesh({0, 0, 0, 1, 0, 0, 0, 1, 0}, {0, 1, 2}, {0, 3}, 3);
    mesh.reserveSpaceForNodes(1);

    const unsigned int point = mesh.addPoint({1.5f, 2.5f, 3.5f});
    EXPECT_EQ(point, 3u);
    EXPECT_EQ(coordinatesOf(mesh, point), (std::vector<double>{1.5, 2.5, 3.5}));
    EXPECT_EQ(mesh.points().capacity(), 12u);

    const unsigned int copy = mesh.duplicatePoint(1);
    EXPECT_EQ(copy, 4u);
    EXPECT_EQ(coordinatesOf(mesh, copy), (std::vector<double>{1.0, 0.0, 0.0}));
}

TEST(IBHMPointsDeathTest, AddPointWithTheWrongNumberOfCoordinates)
{
    IBHM mesh = twoTrianglesMesh();

    EXPECT_DEATH(mesh.addPoint({1.0f}), "coordinates.size\\(\\) == _numberCoordinatesPerVertex");
}


// ---- Half-edges -----------------------------------------------------------------------------------------------------

TEST(IBHMHalfEdges, TheBaseMeshIsValid)
{
    const IBHM mesh = baseMesh();

    EXPECT_EQ(mesh.numberPoints(), 19u);
    EXPECT_EQ(mesh.numberOfElements(), 13u);
    EXPECT_EQ(mesh.numberOfHalfEdges(), 48u);
    EXPECT_EQ(checkTopology(takeSnapshot(mesh)), std::vector<std::string>{});
}

TEST(IBHMHalfEdges, NumberOfHalfEdgesDoesNotCountTheReservedSpace)
{
    IBHM mesh = baseMesh();

    mesh.reserveSpaceForElements(8, 2);
    EXPECT_EQ(mesh.numberOfHalfEdges(), 48u);
    EXPECT_EQ(mesh.numberReservedElements(), 56u);

    mesh.commitElement(4);
    EXPECT_EQ(mesh.numberOfHalfEdges(), 52u);
    EXPECT_EQ(mesh.numberOfElements(), 14u);
}

TEST(IBHMHalfEdges, ReservedOppositesStartAsBorder)
{
    IBHM mesh = baseMesh();

    mesh.reserveSpaceForElements(4, 1);

    for (unsigned int he = 48; he < 52; he++)
    {
        EXPECT_EQ(mesh.heOpposite(he), static_cast<unsigned int>(IBHM::BORDER)) << "reserved half-edge " << he;
    }
}

TEST(IBHMHalfEdgesDeathTest, QueriesRejectSentinelsAndReservedPositions)
{
    // next(), previous() and element() only accept half-edges of valid elements (I6): the sentinels and the positions
    // reserved and not committed yet must stop at the assertion instead of reaching the binary search.
    IBHM mesh = baseMesh();
    mesh.reserveSpaceForElements(4, 1);

    const std::vector<unsigned int> invalid = {static_cast<unsigned int>(IBHM::BORDER),
                                               static_cast<unsigned int>(IBHM::COLLAPSED), 48};
    for (const unsigned int he: invalid)
    {
        SCOPED_TRACE("half-edge " + std::to_string(he));
        EXPECT_DEATH((void) mesh.heNext(he), "halfEdge < numberOfHalfEdges");
        EXPECT_DEATH((void) mesh.hePrevious(he), "halfEdge < numberOfHalfEdges");
        EXPECT_DEATH((void) mesh.heElement(he), "halfEdge < numberOfHalfEdges");
    }
}
