//
// Created by jcoelho on 2/24/2026.
//
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
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
    // Number of half-edges of the mesh before this call. Only smaller indices are accepted as input: the larger ones
    // are outside the element's list or in its reserved space, where the positions are not edges of the mesh. It must
    // be read here, since the elements committed by this call increase it.
    const unsigned int numberOfHalfEdges = _ibhm->numberOfHalfEdges();

    // Get the number of required new vertices and elements.
    const unsigned int numNewVertices = computeNumberOfNewVertices(edges);
    const unsigned int numNewElements = static_cast<unsigned int>(edges.size());

    // Reserve space for the new elements, as if all edges were valid: one element (4 half-edges) for each edge. Only
    // what is missing is added, and the vectors grow exactly (see IBHM::reserveSpaceForElements()). The same policy is
    // used for the interface element flags: one flag for each valid element plus one for each new element.
    _ibhm->reserveSpaceForElements(4 * numNewElements, numNewElements);
    const size_t numberOfElementFlags = static_cast<size_t>(_ibhm->numberOfElements()) + numNewElements;
    if (numberOfElementFlags > _isInterfaceElement.size())
    {
        _isInterfaceElement.reserve(numberOfElementFlags);
        _isInterfaceElement.resize(numberOfElementFlags, false);
    }

    // Reserve space for the new nodes, exactly (see IBHM::reserveSpaceForNodes()), and one flag for each of them.
    _ibhm->reserveSpaceForNodes(numNewVertices);
    const size_t numberOfVertexFlags = static_cast<size_t>(_ibhm->numberPoints()) + numNewVertices;
    if (numberOfVertexFlags > _inInterfaceElement.size())
    {
        _inInterfaceElement.reserve(numberOfVertexFlags);
        _inInterfaceElement.resize(numberOfVertexFlags, false);
    }

    // Insert an element for each edge.
    for (auto &a: edges)
    {
        // C             D
        //  \  a -->    /
        //  A --------- B
        //  /    <-- b  \
        // E             F

        // The index must be a half-edge of the mesh. This test comes first, since the queries below are only valid for
        // half-edges of committed elements.
        if (a >= numberOfHalfEdges)
        {
            printf("The half-edge [%u] is not valid because it is outside the range of the mesh half-edges [0, %u)\n",
                   a, numberOfHalfEdges);
            continue;
        }

        // Get the opposite half-edge from the same edge. The half-edges 'a' and 'b' represent the same edge.
        const unsigned int b = _ibhm->heOpposite(a);

        // If the half-edge has no opposite, it is not valid, once it is not possible to open an edge.
        if (b == IBHM::BORDER || b == IBHM::COLLAPSED)
        {
            printf("The edge represent by the half-edge [%u] is not valid\n", a);
            continue;
        }

        if (_isInterfaceElement[_ibhm->heElement(a)] || _isInterfaceElement[_ibhm->heElement(b)])
        {
            printf("The edge represent by the half-edge [%u] is not valid because it is on an interface element\n", a);
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

        // Retrieve and apply the operator for the vertex A. The architecture guarantees that the chosen operator is
        // applicable, so an operator that returns false, or an undefined operator, is a defect: it stops at the
        // assertion below (B10). The operators return true in the current implementation; when they report failures,
        // the assertion will catch them.
        [[maybe_unused]] bool appliedA = false;
        switch (retrieveOperator(a, availableVertexHE, sharedElementHE))
        {
            case OperatorType::CANONICAL:
                appliedA = canonical(a, h3);
                break;
            case OperatorType::SPLIT_ELEMENT:
                appliedA = splitElement(a, availableVertexHE, sharedElementHE, h3);
                break;
            case OperatorType::EXPAND_EDGE:
                appliedA = expandEdge(a, h3);
                break;
            case OperatorType::OPEN_HOLE:
                appliedA = openHole(a, h3);
                break;
            case OperatorType::UNDEFINED:
            default:
                printf("It was not possible to recognize the operator for the half-edge %u\n", a);
                break;
        }
        assert(appliedA);

        // Retrieve and apply the operator for the vertex B (same contract as for A).
        [[maybe_unused]] bool appliedB = false;
        switch (retrieveOperator(b, availableVertexHE, sharedElementHE))
        {
            case OperatorType::CANONICAL:
                appliedB = canonical(b, h1);
                break;
            case OperatorType::SPLIT_ELEMENT:
                appliedB = splitElement(b, availableVertexHE, sharedElementHE, h1);
                break;
            case OperatorType::EXPAND_EDGE:
                appliedB = expandEdge(b, h1);
                break;
            case OperatorType::OPEN_HOLE:
                appliedB = openHole(b, h1);
                break;
            case OperatorType::UNDEFINED:
            default:
                printf("It was not possible to recognize the operator for the half-edge %u\n", b);
                break;
        }
        assert(appliedB);

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
        // Create the new vertex as a copy of v, i.e., with the same coordinates.
        const unsigned int newVertex = _ibhm->duplicatePoint(v);

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

    // Create the new vertex as a copy of v, i.e., with the same coordinates.
    const unsigned int newVertex = _ibhm->duplicatePoint(v);

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

    // Create the new vertex as a copy of v, i.e., with the same coordinates.
    const unsigned int newVertex = _ibhm->duplicatePoint(v);

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

    // Create the new vertex as a copy of v, i.e., with the same coordinates.
    const unsigned int newVertex = _ibhm->duplicatePoint(v);

    // The corners of v form a single fan, which is split by the new edge (he). The operator moves to the new vertex
    // the sector that goes from he, counterclockwise, to the next "wall", and unsticks the short sides of the interface
    // elements that bound the opening on both sides of he. There are two kinds of wall:
    //     1. An interface element: always the case for an internal vertex, whose fan is a cycle. The short sides of the
    //     interface elements found on both sides are unstuck (opposite = BORDER), which opens or increases the hole.
    //     2. The mesh border (outer border or border of a hole in the mesh): only for a border vertex, whose fan is a
    //     path from border to border. The rotation reaches the border before finding an interface element, so there
    //     is no interface element corner to move nor short side to unstick on that side: the hole is open to the
    //     exterior of the mesh. In this case, that side behaves as the canonical operator on the border, which also
    //     moves the elements from the edge to the border to the new vertex.
    // For example, a border vertex whose first edge was opened by the canonical operator has an interface element on
    // one side of the second edge and the border on the other side: the sector between the second edge and the border
    // gets the new vertex. With k edges, a border vertex ends with k + 1 copies, which is the count of
    // computeNumberOfNewVertices().
    //
    // In all cases, the short side of the new interface element is a border (see below).

    // Loop 1: rotate counterclockwise from he, moving each corner to the new vertex, until the first interface element
    // (J) or the border.
    unsigned int currentHE = he;
    do
    {
        // Reindex the element to the new vertex.
        _ibhm->setElementVertex(currentHE, newVertex);

        // Go to the next element counterclockwise. It is BORDER if the edge before the corner is on the mesh border.
        currentHE = _ibhm->heOpposite(_ibhm->hePrevious(currentHE));

        // The edge before the corner is an edge of a continuum element (the loop stops at the first interface element),
        // so it is never collapsed.
        assert(currentHE != IBHM::COLLAPSED);
    }
    while (currentHE != IBHM::BORDER && !_isInterfaceElement[_ibhm->heElement(currentHE)]);

    // Wall 1 (interface element J): its corner at v is also moved to the new vertex, and the short side of J that
    // arrives at this corner is unstuck (it is already a border if v is a border vertex).
    // Wall 2 (border): nothing else to do on this side.
    const bool foundCCW = currentHE != IBHM::BORDER;
    if (foundCCW)
    {
        _ibhm->setElementVertex(currentHE, newVertex);
        _ibhm->setOpposite(_ibhm->hePrevious(currentHE), IBHM::BORDER);
    }

    // Loop 2: rotate clockwise from he, without changing any corner, until the first interface element (K) or the
    // border. The opposite must be tested before next() is applied: next(BORDER) is not a valid query (in the IBHM, it
    // is a binary search with an invalid index).
    currentHE = he;
    do
    {
        // Go to the next element clockwise, or stop if the edge of the current corner is on the mesh border. As in
        // loop 1, the edge is an edge of a continuum element, so it is never collapsed.
        const unsigned int oppositeHE = _ibhm->heOpposite(currentHE);

        assert(oppositeHE != IBHM::COLLAPSED);

        currentHE = oppositeHE == IBHM::BORDER ? oppositeHE : _ibhm->heNext(oppositeHE);
    }
    while (currentHE != IBHM::BORDER && !_isInterfaceElement[_ibhm->heElement(currentHE)]);

    // Wall 1 (interface element K): the short side of K that leaves v is unstuck (it is already a border if v is a
    // border vertex).
    // Wall 2 (border): nothing to do on this side.
    const bool foundCW = currentHE != IBHM::BORDER;
    if (foundCW)
    {
        _ibhm->setOpposite(currentHE, IBHM::BORDER);
    }

    // The vertex belongs to an interface element and its corners form a single fan, so an interface element corner is
    // always found on at least one side. If none is found, the mesh or the builder state is inconsistent.
    assert(foundCCW || foundCW);

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

        // Precondition (see the header): the rotation never crosses a collapsed edge, so COLLAPSED is never used as a
        // half-edge in the next iteration.
        assert(currentHE != IBHM::COLLAPSED);
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

    // The half-edges he and b belong to continuum elements (the edges of interface elements are rejected before the
    // operators), and an edge of a continuum element is never collapsed: only the short sides of interface elements
    // are. So x1 and x3 are half-edges or BORDER, never COLLAPSED, and the BORDER test is enough before next() and
    // previous() are applied to them.

    // Test the first hypothesis.
    const unsigned int x1 = _ibhm->heOpposite(_ibhm->hePrevious(he));
    assert(x1 != IBHM::COLLAPSED);
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
    assert(x3 != IBHM::COLLAPSED);
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
    // The count is done in two steps:
    //     1. Select the edges that insertInterfaceElements() will really open and, for each vertex, count how many of
    //     them touch it.
    //     2. Apply, once per vertex, the rule of the vertex state.
    //
    // Counting per vertex (and not per edge) gives the exact result when a vertex is touched by several edges of the
    // same call, and the border test, which rotates around the vertex, runs only once per vertex.

    // Information gathered for each vertex touched by the selected edges.
    struct VertexInfo
    {
        // Number of selected edges that touch the vertex.
        unsigned int count = 0;

        // One half-edge outgoing from the vertex. It is used to test if the vertex is on the border.
        unsigned int he = IBHM::BORDER;
    };
    std::unordered_map<unsigned int, VertexInfo> vertices;

    // Edges already selected, each one identified by the smaller of its two half-edges. It detects an edge provided
    // twice, also when it is provided once by each of its half-edges.
    std::unordered_set<unsigned int> selectedEdges;

    // Number of half-edges of the mesh. It is the same limit insertInterfaceElements() reads before calling this
    // function.
    const unsigned int numberOfHalfEdges = _ibhm->numberOfHalfEdges();

    // Step 1: select the edges and count them for their two vertices.
    for (const auto &a: edges)
    {
        // Skip the edges that insertInterfaceElements() skips, using the same tests. The result of these tests does not
        // change while the edges of this call are inserted, except for a repeated edge, which is handled by the last
        // test.

        // 0. An index that is not a half-edge of the mesh is skipped. It must be the first test, since the queries
        // below are only valid for half-edges of committed elements.
        if (a >= numberOfHalfEdges)
        {
            continue;
        }

        // C             D
        //  \  a -->    /
        //  A --------- B
        //  /    <-- b  \
        // E             F
        // The half-edges 'a' and 'b' represent the same edge: 'a' starts at A and 'b' starts at B.
        const unsigned int b = _ibhm->heOpposite(a);

        // 1. A border edge has no opposite, so it cannot be opened.
        if (b == IBHM::BORDER || b == IBHM::COLLAPSED)
        {
            continue;
        }

        // 2. An edge of an interface element cannot be opened. As this test comes before isBorder(), the border test
        // is never applied to a half-edge of an interface element.
        if (_isInterfaceElement[_ibhm->heElement(a)] || _isInterfaceElement[_ibhm->heElement(b)])
        {
            continue;
        }

        // 3. The insertion skips a repeated edge: after the first time, it already belongs to an interface element.
        // The insert() returns false in 'second' when the edge was already in the set.
        if (!selectedEdges.insert(std::min(a, b)).second)
        {
            continue;
        }

        // The edge will be opened: count it for the vertex A (origin of 'a') and for the vertex B (origin of 'b').
        for (const unsigned int he: {a, b})
        {
            // If the element does not exist in the map yet, the operator [] creates it.
            VertexInfo &info = vertices[_ibhm->heVertexIndex(he)];
            info.count++;
            info.he = he;
        }
    }

    // Step 2: apply the rule of each vertex. In the comments, n is the number of selected edges that touch the vertex.
    unsigned int numberOfNewVertices = 0;
    for (const auto &[vertex, info]: vertices)
    {
        if (_inInterfaceElement[vertex])
        {
            // The vertex is already part of an interface element: every new edge duplicates it (Split Element, Expand
            // Edge, or Open Hole), so it needs n new vertices.
            numberOfNewVertices += info.count;
        }
        else if (_ibhm->isBorder(info.he))
        {
            // The vertex is on the border and is not part of an interface element yet: the first edge already
            // duplicates it (Canonical on the border), and so does every other edge, so it needs n new vertices.
            numberOfNewVertices += info.count;
        }
        else
        {
            // The vertex is internal and is not part of an interface element yet: the first edge only creates a
            // collapsed edge (Canonical), without a new vertex, and every other edge duplicates it, so it needs n - 1
            // new vertices.
            numberOfNewVertices += info.count - 1;
        }
    }

    return numberOfNewVertices;
}
