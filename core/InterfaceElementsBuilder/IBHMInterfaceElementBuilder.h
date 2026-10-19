//
// Created by jcoelho on 2/24/2026.
//

#pragma once
#include <unordered_map>
#include <vector>
class IBHM;


/**
 * @brief Insert interface elements in an IBHM mesh.
 *
 * An interface element is a zero-thickness element created by opening an edge of the mesh. The builder receives the
 * mesh it should modify and, for each selected edge, creates the new element and repairs the topology around both of
 * its vertices by applying one of four operators: canonical, split element, expand edge, or open hole.
 *
 * The builder keeps its own bookkeeping about which vertices and elements already belong to an interface element, so a
 * single builder instance must be used for the whole editing session of a mesh. Creating a second builder over a mesh
 * that has already been edited loses that state and leads to the wrong operator being selected.
 *
 * The main algorithm's strengths are:
 * 1. It is based on the operator. There is a traversal algorithm in the selected edges, and an operator is applied to
 * each edge vertex.
 * 2. There are only 4 operators in 2D: canonical, split element, expand edge, and open hole. They are topological only
 * (no coordinates or angles are used), which is meant to ease the future extension to 3D.
 * 3. Although the algorithm is designed to insert elements on the mesh, the decision about what operator should be
 * applied is vertex-based. The vertex information is enough to build the whole decision, avoiding complex algorithms or
 * big combinations to treat all the cases.
 */
class IBHMInterfaceElementBuilder
{
public:
    /**
     * Define the type of all available operators.
     *
     * CANONICAL: The operator that should be applied to a vertex that is not part of any interface element yet. A
     * vertex will be duplicated in the topology only, resulting in a degenerated edge between the duplicated vertices.
     * There is no opposite half-edge defined to this edge. However, the topological operations still working for the
     * internal element. The opposite response for this edge should be COLLAPSED. The exception is when the vertex is on
     * the border. In this case, a geometric vertex is also created and the opposite half-edge for the created edge
     * points to BORDER. By definition, this is the first operation in a regular element. It works as an initialization,
     * preparing the vertex for the other operations. When two internal vertices respect these conditions, the resulting
     * element will be like ABBA with two collapsed edges AA and BB.
     *
     * SPLIT_ELEMENT: When there is a vertex available (duplicated vertex on the topology, but not in the geometry) and
     * the available vertex has an income/outcome half-edge that is part of an element shared by the edge being split.
     * The operator creates a new geometric vertex for the available topological vertex. The collapsed edge in the
     * former interface element now determines the frontier with the brand-new interface element.
     *
     * EXPAND_EDGE: When the former rule fails to find a shared element, this operator should be applied. As the former
     * operator, this operator creates a new geometric vertex for the available topological vertex. The surrounding
     * elements should be reindex with the new vertex. The edge being split determined the point of division between
     * the elements that should be connected with one of the vertices that were collapsed.
     *
     * OPEN_HOLE: The operator that should be applied to a vertex that is already part of an interface element, and
     * there is no vertex available (duplicated vertex in the topology but not in the geometry). The operator will
     * insert or increase a hole.
     *
     * UNDEFINED: Use to initialize variables as an invalid type.
     */
    enum class OperatorType { CANONICAL, SPLIT_ELEMENT, EXPAND_EDGE, OPEN_HOLE, UNDEFINED };

public:
    /**
     * The operators modify a IBHM instance. Therefore, the constructor should receive the instance that should be
     * modified.
     *
     * The builder keeps the information about vertices and elements during the process of element insertion. It
     * supports adaptive elements insertion, but the same builder should be used. It works as a second layer mesh
     * definition. The IBHM defines that current mesh state, including interface elements, but the builder keeps the
     * required definition to be able to insert new interface elements from the last state.
     *
     * As a consequence, there should be only one builder for each IBHM instance. @todo Implement a mechanism to avoid
     * creating more than one builder for the same IBHM instance.
     *
     * @param ibhm The IBHM instance that should be modified.
     */
    explicit IBHMInterfaceElementBuilder(IBHM *ibhm);

