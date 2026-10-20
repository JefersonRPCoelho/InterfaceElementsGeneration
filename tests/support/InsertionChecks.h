#pragma once
#include <set>
#include <string>
#include <vector>
#include "MeshValidation.h"

class IBHMInterfaceElementBuilder;


/**
 * @brief Get the edges of a list that can be opened in the input mesh.
 *
 * An edge can be opened if it is an interior edge of the input mesh (it has an opposite). Each edge is identified by
 * the smaller of its two half-edges, so an edge given twice, or once by each half-edge, appears once.
 * @param input The snapshot of the input mesh (without interface elements).
 * @param halfEdges The half-edges given as input, valid or not.
 * @return The edges that can be opened.
 */
[[nodiscard]] std::set<unsigned int> openableEdges(const MeshSnapshot &input,
                                                   const std::vector<unsigned int> &halfEdges);

/**
 * @brief Compute the expected partition of the corners of the input elements after opening some edges.
 *
 * It is the invariant of the algorithm, computed from the input mesh only, without the operators: the corners of a
 * vertex are split into the components of its star cut by the opened edges, and each component uses its own copy of the
 * vertex. Two corners of the same vertex are in the same component if they are linked through an edge that was not
 * opened (h ~ O[prev(h)]). So an internal vertex with k >= 1 opened edges ends with k copies (one if k = 1, the
 * collapsed case), and a border vertex with k edges ends with k + 1.
 *
 * Comparing it with cornerPartition() of the result checks that the insertion produced the right topology, and not only
 * a valid one: a vertex with one copy too many or too few, or a corner in the wrong copy, changes the partition.
 * @param input The snapshot of the input mesh (without interface elements).
 * @param edges The opened edges (see openableEdges()).
 * @return The expected partition.
 */
[[nodiscard]] CornerPartition expectedCornerPartition(const MeshSnapshot &input, const std::set<unsigned int> &edges);

/**
 * @brief Check the result of the insertion against the input mesh and the opened edges.
 *
 *   1. The partition of the corners of the input elements is the expected one (expectedCornerPartition()).
 *   2. Every vertex is used by an input element: the number of points equals the number of groups of the partition.
 *   3. There is one interface element per opened edge, with 4 half-edges, placed between the two half-edges of the
 *      edge: their opposites are the middle edges (positions 0 and 2) of the same interface element.
 *   4. The number of collapsed half-edges equals the number of internal vertices with exactly one opened edge.
 * @param input The snapshot of the input mesh.
 * @param mesh The snapshot of the mesh after the insertions.
 * @param edges The opened edges (see openableEdges()).
 * @return One message per problem found; an empty vector means that the result is the expected one.
 */
[[nodiscard]] std::vector<std::string> checkInsertionResult(const MeshSnapshot &input, const MeshSnapshot &mesh,
                                                            const std::set<unsigned int> &edges);

/**
 * @brief Check the bookkeeping of the builder against the mesh (the internal structures of the report, table 3).
 *
 *   1. An element is flagged as interface element if and only if it was created by the builder (index >= the number of
 *      input elements). There is a flag for every element (the flags of the reserved elements are false).
 *   2. A vertex is flagged as part of an interface element if and only if an interface element uses it. There is
 *      exactly one flag per vertex (the count of new vertices is exact).
 *   3. The collapsed edges map has exactly the vertices with a collapsed half-edge, each one with that half-edge.
 *   4. A collapsed half-edge is a short side (position 1 or 3) of an interface element, its vertex is internal (no
 *      border half-edge starts at it), and its vertex belongs to exactly one interface element.
 * @param builder The builder.
 * @param mesh The snapshot of the mesh edited by the builder.
 * @param numberOfInputElements The number of elements of the input mesh.
 * @return One message per problem found; an empty vector means that the bookkeeping is consistent.
 */
[[nodiscard]] std::vector<std::string> checkBuilderState(const IBHMInterfaceElementBuilder &builder,
                                                         const MeshSnapshot &mesh, unsigned int numberOfInputElements);

/**
 * @brief Check the state of every vertex of the input mesh (the state machine of the report, section 3.4).
 *
 * With k opened edges at an original vertex, the k short sides of interface elements at its copies are:
 *   internal vertex: k = 1, collapsed (no copy); k = 2, linked to each other (two copies glued by the short sides);
 *   k >= 3, borders (a hole with k sides);
 *   border vertex: k >= 1, borders (the opening reaches the exterior).
 * @param input The snapshot of the input mesh.
 * @param mesh The snapshot of the mesh after the insertions.
 * @param edges The opened edges (see openableEdges()).
 * @param origins For each vertex of the mesh, the original vertex it was copied from (VertexOrigins::origins()).
 * @return One message per problem found; an empty vector means that every vertex is in the expected state.
 */
[[nodiscard]] std::vector<std::string> checkVertexStates(const MeshSnapshot &input, const MeshSnapshot &mesh,
                                                         const std::set<unsigned int> &edges,
                                                         const std::vector<unsigned int> &origins);
