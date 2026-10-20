/* 
 * File:   IBHM.h
 * Author: jeferson
 *
 * Created on 27 de Julho de 2012, 00:28
 */

#pragma once

#include <vector>

/**
 * @brief Index-Based Half-edge data structure for Hybrid Meshes.
 *
 * The mesh is described by three arrays: the vertex coordinates, the element list, that also
 * plays the role of the half-edge to vertex table, and an offset table telling where each
 * element starts in the element list. Storing the offset is what makes the structure hybrid: the
 * elements do not need to share the same number of vertices.
 *
 * A half-edge is simply an index in the element list. Every topological query is therefore an
 * array lookup, and the neighborhood is recovered from the opposite table built at construction
 * time.
 */
class IBHM final
{
public:
    /**
     * Special values stored in the opposite table instead of a half-edge index.
     *
     * BORDER: the half-edge is on the border of the mesh (including the border of a hole), so it has no opposite.
     *
     * COLLAPSED: the half-edge is a collapsed edge of an interface element, i.e., an edge that goes from a vertex to
     * the same vertex. It is created by the interface element insertion and has no opposite.
     */
    enum OPPOSITE
    {
        BORDER = -1,
        COLLAPSED = -2,
    };

public:
    /**
     * @brief Build a IBHM based on the coordinates and elements list.
     * It will save the coordinate's vector, the element's list, and build the opposite table.
     *
     * @param coordinates The coordinate's vector. It should have (number of vertices) * @p numberCoordinates elements.
     * @param elementsList The element list indexes. It is expected to have hybrid elements in the vector.
     * @param offset A vector with size n + 1, where n is the number of elements. The position i stores the index where
     * the element i starts in the elementsList. The position i + 1 is the upper bound.
     * @param numberCoordinates The number of coordinates per point. That is, 2 for 2D, 3 for 3D, 6 for xyz and normals
     * nx ny nz, and so on.
     */
    IBHM(const std::vector<float> &coordinates, const std::vector<unsigned int> &elementsList,
         const std::vector<unsigned int> &offset, unsigned int numberCoordinates);

    /**
     * Default destructor.
     */
    ~IBHM() = default;

    /**
     * @brief Get the element's list.
     *
     * Only the IBHM class can edit the element's list, so it is returned in read-only mode.
     * @return The elements list.
     */
    const std::vector<unsigned int> &elementsList();

    /**
     * @brief Given a half-edge, return the element that contains the provided half-edge.
     *
     * The half-edge must belong to a valid element (halfEdge < numberOfHalfEdges()). The sentinels BORDER and COLLAPSED
     * are not half-edges: test an opposite before passing it here. Checked by an assertion in debug.
     * @param halfEdge The half-edge provided to recognize the element it belongs to.
     * @return The element the provided half-edge belongs to.
     */
    [[nodiscard]] unsigned int heElement(unsigned int halfEdge) const;

    /**
     * @brief Get the next half-edge.
     *
     * Given a half-edge, the next half-edge is that half-edge that appears just after the given half-edge when
     * considering the element orientation.
     *
     * The half-edge must belong to a valid element (halfEdge < numberOfHalfEdges()). The sentinels BORDER and COLLAPSED
     * are not half-edges: test an opposite before passing it here. Checked by an assertion in debug.
     * @param halfEdge The given half-edge to get the next.
     * @return The next half-edge in relation to the provided half-edge.
     */
    [[nodiscard]] unsigned int heNext(unsigned int halfEdge) const;

    /**
     * @brief Get the previous half-edge.
     *
     * Given a half-edge, the previous half-edge is that half-edge that appears just before the given half-edge when
     * considering the element orientation.
     *
     * The half-edge must belong to a valid element (halfEdge < numberOfHalfEdges()). The sentinels BORDER and COLLAPSED
     * are not half-edges: test an opposite before passing it here. Checked by an assertion in debug.
     * @param halfEdge The given half-edge to get the previous.
     * @return The previous half-edge in relation to the provided half-edge.
     */
    [[nodiscard]] unsigned int hePrevious(unsigned int halfEdge) const;

    /**
     * @brief Get the number of points in the mesh.
     * @return The number of points in the mesh.
     */
    [[nodiscard]] unsigned int numberPoints() const;

    /**
     * Get the vector with the points' data.
     * @return The mesh points data.
     */
    [[nodiscard]] const std::vector<float> &points() const;

    /**
     * @brief Add a new point to the mesh geometry.
     *
     * The point is appended to the end of the coordinate's vector, and its index is numberPoints() before the call. If
     * the space was reserved with reserveSpaceForNodes(), no reallocation happens; otherwise, the vector grows as
     * needed.
     * @param coordinates The coordinates for the new point. It should have exactly numberCoordinatesByVertex() values,
     * or be empty to use the origin (the default). It is a separate vector, read only through its own indices, so it is
     * safe to build it from the coordinates of a point of the mesh, e.g., to create a copy of a vertex.
     * @return The index for the new vertex.
     */
    unsigned int addPoint(const std::vector<float> &coordinates = {});

