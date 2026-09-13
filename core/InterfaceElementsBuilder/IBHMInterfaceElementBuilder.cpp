//
// Created by jcoelho on 2/24/2026.
//
#include <iostream>
#include <cassert>
#include "IBHMInterfaceElementBuilder.h"
#include "IBHM.h"



IBHMInterfaceElementBuilder::IBHMInterfaceElementBuilder(IBHM *ibhm) : _ibhm(ibhm)
{
    // In the beginning, there is no interface element in the mesh.
    _inInterfaceElement.resize(_ibhm->numberPoints(), false);
    _isInterfaceElement.resize(_ibhm->numberOfElements(), false);
}



void IBHMInterfaceElementBuilder::insertInterfaceElements(const std::vector<unsigned int> &edges)
{
    // Get the number of required new vertices and elements.
    const unsigned int numNewVertices = computeNumberOfNewVertices(edges);
    const unsigned int numNewElements = static_cast<unsigned int>(edges.size());

    // Reserve space for the new elements. It assumes all edges are valid and one element will be inserted for each edge.
    _ibhm->reserveSpaceForElements(4 * numNewElements, numNewElements);
    _isInterfaceElement.resize(_isInterfaceElement.size() + edges.size(), false);

    // Reserve space for new nodes. It assumes all edges are valid and one element will be inserted for each edge.
    _ibhm->reserveSpaceForNodes(numNewVertices);
    _inInterfaceElement.resize(_inInterfaceElement.size() + numNewVertices, false);

    // Insert an element for each edge.
    for (auto &a: edges)
    {
        // C             D
        //  \  a -->    /
        //  A --------- B
        //  /    <-- b  \
        // E             F
        // Get the opposite half-edge from the same edge. The half-edges 'a' and 'b' represent the same edge.
        const unsigned int b = _ibhm->heOpposite(a);

        // If the half-edge has no opposite, it is not valid, once it is not possible to open an edge.
        if (b == IBHM::BORDER || b == IBHM::COLLAPSED)
        {
            printf("The edge represent by the half-edge [%u]is not valid\n", a);
            continue;
        }

        if (_isInterfaceElement[_ibhm->heElement(a)] || _isInterfaceElement[_ibhm->heElement(b)])
        {
            printf("The edge represent by the half-edge [%u]is not valid because it is on an interface element\n", a);
            continue;
        }

        // Create a new element. All new interface elements will follow the template below. All the operators should
        // take it into consideration. Some key points:
        //     1. The middle edges are always represented by the half-edges h0 and h2.
        //     2. The edge that will 'split' the vertex A has its vertices defined by h3 -> h0, following the
        //     orientation order.
        //     3. The edge that will 'split' the vertex B has its vertices defined by h1 -> h2, following the
        //     orientation order.
        //     4. The new element is defined by FGHE.
        // E +--------------+ H
        //   | h3  <---   h2|
        //   |              |
        //   | h0  --->   h1|
        // F +--------------+ G

        // Get the interface element's half-edge.
        const unsigned int h0 = _ibhm->nextAvailableHE();
        const unsigned int h1 = h0 + 1;
        const unsigned int h2 = h0 + 2;
        const unsigned int h3 = h0 + 3;

        // Validate the new inserted element. @todo check for a better name.
        const unsigned int newElement = _ibhm->commitElement(4);

        // Variable used to determine the available vertex half-edge index for the SPLIT_ELEMENT operator.
        unsigned int availableVertexHE = IBHM::BORDER, sharedElementHE = IBHM::BORDER;

        // Retrieve the operator for the vertex A.
        switch (retrieveOperator(a, availableVertexHE, sharedElementHE))
        {
            case OperatorType::CANONICAL:
                canonical(a, h3);
                break;
            case OperatorType::SPLIT_ELEMENT:
                splitElement(a, availableVertexHE, sharedElementHE, h3);
                break;
            case OperatorType::EXPAND_EDGE:
                expandEdge(a, h3);
                break;
            case OperatorType::OPEN_HOLE:
                openHole(a, h3);
                break;
                printf("It was not possible to recognize the operator for the half-edge %d\n", a);
            case OperatorType::UNDEFINED:
                break;
        }

        // Retrieve the operator for the vertex B.
        switch (retrieveOperator(b, availableVertexHE, sharedElementHE))
        {
            case OperatorType::CANONICAL:
                canonical(b, h1);
                break;
            case OperatorType::SPLIT_ELEMENT:
                splitElement(b, availableVertexHE, sharedElementHE, h1);
                break;
            case OperatorType::EXPAND_EDGE:
                expandEdge(b, h1);
                break;
            case OperatorType::OPEN_HOLE:
                openHole(b, h1);
                break;
            case OperatorType::UNDEFINED:
                printf("It was not possible to recognize the operator for the half-edge %d\n", a);
                break;
        }

        // Update the opposites for the middle edges.
        _ibhm->setOpposite(h0, b);
        _ibhm->setOpposite(h2, a);

        // Label the element as interface element.
        _isInterfaceElement[newElement] = true;
    }
}



