/* 
 * File:   CHELevelOne.h
 * Author: jeferson
 *
 * Created on 27 de Julho de 2012, 00:28
 */

#pragma once

#include <vector>

class CHE final
{
    friend class CHEOperations;
public:
    /**
     * @brief Build a CHE based on the coordinates and element list.
     *
     * This constructor uses only primitive types to be flexible. I will save the coordinate's vector, the
     * element's list, and build the opposite table.
     * @param coordinates The coordinate's vector. It should have @p numberOfVertices * @p numberCoordinates elements.
     * @param elementsList The element list indexes. It should have @p numberElements * @p numberVerticesByElement
     * integers.
     * @param numberOfVertices The number of vertices in the coordinate's vector.
     * @param numberElements The number of elements in the elements vector.
     * @param numberVerticesByElement The number of vertices by element. That is, 3 for triangles, 4 for quads, and so
     * on.
     * @param numberCoordinates The number of coordinates per point. That is, 2 for 2D, 3 for 3D, 6 for xyz and normals
     * nx ny nz, and so on.
     */
    CHE(const double *coordinates, const unsigned int *elementsList, unsigned int numberOfVertices,
        unsigned int numberElements, unsigned int numberVerticesByElement, unsigned int numberCoordinates);

    /**
     * @brief Build a CHE based on the coordinates and elements list.
     * It will save the coordinate's vector, the element's list, and build the opposite table.
     *
     * @param coordinates The coordinate's vector. It should have @p numberOfVertices * @p numberCoordinates elements.
     * @param elementsList The element list indexes. It should have @p numberElements * @p numberVerticesByElement
     * @param numberVerticesByElement The number of vertices by element. That is, 3 for triangles, 4 for quads, and so
     * on.
     * @param numberCoordinates The number of coordinates per point. That is, 2 for 2D, 3 for 3D, 6 for xyz and normals
     * nx ny nz, and so on.
     */
    CHE(const std::vector<double> &coordinates, const std::vector<unsigned int> &elementsList,
        unsigned int numberVerticesByElement,
        unsigned int numberCoordinates);

    /**
     * Default constructor.
     */
    ~CHE() = default;

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
    [[nodiscard]] unsigned int halfEdgeElement(unsigned int halfEdge) const;

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
     * @brief Get the number of vertice for each element.
     *
     * It represents the element order.
     * @return The number of vertices required for each element.
     */
    [[nodiscard]] unsigned int numberVertexByElement() const;

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

public:
    enum OPPOSITE
    {
        BORDER = -1,
        COLLAPSED = -2,
    };

    /**
     * Print the data structure state.
     */
    void print() const;
private:
    /**
     * @brief @todo document
     * @param numberElements
     */
    void reserveSpaceForElements(unsigned int numberElements);
private:
    /**
     * @brief Stores the coordinate's vector.
     *
     * For each _numberCoordinatesPerVertex vector values, a vertice coordinate is represented.
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
     * @brief Stores the number of vertices by element.
     *
     * This variable allows you to extend the structure for any element level. The only constraint is that all elements
     * on the mesh should have the same number of vertices.
     */
    unsigned int _numberVerticesByElement;

    /**
     * @brief Get the number of valid elements.
     *
     * The element list can be pre allocated, allowing to have invalid elements. This variable determines the number
     * of valid elements in the vector.
     */
    unsigned int _numberOfValidElements;

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


