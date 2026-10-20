#include "TestMeshes.h"



IBHM baseMesh()
{
    const std::vector<float> coordinates = {0, 0, 1, 0, 2, 0, 3, 0,
                                            0, 1, 1, 1, 2, 1, 3, 1, 4, 1,
                                            0, 2, 1, 2, 2, 2, 3, 2, 4, 2,
                                            0, 3, 1, 3, 2, 3, 3, 3,
                                            4, 3};
    const std::vector<unsigned int> elements = {0, 1, 5, 4, 1, 2, 6, 5, 2, 3, 7, 6,
                                                4, 5, 10, 9, 5, 6, 11, 10, 6, 7, 12, 11, 7, 8, 12, 8, 13, 12,
                                                9, 10, 15, 14, 10, 11, 16, 15, 11, 12, 17, 16, 12, 13, 18, 12, 18, 17};
    const std::vector<unsigned int> offset = {0, 4, 8, 12, 16, 20, 24, 27, 30, 34, 38, 42, 45, 48};
    return IBHM(coordinates, elements, offset, 2);
}



std::vector<unsigned int> baseMeshEdges()
{
    return {31, 34, 11, 26, 47, 38, 23, 19, 30, 42, 29, 45};
}



IBHM fanMesh()
{
    const std::vector<float> coordinates = {0, 0, 1, 0, 0.5f, 0.866f, -0.5f, 0.866f, -1, 0, 1.2f, 0.7f, 0, 1.4f,
                                            -1.2f, 0.7f};
    const std::vector<unsigned int> elements = {0, 1, 5, 2, 0, 2, 6, 3, 0, 3, 7, 4};
    const std::vector<unsigned int> offset = {0, 4, 8, 12};
    return IBHM(coordinates, elements, offset, 2);
}



IBHM wideFanMesh()
{
    // Q at the origin, p1 to p5 at radius 1 every 45 degrees, and one outer vertex per element at radius 1.5.
    const std::vector<float> coordinates = {0, 0,
                                            1, 0, 0.7071f, 0.7071f, 0, 1, -0.7071f, 0.7071f, -1, 0,
                                            1.3858f, 0.5740f, 0.5740f, 1.3858f, -0.5740f, 1.3858f, -1.3858f, 0.5740f};
    const std::vector<unsigned int> elements = {0, 1, 6, 2, 0, 2, 7, 3, 0, 3, 8, 4, 0, 4, 9, 5};
    const std::vector<unsigned int> offset = {0, 4, 8, 12, 16};
    return IBHM(coordinates, elements, offset, 2);
}



IBHM holeMesh()
{
    const std::vector<float> coordinates = {3, 3, 0, 3, 0, 0, 3, 0, 2, 2, 1, 2, 1, 1, 2, 1, 1.5f, 3};
    const std::vector<unsigned int> elements = {4, 0, 8, 0, 4, 7, 3, 3, 7, 6, 2, 2, 6, 5, 1, 1, 5, 8, 5, 4, 8};
    const std::vector<unsigned int> offset = {0, 3, 7, 11, 15, 18, 21};
    return IBHM(coordinates, elements, offset, 2);
}



IBHM twoTrianglesMesh()
{
    const std::vector<float> coordinates = {0, 0, 1, 0, 1, 1, 0, 1};
    const std::vector<unsigned int> elements = {0, 1, 2, 0, 2, 3};
    const std::vector<unsigned int> offset = {0, 3, 6};
    return IBHM(coordinates, elements, offset, 2);
}