bool IBHMInterfaceElementBuilder::canonical(const unsigned int edgeHE, const unsigned int elementHE)
{
    // e = edgeHE.
    // x = elementHE
    // V +--------------+ V
    //   |     <---    x|e
    //   |      I       |
    //   |              |
    //   +--------------+

    // Get the vertex index.
    const unsigned int v = _ibhm->heVertexIndex(edgeHE);

    // The vertex should not be part of any interface element when using this operator.
    assert(_inInterfaceElement[v] == false);

    // If the vertex is on the border, a new geometry vertex should be created.
    if (_ibhm->isBorder(edgeHE))
    {
        // Get the index for the new vertex.
        const unsigned int newVertex = _ibhm->addPoint();

        // Add the element to the newly created element.
        _ibhm->setElementVertex(elementHE, newVertex);

        // Reindex all nodes with this vertex from the edge to the border.
        reindexElementsCCW(edgeHE, IBHM::BORDER, newVertex);

        // As the vertex is on the border, the created edge is also on the border.
        _ibhm->setOpposite(elementHE, IBHM::BORDER);

        // Label the new vertex as part of an interface element.
        _inInterfaceElement[newVertex] = true;
    }
    //If the vertex is internal, we should only duplicate it.
    else
    {
        // Duplicate the existing vertex for the new element.
        _ibhm->setElementVertex(elementHE, v);

        // In this case, the edge is collapsed, so the opposite is undefined.
        _ibhm->setOpposite(elementHE, IBHM::COLLAPSED);

        // Save the collapsed edge.
        _collapsed[v] = elementHE;
    }

    // Keep the former vertex in the element.
    _ibhm->setElementVertex(_ibhm->heNext(elementHE), v);


    // Label the vertex as part of an interface element.
    _inInterfaceElement[v] = true;

    return true;
}



bool IBHMInterfaceElementBuilder::splitElement(const unsigned int he, const unsigned int availableVertexHE,
                                               const unsigned int sharedElementHE, const unsigned int elementHE)
{
    // Get the vertex index.
    const unsigned int v = _ibhm->heVertexIndex(he);

    // Get the index for the new vertex.
    const unsigned int newVertex = _ibhm->addPoint();

    // Update the pre-existent interface element.
    _ibhm->setElementVertex(availableVertexHE, newVertex);


    // Update the continuous element.
    _ibhm->setElementVertex(sharedElementHE, newVertex);

    // Update the new interface element.
    if (he == sharedElementHE)
    {
        // This is the case where the half-edge he is inside the shared element. In this case the elementHE is used to
        // the new vertex, and the next vertice is kept with the former vertex to keep the right orientation.
        _ibhm->setElementVertex(elementHE, newVertex);
        _ibhm->setElementVertex(_ibhm->heNext(elementHE), v);

        // Update the opposites.
        _ibhm->setOpposite(elementHE, _ibhm->hePrevious(availableVertexHE));
    }
    else
    {
        // This is the case where the half-edge he is not part of the shared element. In this case the elementHE is used
        // to the former vertex, and the next vertice is kept with the new vertex to keep the right orientation.
        _ibhm->setElementVertex(elementHE, v);
        _ibhm->setElementVertex(_ibhm->heNext(elementHE), newVertex);

        // Update the opposites.
        _ibhm->setOpposite(elementHE, availableVertexHE);
    }

    // Label the vertex as part of an interface element.
    _inInterfaceElement[newVertex] = true;
    _inInterfaceElement[v] = true;

    // Remove collapse edge.
    _collapsed.erase(v);

    return true;
}



bool IBHMInterfaceElementBuilder::expandEdge(const unsigned int he, const unsigned int elementHE)
{
    // Get the vertex index.
    const unsigned int v = _ibhm->heVertexIndex(he);

    assert(_collapsed.find(v) != _collapsed.end());

    // Get the index for the new vertex.
    const unsigned int newVertex = _ibhm->addPoint();

    // Get the available vertex half-edge.
    const unsigned int availableVertexHE = _collapsed[v];

    // Reindex all the elements from the "left" side to the new vertex.
    reindexElementsCCW(availableVertexHE, he, newVertex);

    // Update the edge opposites.
    _ibhm->setOpposite(availableVertexHE, elementHE);

    // Update the new interface element.
    _ibhm->setElementVertex(elementHE, v);
    _ibhm->setElementVertex(_ibhm->heNext(elementHE), newVertex);

    // Label the vertex as part of an interface element.
    _inInterfaceElement[newVertex] = true;
    _inInterfaceElement[v] = true;

    // Remove collapse edge.
    _collapsed.erase(v);

    return true;
}



