// Tests of the structure of the algorithm: each testable principle of the report (section 3) has a test here.
//
//   P1  operators and template: each insertion creates one element [F, G, H, E] at the end of the element list, with
//       h2 <-> a and h0 <-> b, E and F copies of A, G and H copies of B, each one on its side;
//   P2  vertex-centered decision: the operator chosen at a vertex is the one given by the state of the vertex alone,
//       computed here from the mesh, independently of the builder;
//   --  contract of each operator: new vertices, short side, collapsed edges, corners moved (Split Element moves
//       exactly two, the O(1) case); P5 (deferred duplication) is the contract of Canonical on an internal vertex;
//   P3  locality: only the corners and the opposites of the stars of the two vertices of the edge change;
//   P4  purely topological: the coordinates do not change the result (same arrays with other coordinates);
//   --  Split Element is a particular case of Expand Edge: replacing it gives the same topology.
// The invariant (one copy per component of the star), the state of each vertex (section 3.4), the bookkeeping of the
// builder, the exact count and the memory policy are checked after every call in test_random_meshes.cpp.
#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "BuilderInspector.h"
#include "InsertionChecks.h"
#include "MeshValidation.h"
#include "RandomMeshes.h"

namespace
{
using Op = IBHMInterfaceElementBuilder::OperatorType;
const unsigned int BORDER = static_cast<unsigned int>(IBHM::BORDER);
const unsigned int COLLAPSED = static_cast<unsigned int>(IBHM::COLLAPSED);

/**
 * Number of random meshes of each kind.
 */
constexpr unsigned int MESHES_PER_KIND = 15;

/**
 * @brief A random mesh and a random selection of its interior edges, one half-edge each, in random order.
 */
struct Case
{
    MeshArrays arrays;
    std::vector<unsigned int> halfEdges;
};

/**
 * @brief Draw a random mesh of a kind and a selection of up to half of its interior edges (at least one).
 * @param kind The kind of mesh.
 * @param index The index of the mesh in its kind.
 * @return The case.
 */
Case randomCase(const MeshKind kind, const unsigned int index)
{
    Random random(7000 + 1000 * static_cast<unsigned int>(kind) + index);
    Case result{randomMesh(kind, random), {}};
    const IBHM mesh = result.arrays.build();
    std::vector<unsigned int> edges = interiorEdges(mesh);
    random.shuffle(edges);
    const unsigned int count = 1 + random.below(std::max(1u, static_cast<unsigned int>(edges.size()) / 2));
    for (unsigned int i = 0; i < count; i++)
    {
        result.halfEdges.push_back(random.chance(50) ? edges[i] : mesh.heOpposite(edges[i]));
    }
    return result;
}

/**
 * @brief Check if a vertex is on the border of the mesh (a half-edge without opposite starts at it).
 * @param mesh The snapshot.
 * @param vertex The vertex.
 * @return True if the vertex is on the border.
 */
bool isBorderVertex(const MeshSnapshot &mesh, const unsigned int vertex)
{
    for (unsigned int he = 0; he < mesh.opposite.size(); he++)
    {
        if (mesh.opposite[he] == BORDER && mesh.vertex(he) == vertex)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief The operator that the state of the vertex of a half-edge calls for, computed from the mesh alone (P2).
 *
 *   the vertex is not used by any interface element -> Canonical;
 *   it has no collapsed edge -> Open Hole;
 *   the interface element that has its collapsed edge is the neighbor of one of the two elements of the new edge,
 *   through the other edge of that element at the vertex -> Split Element; otherwise -> Expand Edge.
 * @param mesh The snapshot of the mesh before the insertion.
 * @param numberOfInputElements The number of elements of the input mesh (the others are interface elements).
 * @param he The half-edge of the new edge that starts at the vertex.
 * @return The operator.
 */
Op expectedOperator(const MeshSnapshot &mesh, const unsigned int numberOfInputElements, const unsigned int he)
{
    const unsigned int vertex = mesh.vertex(he);
    bool inInterfaceElement = false;
    for (unsigned int element = numberOfInputElements; element < mesh.elements.size(); element++)
    {
        const std::vector<unsigned int> &vertices = mesh.elements[element];
        if (std::find(vertices.begin(), vertices.end(), vertex) != vertices.end())
        {
            inInterfaceElement = true;
        }
    }
    if (!inInterfaceElement)
    {
        return Op::CANONICAL;
    }

    const auto numberOfHalfEdges = static_cast<unsigned int>(mesh.opposite.size());
    unsigned int collapsed = numberOfHalfEdges;
    for (unsigned int h = 0; h < numberOfHalfEdges; h++)
    {
        if (mesh.opposite[h] == COLLAPSED && mesh.vertex(h) == vertex)
        {
            collapsed = h;
        }
    }
    if (collapsed == numberOfHalfEdges)
    {
        return Op::OPEN_HOLE;
    }

    const unsigned int interfaceElement = mesh.elementOf[collapsed];
    const unsigned int x1 = mesh.opposite[mesh.previous(he)];
    const unsigned int x3 = mesh.opposite[mesh.next(mesh.opposite[he])];
    const bool shared = (x1 < numberOfHalfEdges && mesh.elementOf[x1] == interfaceElement) ||
                        (x3 < numberOfHalfEdges && mesh.elementOf[x3] == interfaceElement);
    return shared ? Op::SPLIT_ELEMENT : Op::EXPAND_EDGE;
}

/**
 * @brief Count the corners of the elements that existed before an insertion that moved away from a vertex.
 * @param before The snapshot before the insertion.
 * @param after The snapshot after the insertion.
 * @param vertex The vertex.
 * @return The number of corners that used the vertex before and use another one after.
 */
unsigned int movedCorners(const MeshSnapshot &before, const MeshSnapshot &after, const unsigned int vertex)
{
    unsigned int moved = 0;
    for (unsigned int he = 0; he < before.opposite.size(); he++)
    {
        moved += before.vertex(he) == vertex && after.vertex(he) != vertex;
    }
    return moved;
}

/**
 * @brief Check the effect of the operator applied at one vertex of an insertion.
 * @param before The snapshot before the insertion.
 * @param after The snapshot after the insertion.
 * @param op The operator applied.
 * @param he The half-edge of the new edge that starts at the vertex.
 * @param x The short side of the new element at the vertex (h3 at A, h1 at B).
 * @param collapsedBefore The collapsed edges of the builder before the insertion.
 * @param collapsedAfter The collapsed edges of the builder after the insertion.
 * @return The problems found.
 */
std::vector<std::string> checkOperator(const MeshSnapshot &before, const MeshSnapshot &after, const Op op,
                                       const unsigned int he, const unsigned int x,
                                       const std::unordered_map<unsigned int, unsigned int> &collapsedBefore,
                                       const std::unordered_map<unsigned int, unsigned int> &collapsedAfter)
{
    std::vector<std::string> problems;
    const auto fail = [&problems, op](const std::string &what)
    {
        problems.push_back(std::string(BuilderInspector::operatorName(op)) + ": " + what);
    };
    const unsigned int v = before.vertex(he);
    const unsigned int n = after.vertex(x) == v ? after.vertex(after.next(x)) : after.vertex(x);
    const bool newVertex = n >= before.numberOfPoints;
    const unsigned int moved = movedCorners(before, after, v);

    if (op == Op::CANONICAL && !isBorderVertex(before, v))
    {
        // P5: the first contact with an internal vertex only collapses it.
        if (after.vertex(x) != v || after.vertex(after.next(x)) != v || after.opposite[x] != COLLAPSED)
        {
            fail("the short side is not a collapsed edge at the vertex");
        }
        if (collapsedAfter.count(v) == 0 || collapsedAfter.at(v) != x)
        {
            fail("the collapsed edge is not registered");
        }
        if (moved != 0)
        {
            fail("corners moved");
        }
    }
    else if (op == Op::CANONICAL)
    {
        if (!newVertex || after.vertex(x) != n || after.vertex(after.next(x)) != v || after.opposite[x] != BORDER)
        {
            fail("expected a new vertex at the short side, which is a border");
        }
        if (moved == 0)
        {
            fail("no corner moved to the new vertex");
        }
    }
    else if (op == Op::SPLIT_ELEMENT || op == Op::EXPAND_EDGE)
    {
        const auto c = collapsedBefore.find(v);
        if (c == collapsedBefore.end())
        {
            fail("the vertex had no collapsed edge");
            return problems;
        }
        if (!newVertex || after.opposite[x] != c->second || after.opposite[c->second] != x)
        {
            fail("expected a new vertex and the former collapsed edge linked to the short side");
        }
        if (collapsedAfter.count(v) != 0)
        {
            fail("the vertex is still collapsed");
        }
        if (op == Op::SPLIT_ELEMENT && moved != 2)
        {
            fail("Split Element must move exactly two corners (O(1)), moved " + std::to_string(moved));
        }
        if (op == Op::EXPAND_EDGE && moved < 2)
        {
            fail("Expand Edge must move at least two corners, moved " + std::to_string(moved));
        }
    }
    else
    {
        if (!newVertex || after.vertex(x) != n || after.vertex(after.next(x)) != v || after.opposite[x] != BORDER)
        {
            fail("expected a new vertex at the short side, which is a border");
        }
        if (moved == 0)
        {
            fail("no corner moved to the new vertex");
        }
    }
    return problems;
}
}


/**
 * @brief Random meshes of one kind.
 */
class AlgorithmStructure : public testing::TestWithParam<MeshKind>
{
};


TEST_P(AlgorithmStructure, EveryInsertionFollowsTheTemplate)
{
    for (unsigned int index = 0; index < MESHES_PER_KIND; index++)
    {
        SCOPED_TRACE("mesh " + std::to_string(index));
        const Case test = randomCase(GetParam(), index);
        IBHM mesh = test.arrays.build();
        IBHMInterfaceElementBuilder builder(&mesh);
        VertexOrigins origins(mesh.numberPoints());

        for (const unsigned int a: test.halfEdges)
        {
            SCOPED_TRACE("half-edge " + std::to_string(a));
            const MeshSnapshot before = takeSnapshot(mesh);
            const unsigned int b = before.opposite[a];
            const auto first = static_cast<unsigned int>(before.opposite.size());
            builder.insertInterfaceElements({a});
            const MeshSnapshot after = takeSnapshot(mesh);
            origins.update(before, after);
            const std::vector<unsigned int> &origin = origins.origins();

            // One element with four half-edges, at the end of the element list.
            ASSERT_EQ(after.elements.size(), before.elements.size() + 1);
            ASSERT_EQ(after.firstHalfEdge.back(), first);
            ASSERT_EQ(after.elements.back().size(), 4u);

            // The middle edges: h0 <-> b and h2 <-> a.
            EXPECT_EQ(after.opposite[a], first + 2);
            EXPECT_EQ(after.opposite[b], first);

            // [F, G, H, E]: E and F are copies of A, on the side of U (element of a) and of L (element of b); G and H
            // are copies of B, on the side of L and of U.
            const unsigned int E = after.vertex(first + 3), F = after.vertex(first);
            const unsigned int G = after.vertex(first + 1), H = after.vertex(first + 2);
            EXPECT_EQ(E, after.vertex(a));
            EXPECT_EQ(F, after.vertex(after.next(b)));
            EXPECT_EQ(G, after.vertex(b));
            EXPECT_EQ(H, after.vertex(after.next(a)));
            EXPECT_EQ(origin[E], origin[before.vertex(a)]);
            EXPECT_EQ(origin[F], origin[before.vertex(a)]);
            EXPECT_EQ(origin[G], origin[before.vertex(b)]);
            EXPECT_EQ(origin[H], origin[before.vertex(b)]);
        }
    }
}

TEST_P(AlgorithmStructure, TheOperatorDependsOnlyOnTheStateOfTheVertex)
{
    for (unsigned int index = 0; index < MESHES_PER_KIND; index++)
    {
        SCOPED_TRACE("mesh " + std::to_string(index));
        const Case test = randomCase(GetParam(), index);
        IBHM mesh = test.arrays.build();
        IBHMInterfaceElementBuilder builder(&mesh);
        const auto numberOfInputElements = mesh.numberOfElements();

        for (const unsigned int a: test.halfEdges)
        {
            const MeshSnapshot before = takeSnapshot(mesh);
            for (const unsigned int he: {a, before.opposite[a]})
            {
                SCOPED_TRACE("half-edge " + std::to_string(he));
                EXPECT_STREQ(BuilderInspector::operatorName(BuilderInspector::retrieveOperator(builder, he)),
                             BuilderInspector::operatorName(expectedOperator(before, numberOfInputElements, he)));
            }
            builder.insertInterfaceElements({a});
        }
    }
}

TEST_P(AlgorithmStructure, EachOperatorHasItsEffect)
{
    std::map<Op, unsigned int> applied;
    for (unsigned int index = 0; index < MESHES_PER_KIND; index++)
    {
        SCOPED_TRACE("mesh " + std::to_string(index));
        const Case test = randomCase(GetParam(), index);
        IBHM mesh = test.arrays.build();
        IBHMInterfaceElementBuilder builder(&mesh);

        for (const unsigned int a: test.halfEdges)
        {
            SCOPED_TRACE("half-edge " + std::to_string(a));
            const MeshSnapshot before = takeSnapshot(mesh);
            const unsigned int b = before.opposite[a];
            const auto first = static_cast<unsigned int>(before.opposite.size());
            const Op opA = BuilderInspector::retrieveOperator(builder, a);
            const Op opB = BuilderInspector::retrieveOperator(builder, b);
            const auto collapsedBefore = BuilderInspector::collapsed(builder);

            builder.insertInterfaceElements({a});
            const MeshSnapshot after = takeSnapshot(mesh);
            const auto &collapsedAfter = BuilderInspector::collapsed(builder);

            EXPECT_EQ(checkOperator(before, after, opA, a, first + 3, collapsedBefore, collapsedAfter),
                      std::vector<std::string>{}) << "vertex A";
            EXPECT_EQ(checkOperator(before, after, opB, b, first + 1, collapsedBefore, collapsedAfter),
                      std::vector<std::string>{}) << "vertex B";

            // Every operator creates one vertex, except Canonical on an internal vertex.
            unsigned int expected = 0;
            for (const auto &[op, he]: {std::pair<Op, unsigned int>{opA, a}, {opB, b}})
            {
                expected += op == Op::CANONICAL && !isBorderVertex(before, before.vertex(he)) ? 0 : 1;
                applied[op]++;
            }
            EXPECT_EQ(after.numberOfPoints - before.numberOfPoints, expected);
        }
    }
    // The meshes exercise the operators (Split Element and Expand Edge need a vertex with two edges).
    EXPECT_GT(applied[Op::CANONICAL], 0u);
    EXPECT_GT(applied[Op::SPLIT_ELEMENT] + applied[Op::EXPAND_EDGE], 0u);
}

TEST_P(AlgorithmStructure, OnlyTheStarsOfTheTwoVerticesChange)
{
    for (unsigned int index = 0; index < MESHES_PER_KIND; index++)
    {
        SCOPED_TRACE("mesh " + std::to_string(index));
        const Case test = randomCase(GetParam(), index);
        IBHM mesh = test.arrays.build();
        IBHMInterfaceElementBuilder builder(&mesh);

        for (const unsigned int a: test.halfEdges)
        {
            SCOPED_TRACE("half-edge " + std::to_string(a));
            const MeshSnapshot before = takeSnapshot(mesh);
            const std::set<unsigned int> vertices = {before.vertex(a), before.vertex(before.opposite[a])};
            builder.insertInterfaceElements({a});
            const MeshSnapshot after = takeSnapshot(mesh);

            for (unsigned int he = 0; he < before.opposite.size(); he++)
            {
                const bool touches = vertices.count(before.vertex(he)) != 0 ||
                                     vertices.count(before.vertex(before.next(he))) != 0;
                if (after.vertex(he) != before.vertex(he))
                {
                    EXPECT_TRUE(vertices.count(before.vertex(he)) != 0) << "corner of half-edge " << he << " changed";
                }
                if (after.opposite[he] != before.opposite[he])
                {
                    EXPECT_TRUE(touches) << "opposite of half-edge " << he << " changed";
                }
            }
        }
    }
}

TEST_P(AlgorithmStructure, CoordinatesDoNotChangeTheResult)
{
    for (unsigned int index = 0; index < MESHES_PER_KIND; index++)
    {
        SCOPED_TRACE("mesh " + std::to_string(index));
        const Case test = randomCase(GetParam(), index);

        // The same topology with all the points at the origin, and with random coordinates.
        MeshArrays zero = test.arrays, scrambled = test.arrays;
        std::fill(zero.coordinates.begin(), zero.coordinates.end(), 0.0f);
        Random random(index);
        for (float &coordinate: scrambled.coordinates)
        {
            coordinate = static_cast<float>(random.below(2000)) / 100.0f - 10.0f;
        }

        std::vector<MeshSnapshot> results;
        for (const MeshArrays *arrays: std::vector<const MeshArrays *>{&test.arrays, &zero, &scrambled})
        {
            IBHM mesh = arrays->build();
            IBHMInterfaceElementBuilder builder(&mesh);
            for (const unsigned int a: test.halfEdges)
            {
                builder.insertInterfaceElements({a});
            }
            results.push_back(takeSnapshot(mesh));
        }
        EXPECT_TRUE(results[1] == results[0]) << "points at the origin";
        EXPECT_TRUE(results[2] == results[0]) << "random coordinates";
    }
}

TEST_P(AlgorithmStructure, SplitElementIsAParticularCaseOfExpandEdge)
{
    unsigned int replaced = 0;
    for (unsigned int index = 0; index < MESHES_PER_KIND; index++)
    {
        SCOPED_TRACE("mesh " + std::to_string(index));
        const Case test = randomCase(GetParam(), index);

        // The builder, the replica of its insertion (it must give the same mesh), and the replica with Expand Edge
        // instead of Split Element (it must give the same topology, the expected one).
        IBHM real = test.arrays.build(), replica = test.arrays.build(), expand = test.arrays.build();
        IBHMInterfaceElementBuilder realBuilder(&real), replicaBuilder(&replica), expandBuilder(&expand);
        const MeshSnapshot input = takeSnapshot(real);
        VertexOrigins origins(input.numberOfPoints);
        std::vector<unsigned int> given;

        for (const unsigned int a: test.halfEdges)
        {
            SCOPED_TRACE("half-edge " + std::to_string(a));
            given.push_back(a);
            realBuilder.insertInterfaceElements({a});
            EXPECT_EQ(BuilderInspector::insertEdge(replicaBuilder, a, false), 0u);
            const MeshSnapshot before = takeSnapshot(expand);
            replaced += BuilderInspector::insertEdge(expandBuilder, a, true);
            const MeshSnapshot after = takeSnapshot(expand);
            origins.update(before, after);

            ASSERT_TRUE(takeSnapshot(replica) == takeSnapshot(real)) << "the replica diverges from the builder";
            const std::set<unsigned int> edges = openableEdges(input, given);
            EXPECT_EQ(checkTopology(after), std::vector<std::string>{});
            EXPECT_EQ(origins.checkCoordinates(expand), std::vector<std::string>{});
            EXPECT_EQ(checkBuilderState(expandBuilder, after, static_cast<unsigned int>(input.elements.size())),
                      std::vector<std::string>{});
            EXPECT_EQ(checkInsertionResult(input, after, edges), std::vector<std::string>{});
            EXPECT_EQ(cornerPartition(after, static_cast<unsigned int>(input.elements.size())),
                      cornerPartition(takeSnapshot(real), static_cast<unsigned int>(input.elements.size())));
        }
    }
    EXPECT_GT(replaced, 0u) << "no Split Element was replaced: the test does not exercise the equivalence";
}

INSTANTIATE_TEST_SUITE_P(Kinds, AlgorithmStructure,
                         testing::Values(MeshKind::QUADRILATERALS, MeshKind::TRIANGLES, MeshKind::HYBRID,
                                         MeshKind::POLYGONS, MeshKind::HOLES),
                         [](const testing::TestParamInfo<MeshKind> &info)
                         {
                             return std::string(meshKindName(info.param));
                         });