    /**
     * Insert interface elements in a group of provided edges.
     *
     * The algorithm inserts the edges in the order they are provided. The final topology does not depend on this order,
     * nor on inserting the edges in a single call or in several calls.
     *
     * Edges that cannot be opened are skipped with a message on the standard output, and nothing is reported to the
     * caller:
     *     1. Border edges, i.e., the provided half-edge has no opposite.
     *     2. Edges that already belong to an interface element, for example, an edge provided twice in the same call or
     *     in different calls.
     * The space reserved for the skipped edges is left unused.
     *
     * @warning The provided indices are not validated. Every value must be the index of a half-edge of an element that
     * already exists in the mesh (a committed element). A half-edge of an interface element makes
     * computeNumberOfNewVertices() loop forever, because the test that skips these edges runs only after it, and an
     * index outside the valid range leads to an assertion failure or to invalid memory access.
     *
     * @param edges A vector with one half-edge for each input edge. Each edge is defined by two half-edges. The user
     * can select any of them.
     */
    void insertInterfaceElements(const std::vector<unsigned int> &edges);

private:
    /**
     * Perform the canonical operator in a vertex identified by the half-edge edgeHE.
     *
     * The edgeHE half-edge identifies the edge being split and the vertex where the canonical operator should be
     * applied.
     *
     * If the vertex is internal to the mesh, no new geometric vertex is created. Instead, a collapsed edge is created
     * in the pre-existent vertex. This edge is saved to the _collapsed mesh with the half-edge identifying the
     * collapsed edge. The opposite is defined as COLLAPSED.
     *
     * When the vertex is on the border, a new geometric vertex is created and the resulting edge is not collapsed
     * anymore. As a result, all elements from the new vertex side should be re-indexed to use the new vertex. See
     * reindexElementsCCW(). The opposite half-edge is marked as a border.
     *
     * All involved vertices are identified as part of an interface element. It is important to identify the right
     * operator to be used.
     *
     * @param edgeHE The half-edge that identifies the edge being split and the vertex where the operator should be
     * applied.
     * @param elementHE The half-edge that identifies the edge being created in the vertice inside the new interface
     * element. The operator supposes that the element is already created and will only work on adjusting the neighbor
     * around the vertex. Therefore, this parameter provides information about the new interface element that should be
     * modified by the canonical operator.
     * @return True if the operation was performed successfully and false otherwise.
     */
    bool canonical(unsigned int edgeHE, unsigned int elementHE);

    /**
     * Perform the Split Element operator in a vertex identified by the half-edge he.
     *
     * This operator is always applied in a vertex with a collapsed edge. The interface element with this collapsed
     * edge should share an edge with one of the two elements of the edge being split. This common element is the
     * shared element, and there are two possibilities (see retrieveAvailableVertex()):
     *     1. It is the element of he. The interface element half-edge that leaves the vertex just after the collapsed
     *     edge is the opposite of previous(he).
     *     2. It is the element of opposite(he). The interface element half-edge that arrives at the vertex just before
     *     the collapsed edge is the opposite of next(opposite(he)).
     *
     * The half-edge availableVertexHE identifies one vertex of the collapsed edge that should be used to insert the new
     * vertex in the shared element.
     *
     * The half-edge sharedElementHE should identify the available vertex in the shared element.
     *
     * The elementHE, as in the canonical operator, should represent the first edge-vertex being opened. First, in the
     * sense of orientation.
     *
     * @param he A half-edge identifying the vertex where the operator should be applied. This vertex should be part of
     * a collapsed edge.
     * @param availableVertexHE A half-edge representing the selected vertex to be replaced by a geometric vertex in the
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
     * @param he A half-edge identifying the vertex where the operator should be applied. This vertex should already be
     * part of an interface element and should not have a collapsed edge, i.e., it should not be in the _collapsed map.
     * @param elementHE The half-edge that identifies the edge being created in the vertice inside the new interface
     * element. The operator supposes that the element is already created and will only work on adjusting the neighbor
     * around the vertex.
     * @return True if the operation was performed successfully and false otherwise.
     */
    bool openHole(unsigned int he, unsigned int elementHE);

