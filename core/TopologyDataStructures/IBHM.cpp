/* 
 * Author: jeferson
 * 
 * Created on 27 de Julho de 2012, 00:28
 */

#include "IBHM.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>


IBHM::IBHM(
    const std::vector<float> &coordinates,
    const std::vector<unsigned int> &elementsList,
    const std::vector<unsigned int> &offset,
    const unsigned int numberCoordinates): _coordinates(coordinates)
                                         , _halfEdgeVertex(elementsList)
                                         , _numberCoordinatesPerVertex(numberCoordinates)
                                         , _offset(offset)


{
    // Every point needs at least one coordinate (it is also used to compute the number of points below).
    assert(numberCoordinates > 0);

    // Allocate the vectors.
    _oppositeHalfEdge.resize(_halfEdgeVertex.size());

    _numberOfValidElements = static_cast<unsigned int>(_offset.size() - 1);
    _numberOfValidPoints = static_cast<unsigned int>(coordinates.size() / numberCoordinates);

    // Build the opposite table.
    buildOppositesLinear();
}



void IBHM::buildOppositesLinear()
{
    // Initialize the opposite table with -1.
    memset(_oppositeHalfEdge.data(), BORDER, _oppositeHalfEdge.size() * sizeof(unsigned int));

    // Get
    std::vector<std::vector<unsigned int> > vertexElementsVector(numberPoints());

    // Build an auxiliar list where is given, for each vertex, all elements that have it.
    for (unsigned int element = 0; element < numberOfElements(); element++)
    {
        // Save a map from vertex to element.
        for (unsigned int j = _offset[element]; j < _offset[element + 1]; j++)
        {
            // Get the element vertex.
            const unsigned int vertex = _halfEdgeVertex[j];

            // Save the element to the vertex list.
            vertexElementsVector[vertex].push_back(j);
        }
    }

    // Compute the opposite half-edge.
    for (unsigned int i = 0; i < numberOfElements(); i++)
    {
        // For each element's half-edge.
        for (unsigned int halfEdge = _offset[i]; halfEdge < _offset[i + 1]; halfEdge++)
        {
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



const std::vector<unsigned int> &IBHM::elementsList()
{
    return _halfEdgeVertex;
}



unsigned int IBHM::heVertexIndex(const unsigned int halfEdge) const
{
    assert(halfEdge < _halfEdgeVertex.size());

    return _halfEdgeVertex[halfEdge];
}



void IBHM::print() const
{
    printf("Coordinates:\n");
    for (unsigned int i = 0; i < numberPoints(); i++)
    {
        printf("%u: ", i);
        for (unsigned int j = 0; j < numberCoordinatesByVertex(); j++)
        {
            printf("%.2lf ", _coordinates[i * _numberCoordinatesPerVertex + j]);
        }
        printf("\n");
    }
    printf("Elements:\n");
    for (unsigned int i = 0; i < numberOfElements(); i++)
    {
        printf("%u: ", i);
        for (unsigned int j = _offset[i]; j < _offset[i + 1]; j++)
        {
            printf("%u ", _halfEdgeVertex[j]);
        }
        printf("\n");
    }

    printf("Opposites:\n");
    for (unsigned int i = 0; i < numberOfElements(); i++)
    {
        printf("Element %u:\n", i);
        for (unsigned int j = _offset[i]; j < _offset[i + 1]; j++)
        {
            printf("%u: %u\n", j, _oppositeHalfEdge[j]);
        }
        printf("\n");
    }
}



void IBHM::reserveSpaceForElements(const unsigned int numberPositions, const unsigned int numberElements)
{
    _offset.resize(_offset.size() + numberElements);
    _halfEdgeVertex.resize(_halfEdgeVertex.size() + numberPositions);

    // The new positions start as borders (see the header). With the default value (0), they would point to half-edge 0.
    _oppositeHalfEdge.resize(_oppositeHalfEdge.size() + numberPositions, static_cast<unsigned int>(BORDER));
}



void IBHM::reserveSpaceForNodes(const unsigned int numberNodes)
{
    // Only the capacity grows, and exactly to the requested amount. The size keeps matching the number of points, so
    // addPoint() can simply append the new coordinates.
    _coordinates.reserve(_coordinates.size() + static_cast<size_t>(_numberCoordinatesPerVertex) * numberNodes);
}



std::vector<unsigned int>::const_iterator IBHM::offsetIterator(const unsigned int he) const
{
    auto it = std::upper_bound(_offset.begin(), _offset.begin() + _numberOfValidElements + 1, he);
    if (it != _offset.begin())
        --it;

    return it;
}



unsigned int IBHM::heOpposite(const unsigned int halfEdge) const
{
    assert(halfEdge < _halfEdgeVertex.size());

    return _oppositeHalfEdge[halfEdge];
}



unsigned int IBHM::heElement(const unsigned int halfEdge) const
{
    // Only half-edges of valid elements are accepted. The sentinels BORDER and COLLAPSED, and the positions reserved
    // and not committed yet, are not half-edges of an element: the binary search would return a wrong element. The
    // caller must test an opposite before using it here.
    assert(halfEdge < numberOfHalfEdges());

    return static_cast<unsigned int>(offsetIterator(halfEdge) - _offset.begin());
}



unsigned int IBHM::heNext(const unsigned int halfEdge) const
{
    // Only half-edges of valid elements are accepted. The sentinels BORDER and COLLAPSED, and the positions reserved
    // and not committed yet, are not half-edges of an element: the binary search would return a wrong element. The
    // caller must test an opposite before using it here.
    assert(halfEdge < numberOfHalfEdges());

    // Get the element first half-edge.
    const auto it = offsetIterator(halfEdge);
    assert(it != _offset.end());

    // Get the element size.
    const size_t position = it - _offset.begin();
    const size_t elementSize = _offset[position + 1] - _offset[position];
    const unsigned int firstHE = (*it);

    assert(position < _offset.size() - 1);

    // Compute the next half-edge.
    return firstHE + (halfEdge - firstHE + 1) % elementSize;
}



unsigned int IBHM::hePrevious(const unsigned int halfEdge) const
{
    // Only half-edges of valid elements are accepted. The sentinels BORDER and COLLAPSED, and the positions reserved
    // and not committed yet, are not half-edges of an element: the binary search would return a wrong element. The
    // caller must test an opposite before using it here.
    assert(halfEdge < numberOfHalfEdges());

    // Get the element first half-edge.
    const auto it = offsetIterator(halfEdge);
    assert(it != _offset.end());

    // Get the element size.
    const size_t position = it - _offset.begin();
    const unsigned int elementSize = _offset[position + 1] - _offset[position];
    const unsigned int firstHE = (*it);

    assert(position < _offset.size() - 1);

    // Compute the previous half-edge.
    return firstHE + ((halfEdge - firstHE) + (elementSize - 1)) % elementSize;
}



unsigned int IBHM::numberPoints() const
{
    return _numberOfValidPoints;
}



const std::vector<float> &IBHM::points() const
{
    return _coordinates;
}



unsigned int IBHM::addPoint(const std::vector<float> &coordinates)
{
    // The coordinate's vector always has one entry per coordinate of each valid point, so the new point goes to the
    // end.
    assert(_coordinates.size() == static_cast<size_t>(_numberOfValidPoints) * _numberCoordinatesPerVertex);

    // An empty vector means the origin. Otherwise, there must be one value for each coordinate.
    assert(coordinates.empty() || coordinates.size() == _numberCoordinatesPerVertex);

    for (unsigned int i = 0; i < _numberCoordinatesPerVertex; i++)
    {
        _coordinates.push_back(i < coordinates.size() ? coordinates[i] : 0.0f);
    }
    _numberOfValidPoints++;

    return _numberOfValidPoints - 1;
}



unsigned int IBHM::duplicatePoint(const unsigned int vertex)
{
    assert(vertex < _numberOfValidPoints);
    assert(_coordinates.size() == static_cast<size_t>(_numberOfValidPoints) * _numberCoordinatesPerVertex);

    // Each value is copied to a local variable before being appended. The source is accessed by index, so a
    // reallocation caused by push_back does not invalidate it.
    const size_t first = static_cast<size_t>(vertex) * _numberCoordinatesPerVertex;
    for (unsigned int i = 0; i < _numberCoordinatesPerVertex; i++)
    {
        const float value = _coordinates[first + i];
        _coordinates.push_back(value);
    }
    _numberOfValidPoints++;

    return _numberOfValidPoints - 1;
}



unsigned int IBHM::commitElement(const unsigned int elementSize)
{
    assert(_numberOfValidElements + 1 < _offset.size());

    // Increment the number of elements.
    _numberOfValidElements++;

    // Compute the upper bound for the next element.
    _offset[_numberOfValidElements] = _offset[_numberOfValidElements - 1] + elementSize;

    return _numberOfValidElements - 1;
}



void IBHM::setOpposite(const unsigned int halfEdge, const unsigned int oppositeHalfEdge)
{
    assert(halfEdge < _oppositeHalfEdge.size());

    _oppositeHalfEdge[halfEdge] = oppositeHalfEdge;
    if (oppositeHalfEdge != BORDER && oppositeHalfEdge != COLLAPSED)
    {
        _oppositeHalfEdge[oppositeHalfEdge] = halfEdge;
    }
}



void IBHM::setElementVertex(const unsigned int he, const unsigned int vertex)
{
    assert(he < _halfEdgeVertex.size());

    _halfEdgeVertex[he] = vertex;
}



unsigned int IBHM::numberOfElements() const
{
    return _numberOfValidElements;
}



unsigned int IBHM::numberOfHalfEdges() const
{
    // The offset of the first element that does not exist yet is the number of half-edges of the valid elements.
    return _offset[_numberOfValidElements];
}



unsigned int IBHM::firstElementHE(const unsigned int elementIndex) const
{
    assert(elementIndex < _numberOfValidElements + 1);
    return _offset[elementIndex];
}



bool IBHM::isBorder(const unsigned int he) const
{
    // Rotate clockwise around the vertex of he (h <- next(O[h])), visiting its corners one by one, until a border edge
    // is found or the rotation is back to 'he'.
    unsigned int currentHE = he;
    bool border = false;
    do
    {
        const unsigned int oppositeHE = heOpposite(currentHE);

        // If it hits a border, stop.
        if (oppositeHE == IBHM::BORDER)
        {
            border = true;
            break;
        }

        // Go to the next corner around the vertex. A collapsed edge goes from the vertex to itself and has no opposite:
        // the next half-edge in the same interface element also starts at the vertex, so it is the next corner. Like
        // every other step, this one is compared with he by the loop condition. The former code jumped over this
        // comparison and never stopped when the rotation started right after a collapsed edge.
        if (oppositeHE == IBHM::COLLAPSED)
        {
            currentHE = heNext(currentHE);
        }
        else
        {
            currentHE = heNext(oppositeHE);
        }
    }
    while (currentHE != he);

    return border;
}



unsigned int IBHM::nextAvailableHE() const
{
    assert(numberOfElements() < _offset.size() - 1);

    return _offset[_numberOfValidElements];
}



unsigned int IBHM::numberReservedElements() const
{
    return static_cast<unsigned int>(_halfEdgeVertex.size());
}



unsigned int IBHM::numberCoordinatesByVertex() const
{
    return _numberCoordinatesPerVertex;
}



void IBHM::coordinatesFromHalfEge(const unsigned int halfEdge, double *coordinates) const
{
    for (unsigned int i = 0; i < _numberCoordinatesPerVertex; i++)
    {
        coordinates[i] = _coordinates[_numberCoordinatesPerVertex * _halfEdgeVertex[halfEdge] + i];
    }
}



void IBHM::coordinatesFromIndex(const unsigned int index, double *coordinates) const
{
    for (unsigned int i = 0; i < _numberCoordinatesPerVertex; i++)
    {
        coordinates[i] = _coordinates[_numberCoordinatesPerVertex * index + i];
    }
}
