#include "InsertionChecks.h"
#include <algorithm>
#include <map>
#include <numeric>
#include "IBHM.h"
#include "IBHMInterfaceElementBuilder.h"
#include "BuilderInspector.h"


namespace
{
const unsigned int BORDER = static_cast<unsigned int>(IBHM::BORDER);
const unsigned int COLLAPSED = static_cast<unsigned int>(IBHM::COLLAPSED);

/**
 * @brief Get the opened edge that contains a half-edge of the input mesh, if any.
 * @param input The snapshot of the input mesh.
 * @param edges The opened edges.
 * @param he A half-edge of the input mesh.
 * @return True if the edge of he was opened.
 */
bool isOpened(const MeshSnapshot &input, const std::set<unsigned int> &edges, const unsigned int he)
{
    const unsigned int opposite = input.opposite[he];
    return opposite != BORDER && edges.count(std::min(he, opposite)) != 0;
}
}



std::set<unsigned int> openableEdges(const MeshSnapshot &input, const std::vector<unsigned int> &halfEdges)
{
    std::set<unsigned int> edges;
    const auto numberOfHalfEdges = static_cast<unsigned int>(input.opposite.size());
    for (const unsigned int he: halfEdges)
    {
        if (he < numberOfHalfEdges && input.opposite[he] != BORDER)
        {
            edges.insert(std::min(he, input.opposite[he]));
        }
    }
    return edges;
}



CornerPartition expectedCornerPartition(const MeshSnapshot &input, const std::set<unsigned int> &edges)
{
    // Union-find over the corners of the input mesh (a corner is the half-edge that starts at it).
    const auto numberOfHalfEdges = static_cast<unsigned int>(input.opposite.size());
    std::vector<unsigned int> parent(numberOfHalfEdges);
    std::iota(parent.begin(), parent.end(), 0u);
    const auto find = [&parent](unsigned int corner)
    {
        while (parent[corner] != corner)
        {
            corner = parent[corner];
        }
        return corner;
    };

    // The corner of h and the corner across the edge that arrives at it, O[prev(h)], are in the same component of the
    // star of their vertex, unless that edge was opened (or it is a border edge).
    for (unsigned int corner = 0; corner < numberOfHalfEdges; corner++)
    {
        const unsigned int previous = input.previous(corner);
        const unsigned int neighbor = input.opposite[previous];
        if (neighbor != BORDER && !isOpened(input, edges, previous))
        {
            parent[find(corner)] = find(neighbor);
        }
    }

    std::map<unsigned int, std::set<Corner>> groups;
    for (unsigned int corner = 0; corner < numberOfHalfEdges; corner++)
    {
        groups[find(corner)].insert({input.elementOf[corner], input.positionOf[corner]});
    }
    CornerPartition partition;
    for (const auto &[root, corners]: groups)
    {
        partition.insert(corners);
    }
    return partition;
}



