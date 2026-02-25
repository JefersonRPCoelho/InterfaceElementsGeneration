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
    [[nodiscard]] bool canonical(unsigned int he);

    [[nodiscard]] bool splitElement(unsigned int he, unsigned int heVertex);

    [[nodiscard]] bool expandEdge(unsigned int he);

    [[nodiscard]] bool insertHole(unsigned int he);

    [[nodiscard]] OperatorType retrieveOperator(unsigned int he);

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

