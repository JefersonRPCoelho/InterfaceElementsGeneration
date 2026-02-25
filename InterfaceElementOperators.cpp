//
// Created by jcoelho on 2/24/2026.
//

#include "InterfaceElementOperators.h"

#include <assert.h>



InterfaceElementOperators::InterfaceElementOperators(CHE *che) : _che(che)
{
    // In the beginning, there is no interface element in the mesh.
    _inInterfaceElement.resize(_che->numberPoints(), false);
}



void InterfaceElementOperators::insertInterfaceElements(const std::vector<unsigned int> &edges)
{
    // Reserve space for the new elements. It assumes all edges are valid and one element will be insert for each edge.
    _che->reserveSpaceForElements(static_cast<unsigned int>(edges.size()));

    // Insert an element for each edge.
    for (auto &a: edges)
    {
        // C             D
        //  \  a -->    /
        //  A --------- B
        //  /    <-- b  \
        // E             F
        // Get the opposite half-edge from the same edge. The half-edges 'a' and 'b' represent the same edge.
        const unsigned int b = _che->heOpposite(a);

        // If the half-edge has no opposite, it is not valid, once it is not possible to open an edge.
        if (b == CHE::BORDER || b == CHE::COLLAPSED)
        {
            printf("The edge represent by the half-edge [%u]is not valid\n", a);
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
        const unsigned int h0 = _che->nextAvailableHE();
        const unsigned int h2 = h0 + 2;

        // Update the opposites for the middle edges.
        _che->_oppositeHalfEdge[h0] = b;
        _che->_oppositeHalfEdge[b] = h0;

        _che->_oppositeHalfEdge[h2] = a;
        _che->_oppositeHalfEdge[a] = h2;

        // Retrieve the operator for the vertex A.
        OperatorType operatorA = retrieveOperator(a);

        // Retrieve the operator for the vertex B.
        OperatorType operatorB = retrieveOperator(b);

        // Update the number of valid elements. Only update it after all updates are done.
        _che->_numberOfValidElements++;
    }
}



InterfaceElementOperators::OperatorType InterfaceElementOperators::retrieveOperator(const unsigned int he)
{
    auto op = OperatorType::UNDEFINED;

    // Get the edge vertices.
    const unsigned v = _che->heVertexIndex(he);

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
        const unsigned int availableVertex = retrieveAvailableVertex(he);

        if (availableVertex != CHE::BORDER)
        {
            op = OperatorType::SPLIT_ELEMENT;
        }
        else
        {
            op = OperatorType::EXPAND_EDGE;
        }
    }

    // If the vertex is part of an interface element, but has no vertex available (duplicated), it is the case of
    // opening a hole in the mesh.
    else
    {
        op = OperatorType::OPEN_HOLE;
    }

    return op;
}



unsigned int InterfaceElementOperators::retrieveAvailableVertex(const unsigned int he)
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

    assert(_che->heOpposite(he) != CHE::BORDER && _che->heOpposite(he) != CHE::COLLAPSED);

    // Get the opposite half-edge.
    const unsigned int b = _che->heOpposite(he);

    //Get the vertex index.
    const unsigned v = _che->heVertexIndex(he);

    // Get the half-edge from the collapsed edge.
    assert(_collapsed.find(v) != _collapsed.end());

    // The tests are only true when the edge being split and the interface element with the available vertex share an
    // element.

    // Test the first hypothesis.
    if (const unsigned int x1 = _che->heOpposite(_che->hePrevious(he)) != CHE::BORDER)
    {
        if (_collapsed[v] == _che->hePrevious(x1))
        {
            return x1;
        }
    }

    // Test the second hypothesis.
    if (const unsigned int x3 = _che->heOpposite(_che->heNext(b)) != CHE::BORDER)
    {
        const unsigned int x = _che->heNext(x3);
        if (_collapsed[v] == x)
        {
            return x;
        }
    }

    return CHE::BORDER;
}
