/* 
 * File:   CHEOperations.cpp
 * Author: jeferson
 * 
 * Created on 4 de Agosto de 2012, 16:55
 */

#include "CHEOperations.h"
#include <iostream>
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



bool CHEOperations::isBorder(const unsigned int he) const
{
    unsigned int currentHE = he;
    bool border = false;
    //Turn around the vertex until get the starting point or hit a boundary.
    do
    {
        // Go to the next face.
        unsigned int oppositeHE = _che->heOpposite(currentHE);

        // If the edge is collapsed, skip it.
        if (oppositeHE == CHE::COLLAPSED)
        {
            currentHE = _che->heNext(currentHE);
            oppositeHE = _che->heOpposite(currentHE);
        }

        // If it hits a border, stop.
        if (oppositeHE == CHE::BORDER)
        {
            border = true;
            break;
        }

        // Get the next half-edge. Basically, it goes back to a half-edge emanating from the vertex.
        currentHE = _che->heNext(oppositeHE);
    }
    while (currentHE != he);

    return border;
}



std::vector<unsigned int> CHEOperations::getInterfaceElements(const unsigned int he) const
{
    std::vector<unsigned int> interfaceElements;
    unsigned int currentHE = he;

    //Turn around the vertex until get the starting point or hit a boundary.
    do
    {
        // Go to the next face.
        unsigned int oppositeHE = _che->heOpposite(currentHE);

        // If the edge is collapsed, skip it.
        if (oppositeHE == CHE::COLLAPSED)
        {
            currentHE = _che->heNext(currentHE);
            oppositeHE = _che->heOpposite(currentHE);
        }

        const unsigned int element = currentHE /_che->numberVertexByElement();
        if (_che->isInterfaceElement(element))
        {
            // Save it to the neighbor list.
            interfaceElements.push_back(element);
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
        return interfaceElements;

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
        const unsigned int element = currentHE /_che->numberVertexByElement();
        if (_che->isInterfaceElement(element))
        {
            // Save it to the neighbor list.
            interfaceElements.push_back(element);
        }

        // Go to the next face.
        currentHE = _che->heOpposite(hePrevious);
    }
    while (currentHE != CHE::BORDER);

    return interfaceElements;
}



void CHEOperations::addInterfaceElements(const std::vector<unsigned int> &edges)
{
    _che->reserveSpaceForElements(static_cast<unsigned int>(edges.size()));

    // For each edge, add a quad4 element.
    for (const unsigned int he: edges)
    {
        // Get the edge vertices.
        insertElement(he);
        _che->print();
        std::cout << std::endl;
    }
}

unsigned int CHEOperations::getAvailableVertex(const std::vector<unsigned int> &interfaceElements, const unsigned int he)
{
    const unsigned int f1 = _che->halfEdgeElement(he);
    const unsigned int f2 = _che->halfEdgeElement(_che->heOpposite(he));
    unsigned int nodeTobeReused = CHE::BORDER;

    for (const unsigned int f : interfaceElements)
    {
        const unsigned int he1 = f * _che->numberVertexByElement();
        unsigned int ehe = he1;
        do
        {
            const unsigned int opposite = _che->heOpposite(ehe);
            if (opposite != CHE::BORDER && opposite != CHE::COLLAPSED && _che->heVertexIndex(ehe) == _che->heVertexIndex(_che->hePrevious(ehe)))
            {
                const unsigned int oppositeElement = _che->halfEdgeElement(opposite);
                if (oppositeElement == f1 || oppositeElement == f2)
                {
                    nodeTobeReused = ehe;
                }
            }
            ehe = _che->heNext(ehe);
        }while (ehe != he1 && nodeTobeReused == CHE::BORDER);

        if (nodeTobeReused != CHE::BORDER)
        {
            break;;
        }
    }
    return nodeTobeReused;
}


bool CHEOperations::insertElement(const unsigned int he)
{
    // Check the edge validity.
    if (_che->heOpposite(he) == CHE::BORDER || _che->heOpposite(he) == CHE::COLLAPSED)
    {
        return false;
    }

    // Get the edge vertices.
    const unsigned int vertexA = _che->heVertexIndex(he);
    const unsigned int vertexB = _che->heVertexIndex(_che->heNext(he));

    // Test for OP1. None of the vertices are neighbored to an interface element, so the edge should only be opened as a
    // preparation for the next operations.
    if (!_che->_inInterfaceElement[vertexA] && !_che->_inInterfaceElement[vertexB])
    {
        return op1(he);
    }
    if (_che->_inInterfaceElement[vertexA] && !_che->_inInterfaceElement[vertexB])
    {
        auto interfaceElements = getInterfaceElements(he);
        const unsigned int vertexTobeReused = getAvailableVertex(interfaceElements, he);
        return op2(_che->heOpposite(he), vertexTobeReused);
    }
    if (!_che->_inInterfaceElement[vertexA] && _che->_inInterfaceElement[vertexB])
    {
        const auto interfaceElements = getInterfaceElements(_che->heNext(he));
        const unsigned int vertexTobeReused = getAvailableVertex(interfaceElements, he);
        return op2(he, vertexTobeReused);

    }
    if (_che->_inInterfaceElement[vertexA] && _che->_inInterfaceElement[vertexB])
    {
        printf("Error: trying to open an already open edge: (%u, %u)\n", vertexA, vertexB);
    }

    return true;
}



bool CHEOperations::op1(const unsigned int he)
{
    // Get the edge vertices.
    const unsigned int vertexA = _che->heVertexIndex(he);
    const unsigned int vertexB = _che->heVertexIndex(_che->heNext(he));

    // Get the key half-edges
    const unsigned int op = _che->heOpposite(he);

    // It is not possible to open an edge in the border.
    if (op == CHE::BORDER)
    {
        return false;
    }

    // Get the vertex indexes for the new element. By default, it is assumed that the edge is inside the mesh, so no
    // vertex duplication in the geometry vertex is necessary.
    const unsigned int v1 = vertexA;
    unsigned int v2 = vertexB;
    const unsigned int v3 = vertexB;
    unsigned int v4 = vertexA;

    // Check if the vertexes are in the border.
    const bool isVertexABorder = isBorder(he);
    const bool isVertexBBorder = isBorder(_che->heNext(he));

    // Duplicate the vertex A if necessary.
    if (isVertexABorder)
    {
        const unsigned int newVertex = _che->numberPoints();
        _che->reserveSpaceForNodes(1);
        v4 = newVertex;
        _che->_halfEdgeVertex[he] = newVertex;
    }

    // Duplicate the vertex B if necessary.
    if (isVertexBBorder)
    {
        const unsigned int newVertex = _che->numberPoints();
        _che->reserveSpaceForNodes(1);
        v2 = newVertex;
        _che->_halfEdgeVertex[op] = newVertex;
    }

    // Next free position in the elements' vector.
    const unsigned int availableHE = _che->numberOfElements() * _che->numberVertexByElement();

    // Alias to the new element corners.
    const unsigned int a = availableHE + 0;
    const unsigned int b = availableHE + 1;
    const unsigned int c = availableHE + 2;
    const unsigned int d = availableHE + 3;

    // Add the new element.
    _che->_halfEdgeVertex[a] = v1;
    _che->_halfEdgeVertex[b] = v2;
    _che->_halfEdgeVertex[c] = v3;
    _che->_halfEdgeVertex[d] = v4;

    // Label the element.
    _che->_isInterfaceElement[_che->_numberOfValidElements] = true;

    // Update the number of valid elements.
    _che->_numberOfValidElements++;

    // Update the opposites.
    _che->_oppositeHalfEdge[a] = op;
    _che->_oppositeHalfEdge[op] = a;

    // If the vertex A is in a border, the vertex is duplicated and the opposite is a border. Otherwise, the edge is
    // collapsed.
    if (isVertexBBorder)
    {
        _che->_oppositeHalfEdge[b] = CHE::BORDER;
    }
    else
    {
        _che->_oppositeHalfEdge[b] = CHE::COLLAPSED;
    }

    // Same for the vertex B.
    if (isVertexABorder)
    {
        _che->_oppositeHalfEdge[d] = CHE::BORDER;
    }
    else
    {
        _che->_oppositeHalfEdge[d] = CHE::COLLAPSED;
    }
    _che->_oppositeHalfEdge[c] = he;
    _che->_oppositeHalfEdge[he] = c;

    // Update the vertices' status.
    _che->_inInterfaceElement[v1] = true;
    _che->_inInterfaceElement[v2] = true;
    _che->_inInterfaceElement[v3] = true;
    _che->_inInterfaceElement[v4] = true;

    return true;
}


bool CHEOperations::op2(const unsigned int he, const unsigned int heVertex)
{
    // Check the edge validity.
    const unsigned int opposite = _che->heOpposite(he);
    if (opposite == CHE::BORDER || opposite == CHE::COLLAPSED)
    {
        return false;
    }

    // Get the edge vertices.
    const unsigned int vertexA = _che->heVertexIndex(he);
    const unsigned int vertexB = _che->heVertexIndex(_che->heNext(he));

    // Create a new vertex.
    const unsigned int newVertex = _che->numberPoints();
    _che->reserveSpaceForNodes(1);

    // Update the former interface element with the new vertex.
    _che->_halfEdgeVertex[heVertex] = newVertex;

    // Update the neighbor face with the new vertex.
    const unsigned int neighbourHe = _che->heNext(_che->heOpposite(heVertex));
    _che->_halfEdgeVertex[neighbourHe] = newVertex;

    // Get the vertex indexes for the new element. By default, it is assumed that the edge is inside the mesh, so no
    // vertex duplication in the geometry vertex is necessary.
    unsigned int v1 = vertexA;
    const unsigned int v2 = newVertex;
    const unsigned int v3 = vertexB;
    const unsigned int v4 = vertexA;

    const bool isVertexABorder = isBorder(he);

    // Duplicate the vertex A if necessary.
    if (isVertexABorder)
    {
        const unsigned int borderVertex = _che->numberPoints();
        _che->reserveSpaceForNodes(1);
        v1 = borderVertex;

        // In the case of a border, also change the already existent face to use the created border vertex.
        _che->_halfEdgeVertex[_che->heNext(neighbourHe)] = borderVertex;
    }

    // Next free position in the elements' vector.
    const unsigned int availableHE = _che->numberOfElements() * _che->numberVertexByElement();

    // Alias to the new element corners.
    const unsigned int a = availableHE + 0;
    const unsigned int b = availableHE + 1;
    const unsigned int c = availableHE + 2;
    const unsigned int d = availableHE + 3;

    // Add the new element.
    _che->_halfEdgeVertex[a] = v1;
    _che->_halfEdgeVertex[b] = v2;
    _che->_halfEdgeVertex[c] = v3;
    _che->_halfEdgeVertex[d] = v4;

    // Label the element.
    _che->_isInterfaceElement[_che->_numberOfValidElements] = true;

    // Update the number of valid elements.
    _che->_numberOfValidElements++;

    // Update the opposites.
    // If the vertex A is in a border, the vertex is duplicated and the opposite is a border. Otherwise, the edge is
    // collapsed.
    _che->_oppositeHalfEdge[c] = he;
    _che->_oppositeHalfEdge[he] = c;

    _che->_oppositeHalfEdge[b] = _che->hePrevious(heVertex);
    _che->_oppositeHalfEdge[_che->hePrevious(heVertex)] = b;

    _che->_oppositeHalfEdge[a] = opposite;
    _che->_oppositeHalfEdge[opposite] = a;

    if (isVertexABorder)
    {
        _che->_oppositeHalfEdge[d] = CHE::BORDER;
    }
    else
    {
        _che->_oppositeHalfEdge[d] = CHE::COLLAPSED;
    }

    // Update the vertices' status.
    _che->_inInterfaceElement[v1] = true;
    _che->_inInterfaceElement[v2] = true;
    _che->_inInterfaceElement[v3] = true;
    _che->_inInterfaceElement[v4] = true;

    return true;
}




