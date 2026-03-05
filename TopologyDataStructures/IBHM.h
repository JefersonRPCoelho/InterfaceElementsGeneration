/* 
 * File:   CHELevelOne.h
 * Author: jeferson
 *
 * Created on 27 de Julho de 2012, 00:28
 */

#pragma once

#include <vector>

class IBHM final
{
public:
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
     * @param coordinates The coordinate's vector. It should have @p numberOfVertices * @p numberCoordinates elements.
     * @param elementsList The element list indexes. It is expected to have hybrid elements in the vector.
     * @param offset A vector with size n + 1, where n is the number of elements. The position i stores the index where
     * the element i starts in the elementsList. The position i + 1 is the upper bound.
     * @param numberCoordinates The number of coordinates per point. That is, 2 for 2D, 3 for 3D, 6 for xyz and normals
     * nx ny nz, and so on.
     */
    IBHM(const std::vector<double> &coordinates, const std::vector<unsigned int> &elementsList,
         const std::vector<unsigned int> &offset, unsigned int numberCoordinates);

    /**
     * Default constructor.
     */
    ~IBHM() = default;

    /**
     * @brief Get the element's list.
     *
     * The CHE class can only edit the element's list. It is only available in the read-only mode.
     * @return The elements list.
     */
    const std::vector<unsigned int> &elementsList();

    /**
     * @brief Get the coordinate's vector.
     * @return The coordinate's vector.
     */
    std::vector<double> &coordinates();

    /**
     * @brief Given a half-edge, return the element that contains the provided half-edge.
     * @param halfEdge The half-edge provided to recognize the element it belongs to.
     * @return The element the provided half-edge belongs to.
     */
    [[nodiscard]] unsigned int heElement(unsigned int halfEdge) const;

    /**
     * @brief Get the next half-edge.
     *
     * Given a half-edge, the next half-edge is that half-edge that appears just after the given half-edge when
     * considering the element orientation.
     * @param halfEdge The given half-edge to get the next.
     * @return The next half-edge in relation to the provided half-edge.
     */
    [[nodiscard]] unsigned int heNext(unsigned int halfEdge) const;

    /**
     * @brief Get the previous half-edge.
     *
     * Given a half-edge, the previous half-edge is that half-edge that appears just before the given half-edge when
     * considering the element orientation.
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
     * @brief Add a new point to the mesh geometry.
     *
     * The function supposes that the required space is already allocated.
     * @param coordinates - The coordinates for the new point. If it is nullptr the origin point will be used.
     * @return The index for the new vertex.
     */
    unsigned int addPoint(const double *coordinates = nullptr);

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
     * @brief Get the number of reserved elements in the mesh.
     *
     * The number of reserved elements is the number of elements the current vector can store before requiring
     * reallocation.
     * @return The number of elements in the mesh.
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
     * @return The opposite half-edge if it exists or -1 if the opposite is a border.
     */
    [[nodiscard]] unsigned int heOpposite(unsigned int halfEdge) const;

    /**
     * Get the number of elements in the mesh.
     * @return The number of valid elements in the mesh.
     */
    [[nodiscard]] unsigned int numberOfElements() const;

    /**
     * Given an element index, get the first element half-edge.
     * @param elementIndex The element index.
     * @return The first element half-edge.
     */
    [[nodiscard]] unsigned int firstElementHE(unsigned int elementIndex) const;

    /**
     * Determine if a vertex is on the mesh border or not.
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
     * @param numberPositions Number of extra vector positions to be allocated.
     * @param numberElements The number of elements that will be stored in the reserved space.
     */
    void reserveSpaceForElements(unsigned int numberPositions, unsigned int numberElements);

    /**
     * @brief Reserve space for new nodes.
     *
     * Although the space is reserved, it is not used. The number of points inserted in the mesh is controlled by
     * _numberOfValidPoints, so avoid using the vectors' size to determine the number of points.
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
    std::vector<double> _coordinates;

    /**
     * @brief Stores the element's list.
     *
     * It also represents, implicitly, the half-edge list. The index vector is also the half-edge index for that
     * element.
     *
     * For each _numberVerticesByElement vector values, an element is represented.
     */
    std::vector<unsigned int> _halfEdgeVertex;

    /**
     * @brief For each half-edge in the mesh, stores an opposite half-edge or -1 if it is a border.
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
     * The points related structured can be pre-allocated, allowing to having elements that are not part of the mesh yet.
     * This variable determines the number of valid points in the mesh.
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
     * The algorithm builds a temporary map from each vertex to all elements that share that vertex and looks for edge
     * with the same vertex, but with opposite direction. Whenever a pair of edges with this characteristic is found,the
     * correspondent pair of half-edges is assigned as opposite to each other.
     *
     * For those edges with no pair, the half-edge is assigned to -1, which means a border.
     */
    void buildOppositesLinear();
};