std::vector<std::string> checkInsertionResult(const MeshSnapshot &input, const MeshSnapshot &mesh,
                                              const std::set<unsigned int> &edges)
{
    std::vector<std::string> problems;
    const auto numberOfInputElements = static_cast<unsigned int>(input.elements.size());
    const auto numberOfHalfEdges = static_cast<unsigned int>(mesh.opposite.size());

    // 1 and 2: the partition of the corners and the number of points.
    const CornerPartition expected = expectedCornerPartition(input, edges);
    if (cornerPartition(mesh, numberOfInputElements) != expected)
    {
        problems.push_back("the partition of the corners of the input elements is not the expected one");
    }
    if (mesh.numberOfPoints != expected.size())
    {
        problems.push_back("the mesh has " + std::to_string(mesh.numberOfPoints) + " points, expected " +
                           std::to_string(expected.size()));
    }

    // 3: one interface element per opened edge, between the two half-edges of the edge.
    if (mesh.elements.size() != numberOfInputElements + edges.size())
    {
        problems.push_back("the mesh has " + std::to_string(mesh.elements.size() - numberOfInputElements) +
                           " interface elements, expected " + std::to_string(edges.size()));
    }
    std::set<unsigned int> used;
    for (const unsigned int a: edges)
    {
        const unsigned int b = input.opposite[a];
        const unsigned int oa = mesh.opposite[a];
        const unsigned int ob = mesh.opposite[b];
        const std::string edge = "edge " + std::to_string(a) + "-" + std::to_string(b);
        if (oa >= numberOfHalfEdges || ob >= numberOfHalfEdges)
        {
            problems.push_back(edge + ": a half-edge of the opened edge has no opposite");
            continue;
        }
        const unsigned int element = mesh.elementOf[oa];
        const std::set<unsigned int> positions = {mesh.positionOf[oa], mesh.positionOf[ob]};
        if (element < numberOfInputElements || mesh.elementOf[ob] != element || mesh.elements[element].size() != 4 ||
            positions != std::set<unsigned int>{0, 2})
        {
            problems.push_back(edge + ": not linked to the middle edges of one interface element");
            continue;
        }
        if (!used.insert(element).second)
        {
            problems.push_back(edge + ": its interface element is shared with another edge");
        }
    }

    // 4: one collapsed half-edge for each internal vertex with exactly one opened edge.
    std::vector<bool> border(input.numberOfPoints, false);
    std::vector<unsigned int> openedEdges(input.numberOfPoints, 0);
    for (unsigned int he = 0; he < input.opposite.size(); he++)
    {
        if (input.opposite[he] == BORDER)
        {
            border[input.vertex(he)] = true;
        }
    }
    for (const unsigned int a: edges)
    {
        openedEdges[input.vertex(a)]++;
        openedEdges[input.vertex(input.next(a))]++;
    }
    unsigned int expectedCollapsed = 0;
    for (unsigned int vertex = 0; vertex < input.numberOfPoints; vertex++)
    {
        expectedCollapsed += !border[vertex] && openedEdges[vertex] == 1;
    }
    const auto collapsed = static_cast<unsigned int>(std::count(mesh.opposite.begin(), mesh.opposite.end(), COLLAPSED));
    if (collapsed != expectedCollapsed)
    {
        problems.push_back(std::to_string(collapsed) + " collapsed half-edges, expected " +
                           std::to_string(expectedCollapsed));
    }
    return problems;
}



std::vector<std::string> checkBuilderState(const IBHMInterfaceElementBuilder &builder, const MeshSnapshot &mesh,
                                           const unsigned int numberOfInputElements)
{
    std::vector<std::string> problems;

    // 1: interface element flags.
    if (BuilderInspector::numberOfElementFlags(builder) < mesh.elements.size())
    {
        problems.push_back("fewer interface element flags than elements");
        return problems;
    }
    for (unsigned int element = 0; element < mesh.elements.size(); element++)
    {
        if (BuilderInspector::isInterfaceElement(builder, element) != (element >= numberOfInputElements))
        {
            problems.push_back("wrong interface element flag for the element " + std::to_string(element));
        }
    }

    // 2: vertex flags.
    if (BuilderInspector::numberOfVertexFlags(builder) != mesh.numberOfPoints)
    {
        problems.push_back(std::to_string(BuilderInspector::numberOfVertexFlags(builder)) + " vertex flags for " +
                           std::to_string(mesh.numberOfPoints) + " vertices");
        return problems;
    }
    std::vector<bool> inInterfaceElement(mesh.numberOfPoints, false);
    for (unsigned int element = numberOfInputElements; element < mesh.elements.size(); element++)
    {
        for (const unsigned int vertex: mesh.elements[element])
        {
            inInterfaceElement[vertex] = true;
        }
    }
    for (unsigned int vertex = 0; vertex < mesh.numberOfPoints; vertex++)
    {
        if (BuilderInspector::isInInterfaceElement(builder, vertex) != inInterfaceElement[vertex])
        {
            problems.push_back("wrong interface element flag for the vertex " + std::to_string(vertex));
        }
    }

    // 3: collapsed edges.
    std::map<unsigned int, unsigned int> collapsed;
    for (unsigned int he = 0; he < mesh.opposite.size(); he++)
    {
        if (mesh.opposite[he] == COLLAPSED)
        {
            collapsed[mesh.vertex(he)] = he;
        }
    }
    const std::map<unsigned int, unsigned int> registered(BuilderInspector::collapsed(builder).begin(),
                                                          BuilderInspector::collapsed(builder).end());
    if (registered != collapsed)
    {
        problems.push_back("the collapsed edges map does not match the collapsed half-edges of the mesh");
    }

    // 4: where the collapsed half-edges are.
    std::vector<bool> border(mesh.numberOfPoints, false);
    std::vector<unsigned int> interfaceElements(mesh.numberOfPoints, 0);
    for (unsigned int he = 0; he < mesh.opposite.size(); he++)
    {
        if (mesh.opposite[he] == BORDER)
        {
            border[mesh.vertex(he)] = true;
        }
    }
    for (unsigned int element = numberOfInputElements; element < mesh.elements.size(); element++)
    {
        const std::set<unsigned int> vertices(mesh.elements[element].begin(), mesh.elements[element].end());
        for (const unsigned int vertex: vertices)
        {
            interfaceElements[vertex]++;
        }
    }
    for (const auto &[vertex, he]: collapsed)
    {
        const std::string where = "collapsed half-edge " + std::to_string(he) + ": ";
        if (mesh.elementOf[he] < numberOfInputElements || mesh.positionOf[he] % 2 == 0)
        {
            problems.push_back(where + "not a short side of an interface element");
        }
        if (border[vertex])
        {
            problems.push_back(where + "its vertex is on the border");
        }
        if (interfaceElements[vertex] != 1)
        {
            problems.push_back(where + "its vertex belongs to " + std::to_string(interfaceElements[vertex]) +
                               " interface elements");
        }
    }
    return problems;
}



