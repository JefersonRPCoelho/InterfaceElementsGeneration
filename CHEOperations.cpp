/* 
 * File:   CHEOperations.cpp
 * Author: jeferson
 * 
 * Created on 4 de Agosto de 2012, 16:55
 */

#include "CHEOperations.h"
#include <vector>



CHEOperations::CHEOperations(CHE *che)
{
    _che = che;
}



CHEOperations::CHEOperations(const CHEOperations &orig)
{
    _che = orig._che;
}



std::vector<unsigned int> CHEOperations::geNeighbourVertices(const unsigned int he) const
{
    std::vector<unsigned int> neighbourVertices;
    unsigned int currentHE = he;

    //Turn around the vertex until get the starting point or hit a boundary.
    do
    {
        // Get the vertex neighbor to the current half-edge.
        const unsigned int vertex = _che->heVertexIndex(_che->heNext(currentHE));

        // Go to the next face.
        unsigned int oppositeHE = _che->heOpposite(currentHE);

        // If the edge is collapsed, skip it.
        if (oppositeHE == CHE::COLLAPSED)
        {
            currentHE = _che->heNext(currentHE);
            oppositeHE = _che->heOpposite(currentHE);
        }
        else
        {
            // Save it to the neighbor list.
            neighbourVertices.push_back(vertex);
        }

        // If it hits a border, stop.
        if (oppositeHE == CHE::BORDER)
        {
            currentHE = oppositeHE;
            break;
        }

        // Get the next half-edge. Basically it goes back to a half-edge emanating from the vertex.
        currentHE = _che->heNext(oppositeHE);
    }
    while (currentHE != he);

    // If the loop stops because it returns to the beginning, all vertices were already found.
    if (currentHE != CHE::BORDER)
        return neighbourVertices;

    // The loop hit a border. Start from the beginning and turn around the vertex in the other orientation.
    currentHE = he;
    do
    {
        // Loop in the opposite orientation.
        unsigned int hePrevious = _che->hePrevious(currentHE);
        const unsigned int oppositeHE = _che->heOpposite(hePrevious);

        if (oppositeHE == CHE::COLLAPSED)
        {
            currentHE = _che->hePrevious(hePrevious);
            hePrevious = currentHE;
        }
        else
        {
            // Get the vertex neighbor to the current half-edge.
            const unsigned int vertex = _che->heVertexIndex(hePrevious);

            // Save it to the neighbor list.
            neighbourVertices.push_back(vertex);
        }

        // Go to the next face.
        currentHE = _che->heOpposite(hePrevious);
    }
    while (currentHE != CHE::BORDER);

    return neighbourVertices;
}



std::vector<unsigned> CHEOperations::getNeighbourFaces(const unsigned int face) const
{
    std::vector<unsigned> neighbourFaces;
    const unsigned int numberCoordinatesByVertex = _che->numberVertexByElement();

    //Get the face's first half edge id.
    const unsigned int halfEdgeInFace = face * numberCoordinatesByVertex;

    // Initialize the current half-edge to start looping the face.
    unsigned int currentHalfEdge = halfEdgeInFace;

    // Loop the faces getting the opposite half-edge for each edge to get the neighbor faces.
    do
    {
        // Get the neighbor's face half edge.
        const unsigned int opposite = _che->heOpposite(currentHalfEdge);

        // Check if it is not a border.
        if (opposite != CHE::BORDER && opposite != CHE::COLLAPSED)
        {
            // Get the face index.
            const unsigned int faceFound = opposite / numberCoordinatesByVertex;

            // Save the face.
            neighbourFaces.push_back(faceFound);
        }

        // Go the next face's half-edge.
        currentHalfEdge = _che->heNext(currentHalfEdge);
    }
    while (currentHalfEdge != halfEdgeInFace);

    return neighbourFaces;
}



void CHEOperations::addInterfaceElements(const std::vector<unsigned int> &edges)
{
    _che->reserveSpaceForElements(static_cast<unsigned int>(edges.size()));

    // For each edge, add a quad4 element.
    for (const unsigned int he: edges)
    {
        // Get the edge vertices.
        openEdge(he);
        _che->print();
        printf("\n");
    }

    for (auto v: _collapsedVertex2HE)
    {
        printf("%u: %u\n", v.first, v.second);
    }
}