bool IBHMInterfaceElementBuilder::openHole(const unsigned int he, const unsigned int elementHE)
{
    // Get the vertex index.
    const unsigned int v = _ibhm->heVertexIndex(he);

    // Get the index for the new vertex.
    const unsigned int newVertex = _ibhm->addPoint();

    // Reindex with the new vertex.
    unsigned int currentHE = he;
    do
    {
        // Reindex the element to the new vertex.
        _ibhm->setElementVertex(currentHE, newVertex);

        // Get the opposite half-edge.
        currentHE = _ibhm->heOpposite(_ibhm->hePrevious(currentHE));
    }
    while (currentHE != IBHM::BORDER && !_isInterfaceElement[_ibhm->heElement(currentHE)]);

    // @todo Is it possible?
    assert(currentHE != IBHM::BORDER);

    // Update the interface element.
    _ibhm->setElementVertex(currentHE, newVertex);

    // Open a hole in the interface element.
    _ibhm->setOpposite(_ibhm->hePrevious(currentHE), IBHM::BORDER);

    // Look for the half-edge in the other side to create a hole.
    currentHE = he;
    do
    {
        // Get the opposite half-edge.
        currentHE = _ibhm->heNext(_ibhm->heOpposite(currentHE));
    }
    while (currentHE != IBHM::BORDER && !_isInterfaceElement[_ibhm->heElement(currentHE)]);

    // @todo Is it possible?
    assert(currentHE != IBHM::BORDER);

    // Open a hole in the interface element.
    _ibhm->setOpposite(currentHE, IBHM::BORDER);

    // Update the new interface element.
    _ibhm->setElementVertex(elementHE, newVertex);
    _ibhm->setElementVertex(_ibhm->heNext(elementHE), v);

    // Update the opposite for the edge.
    _ibhm->setOpposite(elementHE, IBHM::BORDER);

    // Label the vertex as part of an interface element.
    _inInterfaceElement[newVertex] = true;
    _inInterfaceElement[v] = true;

    return true;
}



IBHMInterfaceElementBuilder::OperatorType IBHMInterfaceElementBuilder::retrieveOperator(
    const unsigned int he, unsigned int &availableVertexHE, unsigned int &sharedElementHE)
{
    auto op = OperatorType::UNDEFINED;
    availableVertexHE = IBHM::BORDER;

    // Get the edge vertices.
    const unsigned v = _ibhm->heVertexIndex(he);

    // If the vertice is not part of any interface element yet, the canonical operator should be used.
    // C             D
    //  \           /
    //  A --------- B
    //  /           \
    // E             F
    if (!_inInterfaceElement[v])
    {
        op = OperatorType::CANONICAL;
    }

    // If there is an available vertex, it can be a case of splitting an element of expanding an edge.
    else if (_collapsed.find(v) != _collapsed.end())
    {
        // Try to retrieve the available vertex. If it is possible, we should use the SPLIT_ELEMENT operator.
        retrieveAvailableVertex(he, availableVertexHE, sharedElementHE);

        if (availableVertexHE != IBHM::BORDER)
        {
            op = OperatorType::SPLIT_ELEMENT;
        }
        else
        {
            op = OperatorType::EXPAND_EDGE;
        }
    }

    // If the vertex is part of an interface element but has no vertex available (duplicated), it is the case of
    // opening a hole in the mesh.
    else
    {
        op = OperatorType::OPEN_HOLE;
    }

    return op;
}



void IBHMInterfaceElementBuilder::reindexElementsCCW(const unsigned int he, const unsigned int stopHE,
                                                     const unsigned int v) const
{
    unsigned int currentHE = he;
    const unsigned int startHE = currentHE;
    do
    {
        // Reindex the element to the new vertex.
        _ibhm->setElementVertex(currentHE, v);

        // Get the opposite half-edge.
        currentHE = _ibhm->heOpposite(_ibhm->hePrevious(currentHE));
    }
    while (currentHE != stopHE && currentHE != IBHM::BORDER && currentHE != startHE);
}



