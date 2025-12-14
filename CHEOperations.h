/* 
 * File:   CHEOperations.h
 * Author: jeferson
 *
 * Created on 4 de Agosto de 2012, 16:55
 */

#pragma once

#include "CHE.h"
#include <vector>

class CHEOperations final
{
public:
    /**
     * @brief Object constructor.
     *
     * It operates over a basic CHE representation, so it should be constructed with a valid one.
     * @param che Valid CHE to operate over it.
     */
    explicit CHEOperations(CHE *che);

    /**
     * Copy constructor.
     * @param orig The object bein copied.
     */
    CHEOperations(const CHEOperations &orig);

    /**
     * @brief Destructor
     */
    ~CHEOperations() = default;

public:
    /**
     * @brief Given a half edge, get the neighbour vertices to the given halfedge.
     * @param halfEdge Half edge used to get the neighbour vertices.
     * @return A vector with all neighbour vertices.
     */
    [[nodiscard]] std::vector<unsigned int> geNeighbourVertices(unsigned int halfEdge) const;

    /**
     * @brief Given a face index, get all neighbour faces.
     * @param face The face index to get all neighbour faces.
     * @return The neighbour faces to the given faces.
     */
    [[nodiscard]] std::vector<unsigned int> getNeighbourFaces(unsigned int face) const;

private:
    /**
     * CHE valid topology representation used to perform the operations.
     */
    CHE *_che;
};
