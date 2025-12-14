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
public:
    /**
     * @brief @todo document
     * @param coordinates
     * @param elementsList 
     * @param numberOfVertices 
     * @param numberElements 
     * @param numberVerticesByElement 
     * @param numberCoordinates 
     */
    CHE(const double *coordinates, const unsigned int *elementsList, unsigned int numberOfVertices,
        unsigned int numberElements, unsigned int numberVerticesByElement, unsigned int numberCoordinates);

    /**
     * @brief @todo document.
     * @param vertices
     * @param elementsList
     * @param numberVerticesByElement
     * @param numberCoordinates
     */
    CHE(const std::vector<double> &vertices, const std::vector<unsigned int> &elementsList,
        unsigned int numberVerticesByElement,
        unsigned int numberCoordinates);

    /**
     * Default constructor.
     */
    ~CHE() = default;

    /**
     * @brief @todo document.
     * @return
     */
    const std::vector<unsigned int> &getElementsList();

    /**
     * @brief @todo document.
     * @return
     */
    std::vector<double> &getCoordinates();

    /**
     * @brief @todo document.
     * @param halfEdge
     * @return
     */
    [[nodiscard]] unsigned int halfEdgeElement(unsigned int halfEdge) const;

    /**
     * @brief @todo document.
     * @param halfEdge
     * @return
     */
    [[nodiscard]] unsigned int halfEdgeNext(unsigned int halfEdge) const;

    /**
     * @brief @todo document.
     * @param halfEdge
     * @return
     */
    [[nodiscard]] unsigned int halfEdgePrevious(unsigned int halfEdge) const;

    /**
     * @brief @todo document.
     * @return
     */
    [[nodiscard]] unsigned int getNumberPoints() const;

    /**
     * @brief @todo document.
     * @return
     */
    [[nodiscard]] unsigned int getNumberElements() const;

    /**
     * @brief @todo document.
     * @return
     */
    [[nodiscard]] unsigned int getNumberCoordinatesByVertex() const;

    /**
     * @brief @todo document.
     * @return
     */
    [[nodiscard]] unsigned int getNumberVertexByElement() const;

    /**
     * @brief @todo document.
     * @param halfEdge
     * @param coordinates
     */
    void getCoordinatesFromHalfEge(unsigned int halfEdge, double *coordinates) const;

    /**
     * @brief @todo document.
     * @param index
     * @param coordinates
     */
    void getCoordinatesFromIndex(unsigned int index, double *coordinates) const;

    /**
     * @brief @todo document.
     * @param halfEdge
     * @return
     */
    [[nodiscard]] unsigned int getHalfEdgeVertexIndex(unsigned int halfEdge) const;

    /**
     * @brief @todo document.
     * @param halfEdge
     * @return
     */
    [[nodiscard]] unsigned int halfEdgeOpposite(unsigned int halfEdge) const;

private:
    /**
     * @brief @todo document.
     */
    std::vector<double> _coordinates;

    /**
     * @brief @todo document.
     */
    std::vector<unsigned int> _halfEdgeVertex;

    /**
     * @brief @todo document.
     */
    std::vector<unsigned int> _oppositeHalfEdge;

    /**
     * @brief @todo document.
     */
    unsigned int _numberCoordinatesPerVertex;

    /**
     * @brief @todo document.
     */
    unsigned int _numberVerticesByElement;

private:
    /**
     * @brief @todo document.
     */
    void buildOppositesLinear();
};


