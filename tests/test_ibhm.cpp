// Tests of the IBHM class: point storage (reserveSpaceForNodes, addPoint, duplicatePoint), the half-edge space
// (numberOfHalfEdges, reserveSpaceForElements, and the half-edge queries reject the sentinels), and the border test
// (isBorder), also on meshes with interface elements, which are built with IBHMInterfaceElementBuilder.
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
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

/**
 * @brief Check isBorder() for every half-edge of a mesh, including the half-edges of interface elements.
 *
 * The expected value is computed on the snapshot, without rotations: a vertex is on the border if some half-edge that
 * starts at it has no opposite (BORDER).
 * @param mesh The mesh.
 */
void expectIsBorderForEveryHalfEdge(const IBHM &mesh)
{
    const MeshSnapshot snapshot = takeSnapshot(mesh);
    std::vector<bool> border(snapshot.numberOfPoints, false);
    for (unsigned int he = 0; he < snapshot.opposite.size(); he++)
    {
        if (snapshot.opposite[he] == static_cast<unsigned int>(IBHM::BORDER))
        {
            border[snapshot.vertex(he)] = true;
        }
    }

    for (unsigned int he = 0; he < snapshot.opposite.size(); he++)
    {
        EXPECT_EQ(mesh.isBorder(he), static_cast<bool>(border[snapshot.vertex(he)]))
            << "half-edge " << he << ", vertex " << snapshot.vertex(he);
    }
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

TEST(IBHMHalfEdges, ReserveSpaceForElementsGrowsExactly)
{
    // I11: resize() alone would grow the capacity geometrically (to 96 positions in libc++ and libstdc++).
    IBHM mesh = baseMesh();

    mesh.reserveSpaceForElements(4, 1);

    EXPECT_EQ(mesh.numberReservedElements(), 52u);
    EXPECT_EQ(mesh.elementsList().size(), 52u);
    EXPECT_EQ(mesh.elementsList().capacity(), 52u);
}

TEST(IBHMHalfEdges, ReserveSpaceForElementsOnlyAddsWhatIsMissing)
{
    // The positions already reserved and not used count toward the request (I11).
    IBHM mesh = baseMesh();

    mesh.reserveSpaceForElements(4, 1);
    mesh.reserveSpaceForElements(4, 1);
    EXPECT_EQ(mesh.numberReservedElements(), 52u) << "4 positions were already free";

    mesh.reserveSpaceForElements(8, 2);
    EXPECT_EQ(mesh.numberReservedElements(), 56u) << "only 4 positions were missing";
    EXPECT_EQ(mesh.elementsList().capacity(), 56u);
    EXPECT_EQ(mesh.numberOfHalfEdges(), 48u);
}

TEST(IBHMHalfEdges, ReserveCapacityForElementsKeepsTheSizeAndAvoidsReallocations)
{
    IBHM mesh = baseMesh();

    mesh.reserveCapacityForElements(400, 100);
    EXPECT_EQ(mesh.numberReservedElements(), 48u) << "no position is reserved";
    EXPECT_EQ(mesh.numberOfHalfEdges(), 48u);
    EXPECT_EQ(mesh.elementsList().capacity(), 448u);

    // The positions are then reserved inside the capacity: no reallocation, and the capacity does not change.
    const unsigned int *data = mesh.elementsList().data();
    mesh.reserveSpaceForElements(4, 1);
    EXPECT_EQ(mesh.numberReservedElements(), 52u);
    EXPECT_EQ(mesh.elementsList().data(), data);
    EXPECT_EQ(mesh.elementsList().capacity(), 448u);
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


// ---- Border test ----------------------------------------------------------------------------------------------------

TEST(IBHMIsBorder, StartingAtACollapsedEdgeAndRightAfterIt)
{
    // I2: on the base mesh, after opening K-L (half-edge 34), the interface element is [48, 49, 50, 51]: 51 is the
    // collapsed edge at K, and 48 starts at K right after it. K is internal. The former isBorder() never returned when
    // it started at 48: the step over the collapsed edge jumped over the comparison with the starting half-edge.
    IBHM mesh = baseMesh();
    IBHMInterfaceElementBuilder builder(&mesh);
    builder.insertInterfaceElements({34});
    ASSERT_EQ(mesh.heOpposite(51), static_cast<unsigned int>(IBHM::COLLAPSED));

    EXPECT_FALSE(mesh.isBorder(51));
    EXPECT_FALSE(mesh.isBorder(48));
}

TEST(IBHMIsBorder, EveryHalfEdgeOfTheBaseMeshExample)
{
    // Every half-edge, in the input mesh and after each of the 12 insertions of the example: collapsed edges (K, L, M,
    // G and F along the way), split vertices, holes (their borders count as borders) and the outer border.
    IBHM mesh = baseMesh();
    IBHMInterfaceElementBuilder builder(&mesh);
    expectIsBorderForEveryHalfEdge(mesh);

    for (const unsigned int edge: baseMeshEdges())
    {
        SCOPED_TRACE("after the edge of half-edge " + std::to_string(edge));
        builder.insertInterfaceElements({edge});
        expectIsBorderForEveryHalfEdge(mesh);
    }
}