std::vector<std::string> checkVertexStates(const MeshSnapshot &input, const MeshSnapshot &mesh,
                                           const std::set<unsigned int> &edges,
                                           const std::vector<unsigned int> &origins)
{
    std::vector<std::string> problems;
    const auto numberOfInputElements = static_cast<unsigned int>(input.elements.size());
    const auto numberOfHalfEdges = static_cast<unsigned int>(mesh.opposite.size());

    // State of each original vertex in the input mesh: border or not, and number of opened edges.
    std::vector<bool> border(input.numberOfPoints, false);
    std::vector<unsigned int> openedEdges(input.numberOfPoints, 0);
    for (unsigned int he = 0; he < input.opposite.size(); he++)
    {
        if (input.opposite[he] == BORDER)
        {
            border[input.vertex(he)] = true;
        }
    }
    for (const unsigned int a: edges)
    {
        openedEdges[input.vertex(a)]++;
        openedEdges[input.vertex(input.next(a))]++;
    }

    // The short sides (positions 1 and 3) of the interface elements, by original vertex and kind of opposite.
    std::vector<unsigned int> collapsed(input.numberOfPoints, 0), linked(input.numberOfPoints, 0);
    std::vector<unsigned int> borders(input.numberOfPoints, 0);
    for (unsigned int element = numberOfInputElements; element < mesh.elements.size(); element++)
    {
        for (const unsigned int position: {1u, 3u})
        {
            const unsigned int he = mesh.firstHalfEdge[element] + position;
            const unsigned int copy = mesh.vertex(he);
            const unsigned int vertex = copy < origins.size() ? origins[copy] : input.numberOfPoints;
            if (vertex >= input.numberOfPoints)
            {
                problems.push_back("short side " + std::to_string(he) + ": its vertex has no known original vertex");
                continue;
            }
            const unsigned int opposite = mesh.opposite[he];
            if (opposite == COLLAPSED)
            {
                collapsed[vertex]++;
            }
            else if (opposite == BORDER)
            {
                borders[vertex]++;
            }
            else if (opposite < numberOfHalfEdges && mesh.elementOf[opposite] >= numberOfInputElements &&
                     mesh.positionOf[opposite] % 2 == 1)
            {
                linked[vertex]++;
            }
            else
            {
                problems.push_back("short side " + std::to_string(he) + " is linked to a continuum element");
            }
        }
    }

    for (unsigned int vertex = 0; vertex < input.numberOfPoints; vertex++)
    {
        const unsigned int k = openedEdges[vertex];
        unsigned int expectedCollapsed = 0, expectedLinked = 0, expectedBorders = 0;
        if (border[vertex])
        {
            expectedBorders = k;
        }
        else if (k == 1)
        {
            expectedCollapsed = 1;
        }
        else if (k == 2)
        {
            expectedLinked = 2;
        }
        else
        {
            expectedBorders = k;
        }
        if (collapsed[vertex] != expectedCollapsed || linked[vertex] != expectedLinked ||
            borders[vertex] != expectedBorders)
        {
            problems.push_back("vertex " + std::to_string(vertex) + (border[vertex] ? " (border)" : " (internal)") +
                               " with " + std::to_string(k) + " opened edges: short sides collapsed/linked/border " +
                               std::to_string(collapsed[vertex]) + "/" + std::to_string(linked[vertex]) + "/" +
                               std::to_string(borders[vertex]) + ", expected " + std::to_string(expectedCollapsed) +
                               "/" + std::to_string(expectedLinked) + "/" + std::to_string(expectedBorders));
        }
    }
    return problems;
}
