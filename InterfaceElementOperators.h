//
// Created by jcoelho on 2/24/2026.
//

#pragma once
#include <unordered_map>
#include <vector>
#include "CHE.h"



class InterfaceElementOperators
{
public:
    /**
     * Define the type of all available operators.
     */
    enum class OperatorType { CANONICAL, SPLIT_ELEMENT, EXPAND_EDGE, OPEN_HOLE, UNDEFINED };

public:
    /**
     * The operators modify a CHE instance. Therefore, the constructor should receive the instance that should be
     * modified.
     * @param che The CHE instance that should be modified.
     */
    explicit InterfaceElementOperators(CHE *che);

    void insertInterfaceElements(const std::vector<unsigned int> &edges);

private:
    /**
     * Perform the canonical operator in a vertex identified by the half-edge edgeHE.
     *
     * The edgeHE half-edge identifies the edge being split and the vertex where the canonical operator should be
     * applied.
     *
     * If the vertex is internal to the mesh, no new vertex is created. Instead, a collapsed edge is created in the
     * pre-existent vertex. This edge is saved to the _collapsed mesh with the half-edge identifying the collapsed edge.
     *
     * When the vertex is on the border, a new geometric vertex is created and the resulting edge is not collapsed
     * anymore. As a result, all elements from the new vertex side should be re-indexed to use the new vertex. See
     * @reindexElements. The opposite half-edge is marked as a border.
     *
     * All involved vertices are identified as part of an interface element. It is important to identify the right
     * operator to be used.
     *
     * @param edgeHE The half-edge that identifies the edge being split and the vertex where the operator should be
     * applied.
     * @param elementHE The half-edge that identifies the edge being created in the vertice inside the new interface
     * element. The operator supposes that the element is already created and will only work on adjusting the neighbor
     * around the vertex.
     * @return True if the operation was performed successfully and false otherwise.
     */
    bool canonical(unsigned int edgeHE, unsigned int elementHE);

    [[nodiscard]] bool splitElement(unsigned int he, unsigned int heVertex);

    [[nodiscard]] bool expandEdge(unsigned int he);

    [[nodiscard]] bool insertHole(unsigned int he);

    [[nodiscard]] OperatorType retrieveOperator(unsigned int he);

    /**
     * Given a half-edge he in a given vertex x, reindex all elements in the CCW direction with the new vertex v until
     * finding the half-edge stopHE. The algorithm will stop if it finds a border before finding stopHE.
     *
     * The half-edge stopHE can be provided as CHE::BORDER.
     *
     * If the stopHE is never found, the algorithm will replace the vertex from he by v for all elements.
     *
     * @param he A half-edge outgoing from the vertex that should be replaced by v.
     * @param stopHE The half-edge the algorithm should stop when finding it. It can be CHE::BORDER.
     * @param v The new vertex index that should be used to replace the vertex in he.
     */
    void reindexElements(unsigned int he, unsigned int stopHE, unsigned int v) const;

    /**
     * Try to retrieve the available vertex to be used in the new interface element. This function supposes:
     *    1. The vertex represented by the half-edge he is already known as a duplicated vertex, i.e., it is on the
     *    _collapsed map.
     *    2. The function is being used as a support to determine the operator type between SPLIT_ELEMENT and
     *    EXPAND_EDGE.
     *
     * The function will try to prove that the operator is SPLIT_ELEMENT. In case of failure, it is determined that the
     * operator is EXPAND_EDGE.
     *
     * @param he A half-edge from the edge being split that belongs to the vertex that should be tested.
     * @return The half-edge from the vertex that should be used in the SPLIT_ELEMENT operator, or CHE::BORDER if the
     * operator is EXPAND_EDGE.
     */
    [[nodiscard]] unsigned int retrieveAvailableVertex(unsigned int he);

private:
    /**
     * CHE instance that should be modified by the class operators.
     */
    CHE *_che = nullptr;

    /**
     * Determine if a vertex is on an interface element or not.
     */
    std::vector<bool> _inInterfaceElement;

    /**
     * Stores the current collapsed edges. The key is the vertex that is collapsed, and the value is the half-edge from
     * the first-edge vertex.
     *
     * @todo evaluate if we can get rid of it by getting this information on the fly.
     */
    std::unordered_map<unsigned int, unsigned int> _collapsed;
};

