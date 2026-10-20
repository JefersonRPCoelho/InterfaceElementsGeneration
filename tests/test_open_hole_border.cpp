// Tests of the Open Hole operator on border vertices (defect I1 of report v0.4, fixed after it).
//
// A border vertex (outer border or border of a hole in the mesh) that receives a second edge is already part of an
// interface element and has no collapsed edge, so the rules choose Open Hole. Its fan goes from border to border, so
// one of the two rotations of the operator may reach the border before an interface element: on that side the hole is
// open to the exterior and there is nothing else to change. Until the fix, the first rotation stopped at an assertion
// (or wrote outside the vector in release) and the second one applied next() to BORDER.
//
// Each test checks V1 to V7 after every call (V6 = the count of computeNumberOfNewVertices()), the number of copies of
// the border vertex (k + 1 with k edges), and that every order, and the insertion in a single call, give the same
// topology.
#include <cstddef>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "BuilderInspector.h"
#include "MeshValidation.h"
#include "TestMeshes.h"

namespace
{
/**
 * @brief Result of inserting groups of edges on a new mesh.
 */
struct Result
{
    CornerPartition partition;
    unsigned int newVertices = 0;
    std::set<unsigned int> copies;
    std::vector<std::string> problems;
};

/**
 * @brief Insert groups of edges on a new mesh, one call per group, checking the mesh after every call.
 * @param makeMesh Function that builds the input mesh.
 * @param calls The edges of each call.
 * @param vertex A vertex of the input mesh whose copies are collected.
 * @return The partition of the corners of the input elements, the number of new vertices, the copies of the vertex
 * (the vertices used, at the end, by the input corners of the vertex), and the problems found by the checks.
 */
Result insert(IBHM (*makeMesh)(), const std::vector<std::vector<unsigned int>> &calls, const unsigned int vertex)
{
    IBHM mesh = makeMesh();
    IBHMInterfaceElementBuilder builder(&mesh);
    const MeshSnapshot input = takeSnapshot(mesh);
    VertexOrigins origins(input.numberOfPoints);

    Result result;
    for (const auto &edges: calls)
    {
        const MeshSnapshot before = takeSnapshot(mesh);
        const unsigned int predicted = builder.computeNumberOfNewVertices(edges);
        builder.insertInterfaceElements(edges);
        const MeshSnapshot after = takeSnapshot(mesh);
        origins.update(before, after);

        const unsigned int created = after.numberOfPoints - before.numberOfPoints;
        if (created != predicted)
        {
            result.problems.push_back("created " + std::to_string(created) + " vertices, predicted " +
                                      std::to_string(predicted));
        }
        for (const std::string &problem: checkTopology(after))
        {
            result.problems.push_back(problem);
        }
        for (const std::string &problem: origins.checkCoordinates(mesh))
        {
            result.problems.push_back(problem);
        }
    }

    const MeshSnapshot last = takeSnapshot(mesh);
    const auto numberOfInputElements = static_cast<unsigned int>(input.elements.size());
    result.partition = cornerPartition(last, numberOfInputElements);
    result.newVertices = last.numberOfPoints - input.numberOfPoints;
    for (unsigned int element = 0; element < numberOfInputElements; element++)
    {
        for (size_t position = 0; position < input.elements[element].size(); position++)
        {
            if (input.elements[element][position] == vertex)
            {
                result.copies.insert(last.elements[element][position]);
            }
        }
    }
    return result;
}

/**
 * @brief Insert the same edges in several ways and check that all of them are valid and give the same result.
 * @param makeMesh Function that builds the input mesh.
 * @param ways The calls of each way of inserting the edges.
 * @param vertex The border vertex being tested.
 * @param copies The expected number of copies of the vertex.
 * @param newVertices The expected number of new vertices in the whole mesh.
 */
void expectSameValidResult(IBHM (*makeMesh)(), const std::vector<std::vector<std::vector<unsigned int>>> &ways,
                           const unsigned int vertex, const size_t copies, const unsigned int newVertices)
{
    const Result reference = insert(makeMesh, ways[0], vertex);
    for (size_t i = 0; i < ways.size(); i++)
    {
        SCOPED_TRACE("way " + std::to_string(i));
        const Result result = i == 0 ? reference : insert(makeMesh, ways[i], vertex);

        EXPECT_EQ(result.problems, std::vector<std::string>{});
        EXPECT_EQ(result.copies.size(), copies) << "copies of the border vertex";
        EXPECT_EQ(result.newVertices, newVertices);
        EXPECT_EQ(result.partition, reference.partition) << "same topology as the first way";
    }
}
}


