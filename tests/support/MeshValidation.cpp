#include "MeshValidation.h"
#include <cstddef>
#include <map>
#include "IBHM.h"


namespace
{
const unsigned int BORDER = static_cast<unsigned int>(IBHM::BORDER);
const unsigned int COLLAPSED = static_cast<unsigned int>(IBHM::COLLAPSED);
}



unsigned int MeshSnapshot::next(const unsigned int he) const
{
    const unsigned int element = elementOf[he];
    const auto size = static_cast<unsigned int>(elements[element].size());
    return firstHalfEdge[element] + (positionOf[he] + 1) % size;
}



unsigned int MeshSnapshot::previous(const unsigned int he) const
{
    const unsigned int element = elementOf[he];
    const auto size = static_cast<unsigned int>(elements[element].size());
    return firstHalfEdge[element] + (positionOf[he] + size - 1) % size;
}



unsigned int MeshSnapshot::vertex(const unsigned int he) const
{
    return elements[elementOf[he]][positionOf[he]];
}



bool operator==(const MeshSnapshot &a, const MeshSnapshot &b)
{
    return a.elements == b.elements && a.firstHalfEdge == b.firstHalfEdge && a.opposite == b.opposite &&
           a.numberOfPoints == b.numberOfPoints;
}



MeshSnapshot takeSnapshot(const IBHM &mesh)
{
    MeshSnapshot snapshot;
    snapshot.numberOfPoints = mesh.numberPoints();

    // The half-edges of the valid elements are contiguous and start at 0, so they are copied in index order.
    for (unsigned int element = 0; element < mesh.numberOfElements(); element++)
    {
        const unsigned int first = mesh.firstElementHE(element);
        const unsigned int end = mesh.firstElementHE(element + 1);
        std::vector<unsigned int> vertices;
        for (unsigned int he = first; he < end; he++)
        {
            vertices.push_back(mesh.heVertexIndex(he));
            snapshot.opposite.push_back(mesh.heOpposite(he));
            snapshot.elementOf.push_back(element);
            snapshot.positionOf.push_back(he - first);
        }
        snapshot.firstHalfEdge.push_back(first);
        snapshot.elements.push_back(vertices);
    }
    return snapshot;
}



std::vector<std::string> checkTopology(const MeshSnapshot &mesh)
{
    std::vector<std::string> problems;
    const auto numberOfHalfEdges = static_cast<unsigned int>(mesh.opposite.size());

    // V1, V2 and V3: each half-edge against its opposite.
    for (unsigned int he = 0; he < numberOfHalfEdges; he++)
    {
        const unsigned int opposite = mesh.opposite[he];
        if (opposite == BORDER)
        {
            continue;
        }
        if (opposite == COLLAPSED)
        {
            if (mesh.vertex(he) != mesh.vertex(mesh.next(he)))
            {
                problems.push_back("collapsed he " + std::to_string(he) + " is not degenerate");
            }
            continue;
        }
        if (opposite >= numberOfHalfEdges)
        {
            problems.push_back("opposite out of range at he " + std::to_string(he));
            continue;
        }
        if (mesh.opposite[opposite] != he)
        {
            problems.push_back("asymmetric opposite: O[" + std::to_string(he) + "]=" + std::to_string(opposite) +
                               " but O[" + std::to_string(opposite) + "]=" + std::to_string(mesh.opposite[opposite]));
        }
        if (mesh.vertex(he) != mesh.vertex(mesh.next(opposite)) || mesh.vertex(mesh.next(he)) != mesh.vertex(opposite))
        {
            problems.push_back("vertex mismatch across he " + std::to_string(he) + " / " + std::to_string(opposite));
        }
    }

    // V4: the corners of every vertex must be connected through h ~ O[prev(h)]. It is a union-find over the corners of
    // each vertex. A collapsed half-edge starts at the same vertex as the corner after it, so it links the two corners
    // of the interface element at that vertex.
    std::map<unsigned int, std::vector<unsigned int>> corners;
    for (unsigned int he = 0; he < numberOfHalfEdges; he++)
    {
        corners[mesh.vertex(he)].push_back(he);
    }
    for (const auto &[vertex, vertexCorners]: corners)
    {
        std::map<unsigned int, unsigned int> parent;
        for (const unsigned int corner: vertexCorners)
        {
            parent[corner] = corner;
        }
        const auto find = [&parent](unsigned int corner)
        {
            while (parent[corner] != corner)
            {
                corner = parent[corner];
            }
            return corner;
        };
        for (const unsigned int corner: vertexCorners)
        {
            const unsigned int previous = mesh.previous(corner);
            unsigned int linked = mesh.opposite[previous];
            if (linked == COLLAPSED)
            {
                linked = previous;
            }
            if (linked != BORDER && parent.count(linked) != 0)
            {
                parent[find(corner)] = find(linked);
            }
        }
        std::set<unsigned int> fans;
        for (const unsigned int corner: vertexCorners)
        {
            fans.insert(find(corner));
        }
        if (fans.size() != 1)
        {
            problems.push_back("vertex " + std::to_string(vertex) + " has " + std::to_string(fans.size()) +
                               " disconnected fans");
        }
    }

    // V5: every vertex is used.
    for (unsigned int vertex = 0; vertex < mesh.numberOfPoints; vertex++)
    {
        if (corners.count(vertex) == 0)
        {
            problems.push_back("vertex " + std::to_string(vertex) + " is not referenced by any element");
        }
    }
    return problems;
}