void IBHMInterfaceElementBuilder::retrieveAvailableVertex(const unsigned int he, unsigned int &availableVertexHE,
                                                          unsigned int &sharedElementHE)
{
    // It is for sure that there is a vertex available at this point. It remains to decide the operation type. The
    // function will try to prove that it is the case of a SPLIT_ELEMENT, in the case of failure, it will be determined
    // that it is the EXPAND_EDGE case. If the function can prove it is the SPLIT_ELEMENT case, it will return the
    // half-edge index that should be used to reindex the elements.
    //
    // Given the he representing a or b from the edge A->B we will try to verify if the edge A->B shares a face with an
    // edge incoming or outgoing from one of the available vertices.
    //
    // In the image below, x represents the possible half-edges stored in the _collapsed map. X is always the start of
    // the collapsed edge.
    //
    // We should validate if one of the x for the vertex is in fact collapsed and, when validated, determine if there is
    // a common element between an edge incoming/outgoing the collapsed edge and the edge being split.
    //
    // If it is not possible to determine this edge, the operator EXPAND_EDGE should be used. Otherwise, it should also
    // be determined the half-edge that should be used to reindex the elements with the available vertex.
    //
    // From the half-edge he, x1 = opposite(previous(he)) and x = next(opposite(next(opposite(he)))).
    //
    // +------+--------------+------+
    // |      |              |      |
    // |  I   |              |   I  |
    // |x   x1|he  --->      |x     |
    // +------A--------------B------+
    // |     x|     <---    b|x2   x|
    // |  I   |              |   I  |
    // |    x3|              |      |
    // +------+--------------+------+
    //
    // About the opposites:
    //     1. Opposites from he and b will always exist at this point because it is the edge where the interface element
    //     is being inserted. The algorithm should ignore edge borders before this point.
    //     2. Once the interface element does exist, it is for sure possible to move from the continuous element to it
    //     by definition, once it was originated, split one of the continuos element edge. If the interface element does
    //     not exist, the opposite may not exist. However, these cases are not the interest of this function.
    //

    assert(_ibhm->heOpposite(he) != IBHM::BORDER && _ibhm->heOpposite(he) != IBHM::COLLAPSED);

    // Initialize output variables.
    availableVertexHE = sharedElementHE = IBHM::BORDER;

    // Get the opposite half-edge.
    const unsigned int b = _ibhm->heOpposite(he);

    //Get the vertex index.
    const unsigned v = _ibhm->heVertexIndex(he);

    // Get the half-edge from the collapsed edge.
    assert(_collapsed.find(v) != _collapsed.end());

    // The tests are only true when the edge being split and the interface element with the available vertex share an
    // element.

    // Test the first hypothesis.
    const unsigned int x1 = _ibhm->heOpposite(_ibhm->hePrevious(he));
    if (x1 != IBHM::BORDER)
    {
        if (_collapsed[v] == _ibhm->hePrevious(x1))
        {
            availableVertexHE = x1;
            sharedElementHE = he;
            return;
        }
    }

    // Test the second hypothesis.
    const unsigned int x3 = _ibhm->heOpposite(_ibhm->heNext(b));
    if (x3 != IBHM::BORDER)
    {
        const unsigned int x = _ibhm->heNext(x3);
        if (_collapsed[v] == x)
        {
            availableVertexHE = x;
            sharedElementHE = _ibhm->heNext(b);
        }
    }
}



unsigned int IBHMInterfaceElementBuilder::computeNumberOfNewVertices(const std::vector<unsigned int> &edges) const
{
    std::unordered_map<unsigned int, unsigned int> vertexCount;
    for (const auto &a: edges)
    {
        // Get the next vertex half-edge.
        const unsigned int b = _ibhm->heNext(a);

        // Get both edges vertices;
        const unsigned int v1 = _ibhm->heVertexIndex(a);
        const unsigned int v2 = _ibhm->heVertexIndex(b);

        // Count the number of times the vertex appears.
        if (vertexCount.find(v1) == vertexCount.end())
        {
            vertexCount[v1] = 1;
        }
        else
        {
            vertexCount[v1]++;
        }

        // If the vertex is not part of an interface element, we will need an extra vertex in the border.
        if (_ibhm->isBorder(a) && !_inInterfaceElement[v1])
        {
            vertexCount[v1]++;
        }

        if (vertexCount.find(v2) == vertexCount.end())
        {
            vertexCount[v2] = 1;
        }
        else
        {
            vertexCount[v2]++;
        }

        if (_ibhm->isBorder(b) && !_inInterfaceElement[v2])
        {
            vertexCount[v2]++;
        }
    }

    unsigned int numberOfNewVertices = 0;
    for (const auto &[vertex, count]: vertexCount)
    {
        if (_inInterfaceElement[vertex] == false)
        {
            // If the vertice is not part of an interface element, we need n - 1 vertex. The vertex itself will be
            // reused. For the vertices on the border, it was already added an extra count, so it is correct to use the
            // same formula.
            numberOfNewVertices += (count - 1);
        }
        else
        {
            // Once the vertex is already part of an interface element, each new edge will generate a new vertex.
            numberOfNewVertices += count;
        }
    }

    return numberOfNewVertices;
}