TEST(OpenHoleBorder, TheRulesChooseOpenHoleForTheSecondEdge)
{
    // Fan mesh: after Q-R (half-edge 4), Q is in an interface element and has no collapsed edge.
    IBHM mesh = fanMesh();
    IBHMInterfaceElementBuilder builder(&mesh);
    builder.insertInterfaceElements({4});

    EXPECT_STREQ(BuilderInspector::operatorName(BuilderInspector::retrieveOperator(builder, 8)), "OPEN_HOLE");
}

TEST(OpenHoleBorder, FanMeshBothOrdersAndOneCall)
{
    // Q-R then Q-P: the counterclockwise rotation of the Open Hole reaches the border. Q-P then Q-R: the clockwise one
    // does. Q ends with 3 copies; R and P (border, one edge each) with 2. New vertices: 2 + 1 + 1.
    expectSameValidResult(fanMesh, {{{4}, {8}}, {{8}, {4}}, {{4, 8}}, {{8, 4}}}, 0, 3, 4);
}

TEST(OpenHoleBorder, BaseMeshVertexH)
{
    // The border vertex H = (3, 1) of the base mesh has two interior edges: H-M (half-edge 26, M->H) and G-H
    // (half-edge 20, G->H). H-M then G-H: the counterclockwise rotation reaches the border D-H. G-H then H-M: the
    // clockwise one reaches the border H-I. H ends with 3 copies; G and M are internal and only collapse.
    expectSameValidResult(baseMesh, {{{26}, {20}}, {{20}, {26}}, {{26, 20}}}, 7, 3, 2);
}

TEST(OpenHoleBorder, BaseMeshExampleThenGH)
{
    // The 12 edges of the example (H gets H-M at step 4), then G-H. G is already in an interface element (1 new
    // vertex) and H is opened again (1 new vertex): 19 + 2.
    std::vector<std::vector<unsigned int>> oneCallPerEdge;
    for (const unsigned int edge: baseMeshEdges())
    {
        oneCallPerEdge.push_back({edge});
    }
    oneCallPerEdge.push_back({20});
    std::vector<unsigned int> all = baseMeshEdges();
    all.push_back(20);

    expectSameValidResult(baseMesh, {oneCallPerEdge, {all}}, 7, 3, 21);
}

TEST(OpenHoleBorder, VertexOnTheBorderOfAHole)
{
    // meshIBHM.txt: the vertex 4 is on the border of the inner hole, with the edges 0-4 (half-edge 3) and 4-8
    // (half-edge 19). The vertices 0 and 8 are on the outer border. New vertices: 2 at the vertex 4, 1 at 0 and 1 at 8.
    expectSameValidResult(holeMesh, {{{3}, {19}}, {{19}, {3}}, {{3, 19}}}, 4, 3, 4);
}

TEST(OpenHoleBorder, BorderVertexWithThreeEdges)
{
    // Wide fan mesh: Q has the edges Q-p2 (4), Q-p3 (8) and Q-p4 (12). When Q-p3 comes last, it is between two
    // interface elements, so both rotations end at an interface element, as on an internal vertex; otherwise one of
    // them reaches the border. Q ends with 4 copies; p2, p3 and p4 (border, one edge each) with 2. New: 3 + 3.
    const std::vector<std::vector<unsigned int>> orders = {{4, 8, 12}, {4, 12, 8}, {8, 4, 12}, {8, 12, 4}, {12, 4, 8},
                                                           {12, 8, 4}};
    std::vector<std::vector<std::vector<unsigned int>>> ways;
    for (const auto &order: orders)
    {
        std::vector<std::vector<unsigned int>> oneCallPerEdge;
        for (const unsigned int edge: order)
        {
            oneCallPerEdge.push_back({edge});
        }
        ways.push_back(oneCallPerEdge);
    }
    ways.push_back({{4, 8, 12}});

    expectSameValidResult(wideFanMesh, ways, 0, 4, 6);
}
