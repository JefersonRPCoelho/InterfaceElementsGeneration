// Tests on random meshes, checked against the expected topology.
//
// For each kind of mesh (quadrilaterals, triangles, hybrid, polygons and meshes with holes), random meshes and random
// selections of interior edges are inserted in three ways: one edge per call (random order, either half-edge of each
// edge), all edges in a single call, and small batches mixed with input that must be rejected (border edges, indices
// that are not half-edges of the mesh, repeated edges, half-edges of interface elements). After every call:
//   - V1 to V7, and the count of computeNumberOfNewVertices(), which must not change the mesh;
//   - the expected topology for the edges given so far, computed from the input mesh only (the invariant), and the
//     state of every original vertex (InsertionChecks.h);
//   - the bookkeeping of the builder;
//   - the memory policy: the coordinates and the element list have no capacity beyond their size (exact growth).
//
// Unlike doc/report/tools/stress.py, which checks that the result is valid and the same for every order, these tests
// also check that it is the right one.
#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "InsertionChecks.h"
#include "MeshValidation.h"
#include "RandomMeshes.h"

namespace
{
/**
 * Number of random meshes of each kind.
 */
constexpr unsigned int MESHES_PER_KIND = 40;

/**
 * Number of random meshes of each kind where every interior edge is opened.
 */
constexpr unsigned int FULLY_OPENED_MESHES_PER_KIND = 10;

/**
 * @brief Insert groups of half-edges on a new mesh, one call per group, and check the mesh and the builder after every
 * call and the result at the end.
 * @param arrays The input mesh.
 * @param calls The half-edges of each call (valid or not).
 * @param edges The edges that must be opened (see openableEdges()).
 * @return The problems found, prefixed by the call; the checks stop at the first call with a problem.
 */
std::vector<std::string> insertAndCheck(const MeshArrays &arrays, const std::vector<std::vector<unsigned int>> &calls,
                                        const std::set<unsigned int> &edges)
{
    IBHM mesh = arrays.build();
    IBHMInterfaceElementBuilder builder(&mesh);
    const MeshSnapshot input = takeSnapshot(mesh);
    const auto numberOfInputElements = static_cast<unsigned int>(input.elements.size());
    VertexOrigins origins(input.numberOfPoints);

    std::vector<std::string> problems;
    std::vector<unsigned int> given;
    const auto add = [&problems](const std::string &prefix, const std::vector<std::string> &found)
    {
        for (const std::string &problem: found)
        {
            problems.push_back(prefix + problem);
        }
    };

    for (size_t call = 0; call < calls.size() && problems.empty(); call++)
    {
        const std::string prefix = "call " + std::to_string(call) + ": ";
        const MeshSnapshot before = takeSnapshot(mesh);

        const unsigned int predicted = builder.computeNumberOfNewVertices(calls[call]);
        if (!(takeSnapshot(mesh) == before))
        {
            problems.push_back(prefix + "computeNumberOfNewVertices() changed the mesh");
        }

        builder.insertInterfaceElements(calls[call]);
        const MeshSnapshot after = takeSnapshot(mesh);
        origins.update(before, after);

        const unsigned int created = after.numberOfPoints - before.numberOfPoints;
        if (created != predicted)
        {
            problems.push_back(prefix + "created " + std::to_string(created) + " vertices, predicted " +
                               std::to_string(predicted));
        }
        add(prefix, checkTopology(after));
        add(prefix, origins.checkCoordinates(mesh));
        add(prefix, checkBuilderState(builder, after, numberOfInputElements));

        // The invariant holds after every call, for the edges given so far.
        given.insert(given.end(), calls[call].begin(), calls[call].end());
        const std::set<unsigned int> soFar = openableEdges(input, given);
        add(prefix, checkInsertionResult(input, after, soFar));
        add(prefix, checkVertexStates(input, after, soFar, origins.origins()));

        // Exact memory: no capacity beyond the size.
        if (mesh.points().capacity() != mesh.points().size() ||
            mesh.elementsList().capacity() != mesh.elementsList().size())
        {
            problems.push_back(prefix + "capacity beyond the size (coordinates or element list)");
        }
    }

    if (problems.empty())
    {
        add("result: ", checkInsertionResult(input, takeSnapshot(mesh), edges));
    }
    return problems;
}

/**
 * @brief Choose, for each edge, one of its two half-edges at random, and shuffle the edges.
 * @param input The snapshot of the input mesh.
 * @param edges The edges (one half-edge each).
 * @param random The generator.
 * @return The half-edges, in random order.
 */
std::vector<unsigned int> randomHalfEdges(const MeshSnapshot &input, const std::set<unsigned int> &edges,
                                          Random &random)
{
    std::vector<unsigned int> halfEdges;
    for (const unsigned int edge: edges)
    {
        halfEdges.push_back(random.chance(50) ? edge : input.opposite[edge]);
    }
    random.shuffle(halfEdges);
    return halfEdges;
}

/**
 * @brief Split the half-edges into small batches (1 to 4 edges) mixed with input that must be rejected.
 *
 * The rejected input: border half-edges of the input mesh, an index that is not a half-edge of the mesh, an edge
 * already given (by either half-edge, in an earlier batch or in the same one), and an index after the input half-edges,
 * which is a half-edge of an interface element when the builder has already created one, or is not a half-edge of the
 * mesh otherwise.
 * @param input The snapshot of the input mesh.
 * @param halfEdges The half-edges to insert.
 * @param random The generator.
 * @return The calls.
 */
std::vector<std::vector<unsigned int>> noisyBatches(const MeshSnapshot &input,
                                                    const std::vector<unsigned int> &halfEdges, Random &random)
{
    std::vector<unsigned int> borderHalfEdges;
    for (unsigned int he = 0; he < input.opposite.size(); he++)
    {
        if (input.opposite[he] == static_cast<unsigned int>(IBHM::BORDER))
        {
            borderHalfEdges.push_back(he);
        }
    }
    const auto numberOfInputHalfEdges = static_cast<unsigned int>(input.opposite.size());

    std::vector<std::vector<unsigned int>> calls;
    std::vector<unsigned int> given;
    size_t next = 0;
    while (next < halfEdges.size())
    {
        std::vector<unsigned int> call;
        const unsigned int size = 1 + random.below(4);
        for (unsigned int i = 0; i < size && next < halfEdges.size(); i++)
        {
            call.push_back(halfEdges[next]);
            given.push_back(halfEdges[next]);
            next++;
        }

        const unsigned int noise = random.below(3);
        for (unsigned int i = 0; i < noise; i++)
        {
            switch (random.below(4))
            {
                case 0:
                    call.push_back(borderHalfEdges[random.below(static_cast<unsigned int>(borderHalfEdges.size()))]);
                    break;
                case 1:
                    call.push_back(0x7FFFFFFFu);
                    break;
                case 2:
                {
                    const unsigned int edge = given[random.below(static_cast<unsigned int>(given.size()))];
                    call.push_back(random.chance(50) ? edge : input.opposite[edge]);
                    break;
                }
                default:
                    call.push_back(numberOfInputHalfEdges + random.below(4 * static_cast<unsigned int>(given.size())));
                    break;
            }
        }
        random.shuffle(call);
        calls.push_back(call);
    }
    return calls;
}

/**
 * @brief Make one call per half-edge.
 * @param halfEdges The half-edges.
 * @return The calls.
 */
std::vector<std::vector<unsigned int>> oneCallPerEdge(const std::vector<unsigned int> &halfEdges)
{
    std::vector<std::vector<unsigned int>> calls;
    for (const unsigned int he: halfEdges)
    {
        calls.push_back({he});
    }
    return calls;
}

/**
 * @brief The seed of a random mesh.
 * @param kind The kind of mesh.
 * @param mesh The index of the mesh in its kind.
 * @return The seed.
 */
unsigned int seedOf(const MeshKind kind, const unsigned int mesh)
{
    return 1000 * static_cast<unsigned int>(kind) + mesh;
}
}