    /**
     * @brief Add a new point with the same coordinates as an existing vertex.
     *
     * It is used to create the copies of a vertex when the mesh is split: the copies are geometrically coincident with
     * the original vertex. The coordinates are read by index while they are appended, so no temporary vector is needed,
     * and the result is correct even if the coordinate's vector is reallocated. As in addPoint(), the new point is
     * appended to the end, and reserveSpaceForNodes() avoids reallocations.
     * @param vertex The index of the existing vertex whose coordinates should be copied.
     * @return The index for the new vertex.
     */
    unsigned int duplicatePoint(unsigned int vertex);

    /**
     * Mark the next element index as valid.
     * @param elementSize The size of the element being inserted.
     * @return The index for the new valid element.
     */
    unsigned int commitElement(unsigned int elementSize);

    /**
     * Define the opposite for halfEdge as oppositeHalfEdge.
     *
     * If oppositeHalfEdge is a valid half-edge, the opposite is set in both directions.
     * @param halfEdge The half-edge to set the opposite.
     * @param oppositeHalfEdge The opposite half-edge.
     */
    void setOpposite(unsigned int halfEdge, unsigned int oppositeHalfEdge);

    /**
     * @brief Set a vertex in an element half-edge, i.e., change the element connectivity.
     * @param he The element half-edge.
     * @param vertex The vertex index.
     */
    void setElementVertex(unsigned int he, unsigned int vertex);

    /**
     * @brief Get the number of reserved half-edge positions in the element's list.
     *
     * Despite the name, the value is not a number of elements: since the elements may have different sizes, it returns
     * the size of the element's list, i.e., the number of half-edge positions available before requiring reallocation,
     * including the positions reserved by reserveSpaceForElements() and not used yet.
     * @return The number of reserved half-edge positions.
     */
    [[nodiscard]] unsigned int numberReservedElements() const;

    /**
     * @brief  Get the number of coordinates by vertex.
     *
     * It represents the vertice dimension.
     * @return The number of coordinates required for each vertex.
     */
    [[nodiscard]] unsigned int numberCoordinatesByVertex() const;

    /**
     * @brief Given a half-edge, get the coordinates from the vertex attached to the half-edge.
     * @param halfEdge The half-edge attached to the vertex of interest.
     * @param coordinates A pre allocated vector with at least _numberCoordinatesPerVertex position. The function will
     * store the vertice coordinates in this vector.
     */
    void coordinatesFromHalfEge(unsigned int halfEdge, double *coordinates) const;

    /**
     * @brief Given a vertex index, get the coordinates.
     * @param index The vertex index.
     * @param coordinates A pre allocated vector with at least _numberCoordinatesPerVertex position. The function will
     * store the vertice coordinates in this vector.
     */
    void coordinatesFromIndex(unsigned int index, double *coordinates) const;

    /**
     * @brief Given a half-edge, get the vertex represented by this half-edge.
     * @param halfEdge The half-edge attached to the vertex that is being required.
     * @return The vertex attached to the given half-edge.
     */
    [[nodiscard]] unsigned int heVertexIndex(unsigned int halfEdge) const;

    /**
     * @brief Given a half-edge, get the opposite half-edge.
     * @param halfEdge The given half-edge to query to the opposite.
     * @return The opposite half-edge if it exists, BORDER (-1) if the half-edge is on the border, or COLLAPSED (-2) if
     * it is a collapsed edge of an interface element. As the return type is unsigned, compare the result with
     * IBHM::BORDER and IBHM::COLLAPSED instead of with negative numbers.
     */
    [[nodiscard]] unsigned int heOpposite(unsigned int halfEdge) const;

    /**
     * Get the number of elements in the mesh.
     * @return The number of valid elements in the mesh.
     */
    [[nodiscard]] unsigned int numberOfElements() const;

    /**
     * @brief Get the number of half-edges of the mesh, i.e., of its valid (committed) elements.
     *
     * The valid half-edge indices go from 0 to numberOfHalfEdges() - 1. The positions reserved by
     * reserveSpaceForElements() and not used by a committed element yet are not counted; numberReservedElements()
     * counts them.
     * @return The number of half-edges of the valid elements.
     */
    [[nodiscard]] unsigned int numberOfHalfEdges() const;

    /**
     * Given an element index, get the first element half-edge.
     * @param elementIndex The element index.
     * @return The first element half-edge.
     */
    [[nodiscard]] unsigned int firstElementHE(unsigned int elementIndex) const;

