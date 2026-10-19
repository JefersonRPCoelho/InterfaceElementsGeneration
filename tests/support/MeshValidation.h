#pragma once
#include <set>
#include <string>
#include <utility>
#include <vector>

class IBHM;


/**
 * @brief Copy of the mesh arrays, taken through the public interface of IBHM.
 *
 * The checks of this file work on the copy and recompute the half-edge queries (element, next, previous and vertex) on
 * their own, so they do not depend on the code under test. Only the valid elements are copied; the space reserved for
 * new elements is not.
 */
struct MeshSnapshot
{
    /**
     * Vertices of each element.
     */
    std::vector<std::vector<unsigned int>> elements;

    /**
     * First half-edge of each element.
     */
    std::vector<unsigned int> firstHalfEdge;

    /**
     * Opposite of each half-edge, with the sentinels IBHM::BORDER and IBHM::COLLAPSED.
     */
    std::vector<unsigned int> opposite;

    /**
     * Element of each half-edge.
     */
    std::vector<unsigned int> elementOf;

    /**
     * Position of each half-edge in its element.
     */
    std::vector<unsigned int> positionOf;

    /**
     * Number of points of the mesh.
     */
    unsigned int numberOfPoints = 0;

    /**
     * @brief Get the next half-edge in the same element.
     * @param he The half-edge.
     * @return The next half-edge.
     */
    [[nodiscard]] unsigned int next(unsigned int he) const;

    /**
     * @brief Get the previous half-edge in the same element.
     * @param he The half-edge.
     * @return The previous half-edge.
     */
    [[nodiscard]] unsigned int previous(unsigned int he) const;

    /**
     * @brief Get the vertex where a half-edge starts.
     * @param he The half-edge.
     * @return The vertex index.
     */
    [[nodiscard]] unsigned int vertex(unsigned int he) const;
};

/**
 * @brief Compare two snapshots: same elements, opposites and number of points.
 * @param a The first snapshot.
 * @param b The second snapshot.
 * @return True if both describe the same mesh with the same numbering.
 */
[[nodiscard]] bool operator==(const MeshSnapshot &a, const MeshSnapshot &b);

/**
 * @brief Copy the valid elements of a mesh.
 * @param mesh The mesh.
 * @return The snapshot.
 */
[[nodiscard]] MeshSnapshot takeSnapshot(const IBHM &mesh);

/**
 * @brief Check the topology of a mesh (V1 to V5 of the report).
 *
 *   V1  opposites are symmetric: O[O[h]] = h for every valid opposite;
 *   V2  opposites agree on the vertices: V[h] = V[next(O[h])] and V[next(h)] = V[O[h]];
 *   V3  a COLLAPSED half-edge has zero length: V[h] = V[next(h)];
 *   V4  manifold: the corners of every vertex form a single fan, connected through h ~ O[prev(h)];
 *   V5  every vertex is used by some element.
 * V6 (the number of created vertices equals the prediction) and V7 (coordinates of the copies, see VertexOrigins) are
 * checked separately.
 * @param mesh The snapshot of the mesh.
 * @return One message per problem found; an empty vector means that the topology is valid.
 */
[[nodiscard]] std::vector<std::string> checkTopology(const MeshSnapshot &mesh);


/**
 * @brief Keep, for each vertex, the original vertex it was copied from (used by V7).
 *
 * A new vertex always replaces an older one in some corner of an element that already existed before the insertion, so
 * comparing the snapshots before and after an insertion is enough to find its origin. An original vertex is its own
 * origin.
 */
class VertexOrigins
{
public:
    /**
     * Value used for a vertex whose origin could not be found.
     */
    static constexpr unsigned int UNKNOWN = 0xFFFFFFFFu;

    /**
     * @brief Start with the vertices of the input mesh, each one being its own origin.
     * @param numberOfInputPoints The number of points of the input mesh.
     */
    explicit VertexOrigins(unsigned int numberOfInputPoints);

    /**
     * @brief Find the origin of the vertices created by an insertion.
     * @param before The snapshot taken before the insertion.
     * @param after The snapshot taken after the insertion.
     */
    void update(const MeshSnapshot &before, const MeshSnapshot &after);

    /**
     * @brief Get the origin of every vertex.
     * @return For each vertex, its original vertex (UNKNOWN if not found).
     */
    [[nodiscard]] const std::vector<unsigned int> &origins() const;

    /**
     * @brief Check that every vertex has exactly the coordinates of its original vertex (V7 of the report).
     * @param mesh The mesh, after the last call to update().
     * @return One message per problem found; an empty vector means that every copy is right.
     */
    [[nodiscard]] std::vector<std::string> checkCoordinates(const IBHM &mesh) const;

private:
    /**
     * Original vertex of each vertex.
     */
    std::vector<unsigned int> _origin;
};


/**
 * A corner of an element: (element, position of the vertex in the element).
 */
using Corner = std::pair<unsigned int, unsigned int>;

/**
 * Partition of the corners by vertex: each set holds the corners that use the same vertex.
 */
using CornerPartition = std::set<std::set<Corner>>;

/**
 * @brief Group the corners of the input elements by vertex (the comparison of compare.py).
 *
 * Two insertions of the same edges in different orders (or in a batch) number the new vertices differently, but the
 * elements of the input mesh keep their indices. The partition of their corners by vertex is the same in two runs if
 * and only if both produced the same topology up to a renumbering of the vertices. The interface elements are not used:
 * they are determined by the elements around them.
 * @param mesh The snapshot of the mesh after the insertions.
 * @param numberOfInputElements The number of elements of the input mesh.
 * @return The partition.
 */
[[nodiscard]] CornerPartition cornerPartition(const MeshSnapshot &mesh, unsigned int numberOfInputElements);
