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



std::vector<unsigned int> CHEOperations::geNeighbourVertices(const unsigned int halfEdge) const
{
    std::vector<unsigned int> neighbourVertices;
    unsigned int currentHalfEdge = halfEdge;

    //Turn around the vertex until get the starting point or hit a boundary.
    do
    {
        // Get the vertex neighbor to the current half-edge.
        const unsigned int vertex = _che->heVertexIndex(_che->heNext(currentHalfEdge));

        // Save it to the neighbor list.
        neighbourVertices.push_back(vertex);

        // Go to the next face.
        currentHalfEdge = _che->heOpposite(currentHalfEdge);

        // If it hits a border, stop.
        if (currentHalfEdge == -1)
        {
            break;
        }

        // Get the next half-edge. Basically it goes back to a half-edge emanating from the vertex.
        currentHalfEdge = _che->heNext(currentHalfEdge);
    }
    while (currentHalfEdge != halfEdge);

    // If the loop stops because it returns to the beginning, all vertices were already found.
    if (currentHalfEdge != -1)
        return neighbourVertices;

    // The loop hit a border. Start from the beginning and turn around the vertex in the other orientation.
    currentHalfEdge = halfEdge;
    do
    {
        // Loop in the opposite orientation.
        const unsigned int halfEdgePrevious = _che->halfEdgePrevious(currentHalfEdge);

        // Get the vertex neighbor to the current half-edge.
        const unsigned int vertex = _che->heVertexIndex(halfEdgePrevious);

        // Save it to the neighbor list.
        neighbourVertices.push_back(vertex);

        // Go to the next face.
        currentHalfEdge = _che->heOpposite(halfEdgePrevious);
    }
    while (currentHalfEdge != -1);

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
        if (opposite != -1)
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
    }
}



void CHEOperations::openEdge(const unsigned int he)
{
    // Get the opposite half-edge.
    const unsigned int opposite = _che->heOpposite(he);

    // It is not possible to open an edge in the border.
    if (opposite == -1)
        return;

    // Get the next half-edge. It represents the other's edge vertex.
    const unsigned int next = _che->heNext(he);

    // Get the edge vertices.
    const unsigned int vertexA = _che->heVertexIndex(he);
    const unsigned int vertexB = _che->heVertexIndex(next);

    // Get the next half-edge available.
    const unsigned int availableHE = _che->numberOfElements() * _che->numberVertexByElement();

    // Add the element to element's list.
    _che->_halfEdgeVertex[availableHE + 0] = vertexA;
    _che->_halfEdgeVertex[availableHE + 1] = vertexB;
    _che->_halfEdgeVertex[availableHE + 2] = vertexB;
    _che->_halfEdgeVertex[availableHE + 3] = vertexA;

    // Update the opposite's list.
    _che->_oppositeHalfEdge[he] = availableHE + 2;
    _che->_oppositeHalfEdge[availableHE + 2] = he;

    _che->_oppositeHalfEdge[opposite] = availableHE;
    _che->_oppositeHalfEdge[availableHE] = opposite;

    // Invalid opposites. @todo think better about this.
    _che->_oppositeHalfEdge[availableHE + 1] = -2;
    _che->_oppositeHalfEdge[availableHE + 3] = -2;

    // Increment the number of valid elements.
    _che->_numberOfValidElements++;
}