/**
 * @brief Random meshes of one kind.
 */
class RandomMeshes : public testing::TestWithParam<MeshKind>
{
};


TEST_P(RandomMeshes, RandomSelectionsGiveTheExpectedTopology)
{
    for (unsigned int index = 0; index < MESHES_PER_KIND; index++)
    {
        const unsigned int seed = seedOf(GetParam(), index);
        SCOPED_TRACE("seed " + std::to_string(seed));
        Random random(seed);
        const MeshArrays arrays = randomMesh(GetParam(), random);
        const IBHM mesh = arrays.build();
        const MeshSnapshot input = takeSnapshot(mesh);

        // Up to half of the interior edges, at least one.
        std::vector<unsigned int> candidates = interiorEdges(mesh);
        random.shuffle(candidates);
        const unsigned int count = 1 + random.below(std::max(1u, static_cast<unsigned int>(candidates.size()) / 2));
        const std::set<unsigned int> edges(candidates.begin(), candidates.begin() + count);

        const std::vector<unsigned int> order = randomHalfEdges(input, edges, random);
        EXPECT_EQ(insertAndCheck(arrays, oneCallPerEdge(order), edges), std::vector<std::string>{})
            << "one edge per call";
        EXPECT_EQ(insertAndCheck(arrays, {randomHalfEdges(input, edges, random)}, edges), std::vector<std::string>{})
            << "single call";
        EXPECT_EQ(insertAndCheck(arrays, noisyBatches(input, randomHalfEdges(input, edges, random), random), edges),
                  std::vector<std::string>{})
            << "batches with rejected input";
    }
}

TEST_P(RandomMeshes, EveryInteriorEdgeGivesTheExpectedTopology)
{
    // Every interior edge is opened: each corner of the input mesh ends with its own vertex, and every input element is
    // connected to the others only through interface elements.
    for (unsigned int index = 0; index < FULLY_OPENED_MESHES_PER_KIND; index++)
    {
        const unsigned int seed = seedOf(GetParam(), 500 + index);
        SCOPED_TRACE("seed " + std::to_string(seed));
        Random random(seed);
        const MeshArrays arrays = randomMesh(GetParam(), random);
        const IBHM mesh = arrays.build();
        const MeshSnapshot input = takeSnapshot(mesh);
        const std::vector<unsigned int> all = interiorEdges(mesh);
        const std::set<unsigned int> edges(all.begin(), all.end());

        EXPECT_EQ(insertAndCheck(arrays, oneCallPerEdge(randomHalfEdges(input, edges, random)), edges),
                  std::vector<std::string>{})
            << "one edge per call";
        EXPECT_EQ(insertAndCheck(arrays, {randomHalfEdges(input, edges, random)}, edges), std::vector<std::string>{})
            << "single call";
    }
}

INSTANTIATE_TEST_SUITE_P(Kinds, RandomMeshes,
                         testing::Values(MeshKind::QUADRILATERALS, MeshKind::TRIANGLES, MeshKind::HYBRID,
                                         MeshKind::POLYGONS, MeshKind::HOLES),
                         [](const testing::TestParamInfo<MeshKind> &info)
                         {
                             return std::string(meshKindName(info.param));
                         });
