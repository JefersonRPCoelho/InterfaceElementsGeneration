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
    long int currentHalfEdge = halfEdge;

    //Turn around the vertex until get the starting point or hit a boundary.
    do
    {
        // Get the vertex neighbour to the current half edge.
        const unsigned int vertex = _che->getHalfEdgeVertexIndex(_che->halfEdgeNext(currentHalfEdge));

        // Save it to the neighbours list.
        neighbourVertices.push_back(vertex);

        // Go the next face.
        currentHalfEdge = _che->halfEdgeOpposite(currentHalfEdge);

        // If it hits a border, stop.
        if (currentHalfEdge == -1)
        {
            break;
        }

        // Get the next half edge. Basically it goes back to a half edge emanating from the vertex.
        currentHalfEdge = _che->halfEdgeNext(currentHalfEdge);
    }
    while (currentHalfEdge != halfEdge);

    // If the loop stops because it returns to the beginning, all vertices were already found.
    if (currentHalfEdge != -1)
        return neighbourVertices;

    // The loop hit a border. Start from beginning and turn around the vertex in the other orientation.
    currentHalfEdge = halfEdge;
    do
    {
        // Loop in the opposite orientation.
        const long int halfEdgePrevious = _che->halfEdgePrevious(currentHalfEdge);

        // Get the vertex neighbour to the current half edge.
        const int vertex = _che->getHalfEdgeVertexIndex(halfEdgePrevious);

        // Save it to the neighbours list.
        neighbourVertices.push_back(vertex);

        // Go the next face.
        currentHalfEdge = _che->halfEdgeOpposite(halfEdgePrevious);
    }
    while (currentHalfEdge != -1);

    return neighbourVertices;
}



std::vector<unsigned> CHEOperations::getNeighbourFaces(const unsigned int face) const
{
    std::vector<unsigned> neighbourFaces;
    const unsigned int numberCoordinatesByVertex = _che->getNumberVertexByElement();

    //Get the face's first half edge id.
    const unsigned int halfEdgeInFace = face * numberCoordinatesByVertex;

    // Initialize the current half edge to start looping the face.
    unsigned int currentHalfEdge = halfEdgeInFace;

    // Loop the faces getting the opposite half edge for each edge in order to get the neighbour faces.
    do
    {
        // Get the neighbour's face half edge.
        const unsigned int opposite = _che->halfEdgeOpposite(currentHalfEdge);

        // Check if it is not a border.
        if (opposite != -1)
        {
            // Get the face index.
            const unsigned int faceFound = opposite / numberCoordinatesByVertex;

            // Save the face.
            neighbourFaces.push_back(faceFound);
        }

        // Go the next face's half edge.
        currentHalfEdge = _che->halfEdgeNext(currentHalfEdge);
    }
    while (currentHalfEdge != halfEdgeInFace);

    return neighbourFaces;
}

