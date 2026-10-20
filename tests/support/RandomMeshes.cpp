#include "RandomMeshes.h"
#include <cstddef>
#include "MeshValidation.h"



Random::Random(const unsigned int seed) : _engine(seed)
{
}



unsigned int Random::below(const unsigned int n)
{
    return static_cast<unsigned int>(_engine() % n);
}



bool Random::chance(const unsigned int percent)
{
    return below(100) < percent;
}



const char *meshKindName(const MeshKind kind)
{
    switch (kind)
    {
        case MeshKind::QUADRILATERALS:
            return "Quadrilaterals";
        case MeshKind::TRIANGLES:
            return "Triangles";
        case MeshKind::HYBRID:
            return "Hybrid";
        case MeshKind::POLYGONS:
            return "Polygons";
        case MeshKind::HOLES:
        default:
            return "Holes";
    }
}



IBHM MeshArrays::build() const
{
    return IBHM(coordinates, elements, offset, 2);
}



namespace
{
/**
 * @brief Draw a mesh on a grid, before the validation of the meshes with holes.
 * @param kind The kind of mesh.
 * @param random The generator.
 * @return The mesh arrays (with all the grid points, used or not).
 */
MeshArrays drawGrid(const MeshKind kind, Random &random)
{
    const bool holes = kind == MeshKind::HOLES;
    const unsigned int nx = (holes ? 4 : 3) + random.below(5);
    const unsigned int ny = (holes ? 4 : 3) + random.below(5);
    const auto id = [nx](const unsigned int i, const unsigned int j) { return j * (nx + 1) + i; };

    MeshArrays mesh;
    for (unsigned int j = 0; j <= ny; j++)
    {
        for (unsigned int i = 0; i <= nx; i++)
        {
            mesh.coordinates.push_back(static_cast<float>(i));
            mesh.coordinates.push_back(static_cast<float>(j));
        }
    }

    mesh.offset.push_back(0);
    const auto add = [&mesh](const std::vector<unsigned int> &element)
    {
        mesh.elements.insert(mesh.elements.end(), element.begin(), element.end());
        mesh.offset.push_back(static_cast<unsigned int>(mesh.elements.size()));
    };

    for (unsigned int j = 0; j < ny; j++)
    {
        unsigned int i = 0;
        while (i < nx)
        {
            const unsigned int a = id(i, j), b = id(i + 1, j), c = id(i + 1, j + 1), d = id(i, j + 1);
            if (holes && random.chance(15))
            {
                i++;
                continue;
            }
            if (kind == MeshKind::POLYGONS && i + 1 < nx && random.chance(25))
            {
                // Two cells merged into a hexagon (two of its vertices are on straight sides).
                add({a, b, id(i + 2, j), id(i + 2, j + 1), c, d});
                i += 2;
                continue;
            }
            const bool split = kind == MeshKind::TRIANGLES || (kind != MeshKind::QUADRILATERALS && random.chance(60));
            if (split && random.chance(50))
            {
                add({a, b, c});
                add({a, c, d});
            }
            else if (split)
            {
                add({a, b, d});
                add({b, c, d});
            }
            else
            {
                add({a, b, c, d});
            }
            i++;
        }
    }
    return mesh;
}

/**
 * @brief Remove the points that no element uses, renumbering the others.
 * @param mesh The mesh arrays.
 */
void removeUnusedPoints(MeshArrays &mesh)
{
    const size_t numberOfPoints = mesh.coordinates.size() / 2;
    std::vector<unsigned int> newIndex(numberOfPoints, 0);
    std::vector<bool> used(numberOfPoints, false);
    for (const unsigned int vertex: mesh.elements)
    {
        used[vertex] = true;
    }
    std::vector<float> coordinates;
    for (size_t point = 0; point < numberOfPoints; point++)
    {
        if (used[point])
        {
            newIndex[point] = static_cast<unsigned int>(coordinates.size() / 2);
            coordinates.push_back(mesh.coordinates[2 * point]);
            coordinates.push_back(mesh.coordinates[2 * point + 1]);
        }
    }
    for (unsigned int &vertex: mesh.elements)
    {
        vertex = newIndex[vertex];
    }
    mesh.coordinates = coordinates;
}
}



MeshArrays randomMesh(const MeshKind kind, Random &random)
{
    while (true)
    {
        MeshArrays mesh = drawGrid(kind, random);
        if (kind != MeshKind::HOLES)
        {
            return mesh;
        }

        // Meshes with holes: keep only valid meshes (removing cells may leave a vertex with two separate fans) that
        // have an interior edge.
        if (mesh.offset.size() < 3)
        {
            continue;
        }
        removeUnusedPoints(mesh);
        const IBHM ibhm = mesh.build();
        if (checkTopology(takeSnapshot(ibhm)).empty() && !interiorEdges(ibhm).empty())
        {
            return mesh;
        }
    }
}



std::vector<unsigned int> interiorEdges(const IBHM &mesh)
{
    std::vector<unsigned int> edges;
    for (unsigned int he = 0; he < mesh.numberOfHalfEdges(); he++)
    {
        const unsigned int opposite = mesh.heOpposite(he);
        if (opposite != static_cast<unsigned int>(IBHM::BORDER) && he < opposite)
        {
            edges.push_back(he);
        }
    }
    return edges;
}
