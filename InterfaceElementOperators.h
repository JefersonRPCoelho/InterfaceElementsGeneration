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

    /**
     * Insert interface elements in a group of provided edges.
     * @param edges A vector with one half-edge for each input edge. Each edge is defined for two half-edges. There user
     * can select anyone of them.
     */
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

    /**
     * Perform the Split Element operator in a vertex identified by the half-edge he.
     *
     * This operator is always applied in a vertex with a collapsed edge. The interface element with this collapsed
     * edge should have an edge in the element of he.
     *
     * The half-edge availableVertexHE identifies one vertex of the collapsed edge that should be used to insert the new
     * vertex in the shared element.
     *
     * The half-edge sharedElementHE should identify the available vertex in the shared element.
     *
     * The elementHE, as in the canonical operator, should represent the first edge-vertex being opened. First, in the
     * sense of orientation.
     * @param he A half-edge identifying the vertex where the operator should be applied. This vertex should be part of
     * a collapsed edge.
     * 
     * @param availableVertexHE A half-edge representing the selected vertex to replaced by a geometric vertex in the
     * pre-existent interface element. The interface element that generated the collapsed edge.
     * @param sharedElementHE A half-edge representing the available vertex in the shared element.
    * @param elementHE The half-edge that identifies the edge being created in the vertice inside the new interface
     * element. The operator supposes that the element is already created and will only work on adjusting the neighbor
     * around the vertex.
     * @return True if the operation was performed successfully and false otherwise.
     */
    bool splitElement(unsigned int he, unsigned int availableVertexHE, unsigned int sharedElementHE,
                      unsigned int elementHE);

    /**
     * Perform the Expand Edge operator in a vertex identified by the half-edge he.
     *
     * This operator is always applied in a vertex with a collapsed edge. Unlike the Split Element operator, the Expand
     * Edge should be used WHEN IT IS NOT POSSIBLE to determine a common element between an edge from the interface
     * element with the collapsed edge and the edge being split.
     *
     * The elementHE, as in the canonical operator, should represent the first edge-vertex being opened. First, in the
     * sense of orientation.
     *
     * @param he A half-edge identifying the vertex where the operator should be applied. This vertex should be part of
     * a collapsed edge.
     * @param elementHE The half-edge that identifies the edge being created in the vertice inside the new interface
     * element. The operator supposes that the element is already created and will only work on adjusting the neighbor
     * around the vertex.
     * @return True if the operation was performed successfully and false otherwise.
     */
    bool expandEdge(unsigned int he, unsigned int elementHE);

    /**
     * Perform the Open Hole operator in a vertex identified by the half-edge he.
     *
     * This operator should be applied in vertices that are already part of an interface element and are not collapsed,
     * i.e., there is no available vertex. In these cases, the vertex is duplicated and one side of the original
     * vertex has all the elements updated with the new vertex. The stop condition is when an interface element is
     * found. The other side should be traversed to update the opposite of the first edge in an interface element to
     * BORDER. Doing it from both sides will open or increase the hole.
     *
     * The elementHE, as in the canonical operator, should represent the first edge-vertex being opened. First, in the
     * sense of orientation.
     *
     * @param he A half-edge identifying the vertex where the operator should be applied. This vertex should be part of
     * a collapsed edge.
     * @param elementHE The half-edge that identifies the edge being created in the vertice inside the new interface
     * element. The operator supposes that the element is already created and will only work on adjusting the neighbor
     * around the vertex.
     * @return True if the operation was performed successfully and false otherwise.
     */
    bool openHole(unsigned int he, unsigned int elementHE);

    /**
     * Given a half-edge defining an edge, determine which operator should be used in the half-edge's vertex.
     * @param he A half-edge defining an edge and the vertex where the operator should be applied.
     * @param availableVertexHE For SPLIT_ELEMENT only: when the SPLIT_ELEMENT operator is selected, the half-edge for
     * one of the vertices in the collapsed edge is selected to be used by the operator and saved in this variable. This
     * half-edge is used to reindex the element with a newly created vertex.
     * @param sharedElementHE For SPLIT_ELEMENT only: when the SPLIT_ELEMENT operator is selected, the half-edge for the
    * selected vertex in the shared element is selected to be used by the operator and saved in this variable. This
     * half-edge is used to reindex the shared element with a newly created vertex.
     * @return The retrieved operator.
     */
    [[nodiscard]] OperatorType retrieveOperator(unsigned int he, unsigned int &availableVertexHE,
                                                unsigned int &sharedElementHE);

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
    void reindexElementsCCW(unsigned int he, unsigned int stopHE, unsigned int v) const;


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
     * @param availableVertexHE If the operation is defined as SPLIT_ELEMENT, the function will write in this variable
    * the half-edge index for the available vertex to be used in the operator or CHE::BORDER if the operator is
    * EXPAND_EDGE.
    * @param sharedElementHE The half-edge for the node that should be index inside the shared face or CHE::BORDER if
    * the operator is EXPAND_EDGE.
     */
    void retrieveAvailableVertex(unsigned int he, unsigned int &availableVertexHE, unsigned int &sharedElementHE);

public:
    /**
     * Compute the number of new vertices will be required to insert interface elements in the provided edges.
     *
     * The current implementation uses the following rules to determine the number of new vertices:
     *     1. As a general rule, from the start, each vertex will need n - 1 new vertices if it is not in the border or
     *     n if it is in the border, where n is the vertex valence in terms of selected edges.
     *     2. If the vertex is already part of an interface element, the number of required new vertices is n.
     *
     * The first time a vertex is split, it generates a collapsed edge, which does not require a new vertex. From that
     * on, every new operation will generate a new vertex.
     * @param edges The group of edges represented one of their half-edges that should be used to insert new interface
     * elements.
     */
    [[nodiscard]] unsigned int computeNumberOfNewVertices(const std::vector<unsigned int> &edges) const;

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