    /**
     * Given a half-edge defining an edge, determine which operator should be used in the half-edge's vertex.
     *
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
     * The half-edge stopHE can be provided as IBHM::BORDER.
     *
     * If the stopHE is never found, the algorithm will replace the vertex from 'he' by 'v' for all elements.
     *
     * Precondition: the rotation must reach stopHE, the border, or 'he' again before it arrives at the half-edge that
     * follows a collapsed edge. The next half-edge is obtained as opposite(previous(currentHE)), and a COLLAPSED value
     * is not tested, so it would be used as a half-edge index. The operators that call this method respect this
     * condition.
     *
     * @param he A half-edge outgoing from the vertex that should be replaced by v.
     * @param stopHE The half-edge the algorithm should stop when finding it. It can be IBHM::BORDER.
     * @param v The new vertex index that should be used to replace the vertex in 'he'.
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
     * the half-edge index for the available vertex to be used in the operator or IBHM::BORDER if the operator is
     * EXPAND_EDGE.
     * @param sharedElementHE The half-edge for the node that should be index inside the shared face or IBHM::BORDER if
     * the operator is EXPAND_EDGE.
     */
    void retrieveAvailableVertex(unsigned int he, unsigned int &availableVertexHE, unsigned int &sharedElementHE);

public:
    /**
     * Compute the number of new vertices that will be required to insert interface elements in the provided edges.
     *
     * Only the edges that insertInterfaceElements() will really open are counted. The edges it skips are skipped here
     * too, with the same tests: border edges, edges of interface elements, and repeated edges (also when an edge is
     * given once by each of its half-edges).
     *
     * Then, for each vertex, let n be the number of counted edges that touch it. Considering the state of the vertex
     * before the insertion:
     *     1. If the vertex is already part of an interface element, it needs n new vertices, since every new edge
     *     duplicates it.
     *     2. If it is not part of an interface element and is on the border, it also needs n new vertices: a border
     *     vertex is duplicated already at the first edge.
     *     3. If it is not part of an interface element and is internal, it needs n - 1 new vertices: the first edge
     *     only creates a collapsed edge, which does not require a new vertex.
     * The rule, and the border test, are applied once per vertex.
     *
     * The result is the exact number of vertices the insertion creates, so it can be used to reserve exactly the memory
     * needed. A second edge on a border vertex that is already part of an interface element is counted as one new
     * vertex, which is the expected result, although the insertion does not support this case yet.
     *
     * @param edges The group of edges, each one represented by one of its half-edges, that should be used to insert new
     * interface elements.
     * @return The number of new vertices.
     */
    [[nodiscard]] unsigned int computeNumberOfNewVertices(const std::vector<unsigned int> &edges) const;

private:
    /**
     * IBHM instance that should be modified by the class operators.
     */
    IBHM *_ibhm = nullptr;

    /**
     * Determine if a vertex is on an interface element or not.
     */
    std::vector<bool> _inInterfaceElement;

    /**
     * Determine if an element is an interface element or not.
     */
    std::vector<bool> _isInterfaceElement;

    /**
     * Stores the current collapsed edges. The key is the vertex that has a collapsed edge, and the value is the
     * collapsed half-edge itself, i.e., the short side (h3 or h1) of the interface element that created it, which goes
     * from the vertex to the vertex itself. The entry is removed when the vertex is split by SPLIT_ELEMENT or
     * EXPAND_EDGE.
     *
     * @todo evaluate if we can get rid of it by getting this information on the fly.
     */
    std::unordered_map<unsigned int, unsigned int> _collapsed;
};

