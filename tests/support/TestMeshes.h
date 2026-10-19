#pragma once
#include <vector>
#include "IBHM.h"


/**
 * @brief The base mesh: the example of the slides and of the report (malha2Hyb.txt).
 *
 * 19 vertices and 13 elements (quadrilaterals and triangles), 48 half-edges. Vertex names used in the comments of the
 * tests:
 *     y=0: A B C D      y=1: E F G H I      y=2: J K L M N      y=3: P Q R S      and T = (4, 3).
 * The vertex T (18) is at (4, 3); in malha2Hyb.txt it is at (3, 4), which only changes the geometry.
 * @return The mesh.
 */
[[nodiscard]] IBHM baseMesh();

/**
 * @brief The edges of the example of the slides on the base mesh, in the order of the slides.
 * @return One half-edge per edge.
 */
[[nodiscard]] std::vector<unsigned int> baseMeshEdges();

/**
 * @brief Three quadrilaterals around the border vertex Q = 0 (a half disk), so Q has two interior edges: Q-R
 * (half-edge 4) and Q-P (half-edge 8).
 * @return The mesh.
 */
[[nodiscard]] IBHM fanMesh();

/**
 * @brief Two triangles sharing an edge: 4 points in 2D, (0, 0), (1, 0), (1, 1) and (0, 1).
 * @return The mesh.
 */
[[nodiscard]] IBHM twoTrianglesMesh();
