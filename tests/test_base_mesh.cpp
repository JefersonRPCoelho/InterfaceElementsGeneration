// Tests on the base mesh (the example of the slides and of the report): the operator chosen for each vertex, the
// vertices created by each insertion, the validity of the topology after every insertion (V1 to V7), and the
// independence of the result from the insertion order and from inserting in one or several calls.
#include <cstddef>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "BuilderInspector.h"
#include "InsertionChecks.h"
#include "MeshValidation.h"
#include "TestMeshes.h"

namespace
{
using Op = IBHMInterfaceElementBuilder::OperatorType;

/**
 * Number of elements and points of the base mesh.
 */
constexpr unsigned int INPUT_ELEMENTS = 13;
constexpr unsigned int INPUT_POINTS = 19;

/**
 * @brief One insertion of the example: the edge, the operators expected at its two vertices and the new vertices.
 */
struct ExpectedStep
{
    unsigned int edge;
    Op operatorA;
    Op operatorB;
    unsigned int newVertices;
};

/**
 * The 12 insertions of the example, one edge per call, in the order of the slides. A is the vertex where the given
 * half-edge starts, and B is the other vertex.
 */
const std::vector<ExpectedStep> EXAMPLE = {
    {31, Op::CANONICAL, Op::CANONICAL, 1},
    {34, Op::SPLIT_ELEMENT, Op::CANONICAL, 1},
    {11, Op::CANONICAL, Op::CANONICAL, 1},
    {26, Op::CANONICAL, Op::CANONICAL, 1},
    {47, Op::CANONICAL, Op::EXPAND_EDGE, 2},
    {38, Op::EXPAND_EDGE, Op::OPEN_HOLE, 2},
    {23, Op::OPEN_HOLE, Op::EXPAND_EDGE, 2},
    {19, Op::OPEN_HOLE, Op::CANONICAL, 1},
    {30, Op::CANONICAL, Op::OPEN_HOLE, 2},
    {42, Op::OPEN_HOLE, Op::CANONICAL, 2},
    {29, Op::OPEN_HOLE, Op::CANONICAL, 2},
    {45, Op::OPEN_HOLE, Op::CANONICAL, 2},
};

/**
 * @brief Result of inserting the edges of the example on a new base mesh.
 */
struct Result
{
    CornerPartition partition;
    unsigned int numberOfPoints = 0;
    unsigned int numberOfElements = 0;
    std::vector<std::string> problems;
};

/**
 * @brief Insert groups of edges on a new base mesh, one call per group, and summarize the final mesh.
 * @param calls The edges of each call.
 * @return The partition of the corners of the input elements, the size of the mesh, and the problems found by the
 * checks (V1 to V7) after the last call.
 */
Result insert(const std::vector<std::vector<unsigned int>> &calls)
{
    IBHM mesh = baseMesh();
    IBHMInterfaceElementBuilder builder(&mesh);
    VertexOrigins origins(mesh.numberPoints());
    for (const auto &edges: calls)
    {
        const MeshSnapshot before = takeSnapshot(mesh);
        builder.insertInterfaceElements(edges);
        origins.update(before, takeSnapshot(mesh));
    }

    const MeshSnapshot last = takeSnapshot(mesh);
    Result result;
    result.partition = cornerPartition(last, INPUT_ELEMENTS);
    result.numberOfPoints = mesh.numberPoints();
    result.numberOfElements = mesh.numberOfElements();
    result.problems = checkTopology(last);
    for (const std::string &problem: origins.checkCoordinates(mesh))
    {
        result.problems.push_back(problem);
    }
    return result;
}
}


/**
 * @brief The base mesh, a builder for it, and the origin of each vertex.
 */
class BaseMesh : public testing::Test
{
protected:
    IBHM mesh = baseMesh();
    IBHMInterfaceElementBuilder builder{&mesh};
    VertexOrigins origins{INPUT_POINTS};
};


TEST_F(BaseMesh, OperatorsAndNewVerticesOfEachInsertion)
{
    ASSERT_EQ(EXAMPLE.size(), baseMeshEdges().size());
    for (size_t i = 0; i < EXAMPLE.size(); i++)
    {
        ASSERT_EQ(EXAMPLE[i].edge, baseMeshEdges()[i]) << "the table must follow the order of baseMeshEdges()";
    }

    for (size_t i = 0; i < EXAMPLE.size(); i++)
    {
        const ExpectedStep &step = EXAMPLE[i];
        SCOPED_TRACE("step " + std::to_string(i + 1) + ", half-edge " + std::to_string(step.edge));

        // The operators are queried before the insertion, as the builder does.
        const unsigned int opposite = mesh.heOpposite(step.edge);
        EXPECT_STREQ(BuilderInspector::operatorName(BuilderInspector::retrieveOperator(builder, step.edge)),
                     BuilderInspector::operatorName(step.operatorA)) << "vertex A";
        EXPECT_STREQ(BuilderInspector::operatorName(BuilderInspector::retrieveOperator(builder, opposite)),
                     BuilderInspector::operatorName(step.operatorB)) << "vertex B";

        const MeshSnapshot before = takeSnapshot(mesh);
        const unsigned int predicted = builder.computeNumberOfNewVertices({step.edge});
        builder.insertInterfaceElements({step.edge});
        const MeshSnapshot after = takeSnapshot(mesh);
        origins.update(before, after);

        EXPECT_EQ(after.numberOfPoints - before.numberOfPoints, step.newVertices) << "vertices created";
        EXPECT_EQ(predicted, step.newVertices) << "computeNumberOfNewVertices() (V6)";
        EXPECT_EQ(after.elements.size(), before.elements.size() + 1) << "one interface element per edge";
        EXPECT_EQ(checkTopology(after), std::vector<std::string>{}) << "topology (V1 to V5)";
        EXPECT_EQ(origins.checkCoordinates(mesh), std::vector<std::string>{}) << "coordinates of the copies (V7)";
        EXPECT_EQ(checkBuilderState(builder, after, INPUT_ELEMENTS), std::vector<std::string>{}) << "builder state";
    }

    // The final mesh is the expected topology (one copy per component of the star of each vertex).
    const MeshSnapshot input = takeSnapshot(baseMesh());
    EXPECT_EQ(checkInsertionResult(input, takeSnapshot(mesh), openableEdges(input, baseMeshEdges())),
              std::vector<std::string>{});
}

