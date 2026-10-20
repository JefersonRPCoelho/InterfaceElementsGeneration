#pragma once
#include <cstddef>
#include <ostream>
#include <random>
#include <utility>
#include <vector>
#include "IBHM.h"


/**
 * @brief Deterministic random numbers, the same on every platform.
 *
 * std::mt19937 is fully specified by the standard, but the distributions and std::shuffle are not, so they are written
 * here.
 */
class Random
{
public:
    /**
     * @brief Create the generator.
     * @param seed The seed.
     */
    explicit Random(unsigned int seed);

    /**
     * @brief Draw an integer.
     * @param n The number of possible values (n > 0).
     * @return A value in [0, n).
     */
    [[nodiscard]] unsigned int below(unsigned int n);

    /**
     * @brief Draw an event.
     * @param percent The probability, in percent.
     * @return True with the given probability.
     */
    [[nodiscard]] bool chance(unsigned int percent);

    /**
     * @brief Shuffle a vector (Fisher-Yates).
     * @param values The values.
     */
    template <typename T>
    void shuffle(std::vector<T> &values)
    {
        for (size_t i = values.size(); i > 1; i--)
        {
            std::swap(values[i - 1], values[below(static_cast<unsigned int>(i))]);
        }
    }

private:
    /**
     * The engine.
     */
    std::mt19937 _engine;
};


/**
 * Kinds of random meshes, as in doc/report/tools/stress.py, plus meshes with holes.
 */
enum class MeshKind
{
    QUADRILATERALS,
    TRIANGLES,
    HYBRID,
    POLYGONS,
    HOLES,
};

/**
 * @brief Get the name of a mesh kind.
 * @param kind The kind.
 * @return The name, for example "Triangles".
 */
[[nodiscard]] const char *meshKindName(MeshKind kind);

/**
 * @brief Print a mesh kind in the test names and messages of Google Test (found by argument-dependent lookup).
 * @param kind The kind.
 * @param stream The output stream.
 */
inline void PrintTo(const MeshKind kind, std::ostream *stream)
{
    *stream << meshKindName(kind);
}

/**
 * @brief The arrays of a mesh, ready to build an IBHM (2 coordinates per point).
 */
struct MeshArrays
{
    std::vector<float> coordinates;
    std::vector<unsigned int> elements;
    std::vector<unsigned int> offset;

    /**
     * @brief Build a new IBHM from the arrays.
     * @return The mesh.
     */
    [[nodiscard]] IBHM build() const;
};

/**
 * @brief Build a random mesh on a grid of 3 to 7 by 3 to 7 cells (4 to 8 for the meshes with holes).
 *
 *   QUADRILATERALS  only quadrilaterals;
 *   TRIANGLES       every cell split into two triangles, by one of its diagonals;
 *   HYBRID          60% of the cells split into triangles, the others kept as quadrilaterals;
 *   POLYGONS        like HYBRID, but 25% of the time two neighbor cells are merged into a hexagon;
 *   HOLES           like HYBRID, with about 15% of the cells removed. Holes that touch the outer border open it. The
 *                   drawing is repeated until the mesh is valid (no vertex with two separate fans) and has an interior
 *                   edge, and the unused points are removed.
 * The elements are counterclockwise. The same kind and seed always give the same mesh.
 * @param kind The kind of mesh.
 * @param random The generator.
 * @return The mesh arrays.
 */
[[nodiscard]] MeshArrays randomMesh(MeshKind kind, Random &random);

/**
 * @brief Get one half-edge of each interior edge of a mesh: the smaller of the two.
 * @param mesh The mesh.
 * @return The half-edges, in increasing order.
 */
[[nodiscard]] std::vector<unsigned int> interiorEdges(const IBHM &mesh);