void CHEOperations::openEdge(const unsigned int he)
{
    // Get the opposite half-edge.
    const unsigned int opposite = _che->heOpposite(he);

    // It is not possible to open an edge in the border.
    if (opposite == CHE::BORDER)
        return;

    // Get the next half-edge. It represents the other's edge vertex.
    const unsigned int next = _che->heNext(he);

    // Get the edge vertices.
    const unsigned int vertexA = _che->heVertexIndex(he);
    const unsigned int vertexB = _che->heVertexIndex(next);

    // Get the next half-edge available.
    const unsigned int availableHE = _che->numberOfElements() * _che->numberVertexByElement();

    if (_collapsedVertex2HE.find(vertexA) != _collapsedVertex2HE.end())
    {
        const unsigned int collapsedHE = _collapsedVertex2HE.find(vertexA)->second;
        printf("Vertex %u should be duplicated: %u\n", vertexA, collapsedHE);
        const unsigned int newVertex = duplicateNode(collapsedHE, he);
        _che->_halfEdgeVertex[availableHE + 0] = newVertex;

        _che->_oppositeHalfEdge[availableHE + 3] = collapsedHE;
        _che->_oppositeHalfEdge[collapsedHE] = availableHE + 3;
    }
    else
    {
        _che->_halfEdgeVertex[availableHE + 0] = vertexA;
        _collapsedVertex2HE[vertexA] = availableHE + 3;
        _che->_oppositeHalfEdge[availableHE + 3] = CHE::COLLAPSED;
    }

    if (_collapsedVertex2HE.find(vertexB) != _collapsedVertex2HE.end())
    {
        const unsigned int collapsedHE1 = _collapsedVertex2HE.find(vertexB)->second;
        const unsigned int collapsedHE = _che->heNext(collapsedHE1);
        printf("Vertex %u should be duplicated: %u\n", vertexB, collapsedHE);
        const unsigned int newVertex = duplicateNode(_che->heOpposite(he), collapsedHE);
        _che->_halfEdgeVertex[availableHE + 1] = newVertex;
        _che->_halfEdgeVertex[availableHE + 2] = vertexB;
        _che->_halfEdgeVertex[collapsedHE] = newVertex;

        _che->_oppositeHalfEdge[availableHE + 3] = CHE::COLLAPSED;

        _che->_oppositeHalfEdge[availableHE + 1] = collapsedHE1;
        _che->_oppositeHalfEdge[collapsedHE1] = availableHE + 1;
    }
    else
    {
        _che->_halfEdgeVertex[availableHE + 1] = vertexB;
        _che->_halfEdgeVertex[availableHE + 2] = vertexB;

        // Save the collapsed vertex.
        _collapsedVertex2HE[vertexB] = availableHE + 1;

        _che->_oppositeHalfEdge[availableHE + 1] = CHE::COLLAPSED;
    }

    // Add the element to element's list.
    _che->_halfEdgeVertex[availableHE + 3] = vertexA;

    // Update the opposite's list.
    _che->_oppositeHalfEdge[he] = availableHE + 2;
    _che->_oppositeHalfEdge[availableHE + 2] = he;

    _che->_oppositeHalfEdge[opposite] = availableHE;
    _che->_oppositeHalfEdge[availableHE] = opposite;

    // Increment the number of valid elements.
    _che->_numberOfValidElements++;
}



unsigned int CHEOperations::duplicateNode(const unsigned int startHE, const unsigned int endHE)
{
    const unsigned int availableNode = _che->numberPoints();

    // Get space for extra node.
    _che->reserveSpaceForNodes(1);

    // Remove duplicate node from the list.
    _collapsedVertex2HE.erase(_che->heVertexIndex(startHE));

    // Update all neighbour elements.
    unsigned int currentHE = startHE;
    do
    {
        // Update the element with the new node.
        _che->_halfEdgeVertex[currentHE] = availableNode;

        // Move to the other element.
        currentHE = _che->heOpposite(_che->hePrevious(currentHE));

        if (currentHE == CHE::BORDER)
        {
            // @todo test.
            break;
        }
    }
    while (currentHE != endHE);

    return availableNode;
}