VertexOrigins::VertexOrigins(const unsigned int numberOfInputPoints) : _origin(numberOfInputPoints)
{
    for (unsigned int vertex = 0; vertex < numberOfInputPoints; vertex++)
    {
        _origin[vertex] = vertex;
    }
}



void VertexOrigins::update(const MeshSnapshot &before, const MeshSnapshot &after)
{
    _origin.resize(after.numberOfPoints, UNKNOWN);

    // The elements that existed before keep their indices. A corner that now uses a new vertex used its origin before.
    for (size_t element = 0; element < before.elements.size(); element++)
    {
        for (size_t position = 0; position < before.elements[element].size(); position++)
        {
            const unsigned int newVertex = after.elements[element][position];
            const unsigned int oldVertex = before.elements[element][position];
            if (newVertex >= before.numberOfPoints)
            {
                _origin[newVertex] = _origin[oldVertex];
            }
        }
    }
}



const std::vector<unsigned int> &VertexOrigins::origins() const
{
    return _origin;
}



std::vector<std::string> VertexOrigins::checkCoordinates(const IBHM &mesh) const
{
    std::vector<std::string> problems;
    const std::vector<float> &points = mesh.points();
    const unsigned int dimension = mesh.numberCoordinatesByVertex();
    for (unsigned int vertex = 0; vertex < mesh.numberPoints(); vertex++)
    {
        const unsigned int origin = vertex < _origin.size() ? _origin[vertex] : UNKNOWN;
        if (origin == UNKNOWN)
        {
            problems.push_back("vertex " + std::to_string(vertex) + " has no known original vertex");
            continue;
        }
        const size_t first = static_cast<size_t>(vertex) * dimension;
        const size_t originFirst = static_cast<size_t>(origin) * dimension;
        for (unsigned int i = 0; i < dimension; i++)
        {
            if (points[first + i] != points[originFirst + i])
            {
                problems.push_back("vertex " + std::to_string(vertex) +
                                   " does not have the coordinates of its original vertex " + std::to_string(origin));
                break;
            }
        }
    }
    return problems;
}



CornerPartition cornerPartition(const MeshSnapshot &mesh, const unsigned int numberOfInputElements)
{
    std::map<unsigned int, std::set<Corner>> groups;
    for (unsigned int element = 0; element < numberOfInputElements; element++)
    {
        for (unsigned int position = 0; position < mesh.elements[element].size(); position++)
        {
            groups[mesh.elements[element][position]].insert({element, position});
        }
    }

    CornerPartition partition;
    for (const auto &[vertex, corners]: groups)
    {
        partition.insert(corners);
    }
    return partition;
}
