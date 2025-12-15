/* 
 * File:   CHELevelOne.cpp
 * Author: jeferson
 * 
 * Created on 27 de Julho de 2012, 00:28
 */

#include "CHE.h"
#include <map>
#include <cstring>
#include <cstdio>



CHE::CHE(const double *coordinates, const unsigned int *elementsList, const unsigned int numberOfVertices,
         const unsigned int numberElements, const unsigned int numberVerticesByElement,
         const unsigned int numberCoordinates): _numberCoordinatesPerVertex(numberCoordinates)
                                              , _numberVerticesByElement(numberVerticesByElement)


{
    // Allocate vectors.
    _coordinates.resize(numberCoordinates * numberOfVertices);
    _halfEdgeVertex.resize(numberVerticesByElement * numberElements);
    _oppositeHalfEdge.resize(numberVerticesByElement * numberElements);

    // Copy the data.
    memcpy(_coordinates.data(), coordinates, numberCoordinates * numberOfVertices * sizeof(double));
    memcpy(_halfEdgeVertex.data(), elementsList, numberVerticesByElement * numberElements * sizeof(unsigned int));

    // Build the opposite table.
    buildOppositesLinear();
}



CHE::CHE(
    const std::vector<double> &coordinates,
    const std::vector<unsigned int> &elementsList,
    const unsigned int numberVerticesByElement,
    const unsigned int numberCoordinates): _coordinates(coordinates)
                                         , _halfEdgeVertex(elementsList)
                                         , _numberCoordinatesPerVertex(numberCoordinates)
                                         , _numberVerticesByElement(numberVerticesByElement)

{
    // Allocate the vectors.
    _oppositeHalfEdge.resize(_halfEdgeVertex.size());

    // Build the opposite table.
    buildOppositesLinear();
}



void CHE::buildOppositesLinear()
{
    // Initialize the opposite table with -1.
    memset(_oppositeHalfEdge.data(), -1, _oppositeHalfEdge.size() * sizeof(unsigned int));

    // Get
    std::vector<std::vector<unsigned int> > vertexElementsVector(getNumberPoints());

    // Build an auxiliar list where is given, for each vertex, all elements that have it.
    for (unsigned int element = 0; element < getNumberElements(); element++)
    {
        // Save a map from vertex to element.
        for (unsigned int j = 0; j < _numberVerticesByElement; j++)
        {
            // Get the element vertex.
            const unsigned int vertex = _halfEdgeVertex[_numberVerticesByElement * element + j];

            // Save the element to the vertex list.
            vertexElementsVector[vertex].push_back(_numberVerticesByElement * element + j);
        }
    }

    for (unsigned int i = 0; i < getNumberPoints(); i++)
    {
        printf("%u: ", i);
        for (unsigned int j: vertexElementsVector[i])
        {
            printf("%u ", j);
        }
        printf("\n");
    }
    // Compute the opposite half edge.
    for (unsigned int i = 0; i < getNumberElements(); i++)
    {
        // For each element's half edge.
        for (unsigned int j = 0; j < _numberVerticesByElement; j++)
        {
            // Compute the opposite half edge.
            const unsigned int halfEdge = _numberVerticesByElement * i + j;

            // Avoid recomputing the opposite half edge.
            if (_oppositeHalfEdge[halfEdge] == -1)
            {
                // Get the next half edge. It will enable us to get the edge vertices.
                const unsigned int next = halfEdgeNext(halfEdge);

                // Get the edge vertices.
                const unsigned int vertexA = _halfEdgeVertex[halfEdge];
                const unsigned int vertexB = _halfEdgeVertex[next];

                auto oppositeHalfEdge = static_cast<unsigned int>(-1);

                // Check in the shared elements if there is a reverse edge.
                for (const unsigned int halfEdgeVertex: vertexElementsVector[vertexB])
                {
                    const unsigned int nextHE = halfEdgeNext(halfEdgeVertex);
                    if (_halfEdgeVertex[nextHE] == vertexA)
                    {
                        oppositeHalfEdge = halfEdgeVertex;
                        break;
                    }
                }

                // Check if it is a border.
                if (oppositeHalfEdge == -1)
                    continue;

                _oppositeHalfEdge[halfEdge] = oppositeHalfEdge;
                _oppositeHalfEdge[oppositeHalfEdge] = halfEdge;
            }
        }
    }
}



const std::vector<unsigned int> &CHE::getElementsList()
{
    return _halfEdgeVertex;
}



std::vector<double> &CHE::getCoordinates()
{
    return _coordinates;
}



unsigned int CHE::getHalfEdgeVertexIndex(const unsigned int halfEdge) const
{
    return _halfEdgeVertex[halfEdge];
}



unsigned int CHE::halfEdgeOpposite(unsigned int halfEdge) const
{
    return _oppositeHalfEdge[halfEdge];
}



unsigned int CHE::halfEdgeElement(const unsigned int halfEdge) const
{
    return halfEdge / _numberVerticesByElement;
}



unsigned int CHE::halfEdgeNext(const unsigned int halfEdge) const
{
    return _numberVerticesByElement * halfEdgeElement(halfEdge) + (halfEdge + 1) % _numberVerticesByElement;
}



unsigned int CHE::halfEdgePrevious(const unsigned int halfEdge) const
{
    return _numberVerticesByElement * halfEdgeElement(halfEdge) + (halfEdge + _numberVerticesByElement - 1) %
           _numberVerticesByElement;
}



unsigned int CHE::getNumberPoints() const
{
    return _coordinates.size() / _numberCoordinatesPerVertex;
}



unsigned int CHE::getNumberElements() const
{
    return _halfEdgeVertex.size() / _numberVerticesByElement;
}



unsigned int CHE::getNumberCoordinatesByVertex() const
{
    return _numberCoordinatesPerVertex;
}



unsigned int CHE::getNumberVertexByElement() const
{
    return _numberVerticesByElement;
}



void CHE::getCoordinatesFromHalfEge(const unsigned int halfEdge, double *coordinates) const
{
    for (unsigned int i = 0; i < _numberCoordinatesPerVertex; i++)
    {
        coordinates[i] = _coordinates[_numberCoordinatesPerVertex * _halfEdgeVertex[halfEdge] + i];
    }
}



void CHE::getCoordinatesFromIndex(const unsigned int index, double *coordinates) const
{
    for (unsigned int i = 0; i < _numberCoordinatesPerVertex; i++)
    {
        coordinates[i] = _coordinates[_numberCoordinatesPerVertex * index + i];
    }
}
