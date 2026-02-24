/* 
 * File:   CHELevelOne.cpp
 * Author: jeferson
 * 
 * Created on 27 de Julho de 2012, 00:28
 */

#include "CHE.h"
#include <map>
#include <cstdio>



CHE::CHE(const double *coordinates, const unsigned int *elementsList, const unsigned int numberOfVertices,
         const unsigned int numberElements, const unsigned int numberVerticesByElement,
         const unsigned int numberCoordinates): _numberCoordinatesPerVertex(numberCoordinates)
                                              , _numberVerticesByElement(numberVerticesByElement)
                                              , _numberOfValidElements(numberElements)


{
    // Allocate vectors.
    _coordinates.resize(numberCoordinates * numberOfVertices);
    _halfEdgeVertex.resize(numberVerticesByElement * numberElements);
    _oppositeHalfEdge.resize(numberVerticesByElement * numberElements);
    _isInterfaceElement.resize(numberElements, false);
    _inInterfaceElement.resize(numberOfVertices, false);


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

    _numberOfValidElements = static_cast<unsigned int>(elementsList.size() / numberVerticesByElement);
    _isInterfaceElement.resize(_numberOfValidElements, false);
    _inInterfaceElement.resize(_coordinates.size() / _numberCoordinatesPerVertex, false);


    // Build the opposite table.
    buildOppositesLinear();
}



void CHE::buildOppositesLinear()
{
    // Initialize the opposite table with -1.
    memset(_oppositeHalfEdge.data(), BORDER, _oppositeHalfEdge.size() * sizeof(unsigned int));

    // Get
    std::vector<std::vector<unsigned int> > vertexElementsVector(numberPoints());

    // Build an auxiliar list where is given, for each vertex, all elements that have it.
    for (unsigned int element = 0; element < numberOfElements(); element++)
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

    // Compute the opposite half-edge.
    for (unsigned int i = 0; i < numberOfElements(); i++)
    {
        // For each element's half-edge.
        for (unsigned int j = 0; j < _numberVerticesByElement; j++)
        {
            // Compute the opposite half-edge.
            const unsigned int halfEdge = _numberVerticesByElement * i + j;

            // Avoid recomputing the opposite half-edge.
            if (_oppositeHalfEdge[halfEdge] == BORDER)
            {
                // Get the next half-edge. It will enable us to get the edge vertices.
                const unsigned int next = heNext(halfEdge);

                // Get the edge vertices.
                const unsigned int vertexA = _halfEdgeVertex[halfEdge];
                const unsigned int vertexB = _halfEdgeVertex[next];

                auto oppositeHalfEdge = static_cast<unsigned int>(BORDER);

                // Check in the shared elements if there is a reverse edge.
                for (const unsigned int halfEdgeVertex: vertexElementsVector[vertexB])
                {
                    const unsigned int nextHE = heNext(halfEdgeVertex);
                    if (_halfEdgeVertex[nextHE] == vertexA)
                    {
                        oppositeHalfEdge = halfEdgeVertex;
                        break;
                    }
                }

                // Check if it is a border.
                if (oppositeHalfEdge == BORDER)
                    continue;

                _oppositeHalfEdge[halfEdge] = oppositeHalfEdge;
                _oppositeHalfEdge[oppositeHalfEdge] = halfEdge;
            }
        }
    }
}



const std::vector<unsigned int> &CHE::elementsList()
{
    return _halfEdgeVertex;
}



std::vector<double> &CHE::coordinates()
{
    return _coordinates;
}



unsigned int CHE::heVertexIndex(const unsigned int halfEdge) const
{
    return _halfEdgeVertex[halfEdge];
}



unsigned int CHE::heOpposite(const unsigned int halfEdge) const
{
    return _oppositeHalfEdge[halfEdge];
}



void CHE::print() const
{
    printf("Coordinates:\n");
    for (unsigned int i = 0; i < numberPoints(); i++)
    {
        printf("%u: ", i);
        for (unsigned int j = 0; j < numberCoordinatesByVertex(); j++)
        {
            printf("%.2lf ", _coordinates[i * _numberCoordinatesPerVertex + j]);
        }
        printf(" -> %d\n", static_cast<int>(_inInterfaceElement[i]));
    }
    printf("Elements:\n");
    for (unsigned int i = 0; i < numberOfElements(); i++)
    {
        printf("%u: ", i);
        for (unsigned int j = 0; j < _numberVerticesByElement; j++)
        {
            printf("%u ", _halfEdgeVertex[i * _numberVerticesByElement + j]);
        }
        printf(" -> %d\n", static_cast<int>(_isInterfaceElement[i]));
    }

    printf("Opposites:\n");
    for (unsigned int i = 0; i < numberOfElements() * _numberVerticesByElement; i++)
    {
        printf("%u: %u\n", i, _oppositeHalfEdge[i]);
    }
}



void CHE::reserveSpaceForElements(const unsigned int numberElements)
{
    _halfEdgeVertex.resize(_halfEdgeVertex.size() + _numberVerticesByElement * numberElements);
    _oppositeHalfEdge.resize(_oppositeHalfEdge.size() + _numberVerticesByElement * numberElements);
    _isInterfaceElement.resize(_isInterfaceElement.size() + numberElements);
}



void CHE::reserveSpaceForNodes(const unsigned int numberNodes)
{
    _coordinates.resize(_coordinates.size() + _numberCoordinatesPerVertex * numberNodes);
    _inInterfaceElement.resize(_inInterfaceElement.size() + numberNodes);
}



unsigned int CHE::heElement(const unsigned int halfEdge) const
{
    return halfEdge / _numberVerticesByElement;
}



unsigned int CHE::heNext(const unsigned int halfEdge) const
{
    return _numberVerticesByElement * heElement(halfEdge) + (halfEdge + 1) % _numberVerticesByElement;
}



unsigned int CHE::hePrevious(const unsigned int halfEdge) const
{
    return _numberVerticesByElement * heElement(halfEdge) + (halfEdge + _numberVerticesByElement - 1) %
           _numberVerticesByElement;
}



unsigned int CHE::numberPoints() const
{
    return static_cast<unsigned int>(_coordinates.size()) / _numberCoordinatesPerVertex;
}



unsigned int CHE::numberOfElements() const
{
    return _numberOfValidElements;
}



bool CHE::isInterfaceElement(const unsigned int element) const
{
    if (element >= _isInterfaceElement.size())
    {
        return false;
    }

    return _isInterfaceElement[element];
}



unsigned int CHE::numberReservedElements() const
{
    return static_cast<unsigned int>(_halfEdgeVertex.size()) / _numberVerticesByElement;
}



unsigned int CHE::numberCoordinatesByVertex() const
{
    return _numberCoordinatesPerVertex;
}



unsigned int CHE::numberVertexByElement() const
{
    return _numberVerticesByElement;
}



void CHE::coordinatesFromHalfEge(const unsigned int halfEdge, double *coordinates) const
{
    for (unsigned int i = 0; i < _numberCoordinatesPerVertex; i++)
    {
        coordinates[i] = _coordinates[_numberCoordinatesPerVertex * _halfEdgeVertex[halfEdge] + i];
    }
}



void CHE::coordinatesFromIndex(const unsigned int index, double *coordinates) const
{
    for (unsigned int i = 0; i < _numberCoordinatesPerVertex; i++)
    {
        coordinates[i] = _coordinates[_numberCoordinatesPerVertex * index + i];
    }
}
