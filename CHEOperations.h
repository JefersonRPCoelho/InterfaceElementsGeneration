/* 
 * File:   CHEOperations.h
 * Author: jeferson
 *
 * Created on 4 de Agosto de 2012, 16:55
 */

#pragma once

#include "CHE.h"
#include <vector>
#include <map>

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
     * @brief Given a half-edge, get the neighbor vertices to the given halfedge.
     * @param he Half-edge used to get the neighbor vertices.
     * @return A vector with all neighbor vertices.
     */
    [[nodiscard]] std::vector<unsigned int> geNeighbourVertices(unsigned int he) const;

    /**
     * @brief Given a face index, get all neighbor faces.
     * @param face The face index to get all neighbor faces.
     * @return The neighbor faces to the given faces.
     */
    [[nodiscard]] std::vector<unsigned int> getNeighbourFaces(unsigned int face) const;

    /**
     * @brief Add interface elements in the required edges.
     *
     * For each required edge a quad4 is inserted in the mesh. The function expects that each element in the input
     * vector is a half edge along the desired edge to insert the interface element. That means, the desired edge is
     * formed by the half-edge's vertex and the next half-edge's vertex.
     * @param edges A list of half-edge representing the desired edges to insert the interface element.
     */
    void addInterfaceElements(const std::vector<unsigned int> &edges);

private:
    /**
     * @todo document.
     * @param he
     */
    void openEdge(unsigned int he);

    /**
     * @brief @todo document.
     * @param startHE
     * @param endHE
     * @return
     */
    unsigned int duplicateNode(unsigned int startHE, unsigned int endHE);

private:

private:
    /**
     * CHE valid topology representation used to perform the operations.
     */
    CHE *_che;

    std::map<unsigned int, unsigned int> _collapsedVertex2HE;
};