    /**
     * Determine if a vertex is on the mesh border or not.
     *
     * The method rotates clockwise around the vertex until it finds a half-edge without opposite (BORDER) or returns to
     * the starting half-edge. A collapsed edge of an interface element (opposite COLLAPSED) is crossed through the next
     * half-edge of the same element, which also starts at the vertex. Any half-edge that starts at the vertex can be
     * used, including the half-edges of interface elements. The border of a hole in the mesh, including the hole that
     * Open Hole creates at a junction, is a border too.
     * @param he A half-edge in the vertex that should be tested if it is on the border or not.
     * @return True if the vertex is on the border and false otherwise.
     */
    [[nodiscard]] bool isBorder(unsigned int he) const;

    /**
     * Get the next available half-edge in the mesh available to be used in new elements.
     * @return The next available half-edge index.
     */
    [[nodiscard]] unsigned int nextAvailableHE() const;

    /**
     * Print the data structure state.
     */
    void print() const;

    /**
     * @brief Reserve space for new elements.
     *
     * Although the space is reserved, it is not used. The number of elements inserted in the mesh is controlled by
     * _numberOfValidElements, so avoid using the vectors' size to determine the number of elements.
     *
     * The reserved positions of the opposite table start with BORDER. So, a reserved half-edge queried before being
     * linked by its owner is seen as a border, and not as a link to half-edge 0, which is a valid half-edge of the
     * mesh.
     * @param numberPositions Number of extra vector positions to be allocated.
     * @param numberElements The number of elements that will be stored in the reserved space.
     */
    void reserveSpaceForElements(unsigned int numberPositions, unsigned int numberElements);

    /**
     * @brief Reserve space for new nodes.
     *
     * Only the capacity of the coordinate's vector changes; its size, and therefore numberPoints(), stays the same.
     * The capacity becomes exactly the current number of points plus @p numberNodes (or stays the same if it is
     * already larger), so no memory is reserved beyond what is requested, and the next @p numberNodes calls to
     * addPoint() do not reallocate the vector.
     *
     * As the reservation is exact, calling this function many times with small values may reallocate the vector at
     * each call. If many insertions are expected, the user can reserve more space in advance with a single call.
     * @param numberNodes Number of extra points that should be reserved in the mesh.
     */
    void reserveSpaceForNodes(unsigned int numberNodes);

private:
    /**
     * Given a half-edge he, get the iterator for the element position.
     * @param he The given half-edge.
     * @return An iterator for the element position that the half-edge belongs to.
     */
    std::vector<unsigned int>::const_iterator offsetIterator(unsigned int he) const;

private:
    /**
     * @brief Stores the coordinate's vector.
     *
     * For each _numberCoordinatesPerVertex vector value, a vertice coordinate is represented.
     */
    std::vector<float> _coordinates;

    /**
     * @brief Stores the element's list.
     *
     * It also represents the half-edge list implicitly. The index vector is also the half-edge index for that element.
     *
     * The elements are stored one after the other and may have different sizes. The element 'i' occupies the positions
     * _offset[i] to _offset[i + 1] - 1, and the position _offset[i] + k stores the k-th vertex of the element, which is
     * the origin of the corresponding half-edge. The vector may be larger than _offset[numberOfElements()] when space
     * was reserved for new elements.
     */
    std::vector<unsigned int> _halfEdgeVertex;

    /**
     * @brief For each half-edge in the mesh, stores the opposite half-edge, BORDER (-1) if it is a border, or COLLAPSED
     * (-2) if it is a collapsed edge of an interface element.
     */
    std::vector<unsigned int> _oppositeHalfEdge;

    /**
     * @brief Store the number of dimensions each vertex should store in the coordinate's vector.
     */
    unsigned int _numberCoordinatesPerVertex;

    /**
     * @brief Get the number of valid elements.
     *
     * The element list can be pre allocated, allowing to have invalid elements. This variable determines the number
     * of valid elements in the vector.
     */
    unsigned int _numberOfValidElements;

    /**
     * @brief Stores the number of points inserted in the mesh.
     *
     * The coordinate's vector always stores exactly _numberOfValidPoints * _numberCoordinatesPerVertex values; the
     * space for future points is kept as reserved capacity (see reserveSpaceForNodes()).
     */
    unsigned int _numberOfValidPoints;

    /**
    * A vector with size n + 1, where n is the number of elements. The position i stores the index where the element i
    * starts in the _halfEdgeVertex. The position i + 1 is the upper bound for the element i.
     */
    std::vector<unsigned int> _offset;

private:
    /**
     * @brief Build the opposite table.
     *
     * The algorithm builds a temporary list, for each vertex, with the half-edges that start at that vertex. For each
     * half-edge from vertex A to vertex B, it looks among the half-edges that start at B for one that goes back to A.
     * Whenever such a pair is found, the two half-edges are assigned as opposites to each other.
     *
     * For those edges with no pair, the half-edge is assigned to -1, which means a border.
     */
    void buildOppositesLinear();
};