TEST_F(BaseMesh, FinalMesh)
{
    builder.insertInterfaceElements(baseMeshEdges());

    EXPECT_EQ(mesh.numberPoints(), INPUT_POINTS + 19);
    EXPECT_EQ(mesh.numberOfElements(), INPUT_ELEMENTS + 12);
    for (unsigned int element = 0; element < mesh.numberOfElements(); element++)
    {
        EXPECT_EQ(BuilderInspector::isInterfaceElement(builder, element), element >= INPUT_ELEMENTS)
            << "element " << element;
    }
    for (unsigned int element = INPUT_ELEMENTS; element < mesh.numberOfElements(); element++)
    {
        EXPECT_EQ(mesh.firstElementHE(element + 1) - mesh.firstElementHE(element), 4u)
            << "interface element " << element << " must have 4 half-edges";
    }
}

TEST_F(BaseMesh, PreReservedCapacityAvoidsReallocations)
{
    // I11: with the capacity reserved in advance for the 12 elements (48 half-edges) and the 19 new vertices, inserting
    // one edge per call never reallocates the element list nor the coordinates, and no memory is left over.
    mesh.reserveCapacityForElements(48, 12);
    mesh.reserveSpaceForNodes(19);
    const unsigned int *elements = mesh.elementsList().data();
    const float *points = mesh.points().data();

    for (const unsigned int edge: baseMeshEdges())
    {
        builder.insertInterfaceElements({edge});
    }

    EXPECT_EQ(mesh.elementsList().data(), elements);
    EXPECT_EQ(mesh.points().data(), points);
    EXPECT_EQ(mesh.numberReservedElements(), 96u);
    EXPECT_EQ(mesh.elementsList().capacity(), 96u);
    EXPECT_EQ(mesh.points().capacity(), (INPUT_POINTS + 19) * 2u);
}

TEST(BaseMeshTopology, OneCallGivesTheSameTopologyAsOneCallPerEdge)
{
    std::vector<std::vector<unsigned int>> oneCallPerEdge;
    for (const unsigned int edge: baseMeshEdges())
    {
        oneCallPerEdge.push_back({edge});
    }

    const Result incremental = insert(oneCallPerEdge);
    const Result batch = insert({baseMeshEdges()});

    EXPECT_EQ(incremental.problems, std::vector<std::string>{});
    EXPECT_EQ(batch.problems, std::vector<std::string>{});
    EXPECT_EQ(batch.numberOfPoints, incremental.numberOfPoints);
    EXPECT_EQ(batch.numberOfElements, incremental.numberOfElements);
    EXPECT_EQ(batch.partition, incremental.partition);
}

TEST(BaseMeshTopology, SeveralCallsGiveTheSameTopologyAsOneCall)
{
    const std::vector<unsigned int> edges = baseMeshEdges();
    const std::vector<unsigned int> first(edges.begin(), edges.begin() + 5);
    const std::vector<unsigned int> second(edges.begin() + 5, edges.end());

    const Result oneCall = insert({edges});
    const Result twoCalls = insert({first, second});

    EXPECT_EQ(twoCalls.problems, std::vector<std::string>{});
    EXPECT_EQ(twoCalls.numberOfPoints, oneCall.numberOfPoints);
    EXPECT_EQ(twoCalls.partition, oneCall.partition);
}

TEST(BaseMeshTopology, OrderDoesNotChangeTheTopology)
{
    const IBHM input = baseMesh();
    const std::vector<unsigned int> edges = baseMeshEdges();
    const Result reference = insert({edges});

    // The reversed order, then random orders where each edge is given by either of its two half-edges. The generator
    // and the shuffle are written here (and not taken from <algorithm>), so the orders are the same on every platform.
    std::vector<std::vector<unsigned int>> orders;
    orders.emplace_back(edges.rbegin(), edges.rend());
    std::mt19937 random(2026);
    for (int i = 0; i < 20; i++)
    {
        std::vector<unsigned int> order = edges;
        for (size_t j = order.size() - 1; j > 0; j--)
        {
            std::swap(order[j], order[random() % (j + 1)]);
        }
        for (unsigned int &edge: order)
        {
            if (random() % 2 == 1)
            {
                edge = input.heOpposite(edge);
            }
        }
        orders.push_back(order);
    }

    for (size_t i = 0; i < orders.size(); i++)
    {
        SCOPED_TRACE("order " + std::to_string(i));
        std::vector<std::vector<unsigned int>> oneCallPerEdge;
        for (const unsigned int edge: orders[i])
        {
            oneCallPerEdge.push_back({edge});
        }
        const Result result = insert(oneCallPerEdge);

        EXPECT_EQ(result.problems, std::vector<std::string>{});
        EXPECT_EQ(result.numberOfPoints, reference.numberOfPoints);
        EXPECT_EQ(result.partition, reference.partition);
    }
}
