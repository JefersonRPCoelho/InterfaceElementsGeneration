/* 
 * File:   main.cpp
 * Author: jeferson
 *
 * Created on 27 de Julho de 2012, 00:27
 */

#include <vector>
#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include "CHE.h"
#include "CHEOperations.h"
#include <set>



CHE *readMesh(const std::string &filename)
{
    printf("Reading file!...\n");
    std::ifstream in(filename.c_str());
    if (in.is_open())
    {
        std::cout << "File found." << std::endl;
    }
    else
    {
        std::cout << "File not found." << std::endl;
        return nullptr;
    }
    int numberPoints = 0, numberTriangles = 0;
    in >> numberPoints >> numberTriangles;

    std::vector<double> points;
    std::vector<unsigned int> elements;

    printf("Reading points...\n");
    for (int i = 0; i < numberPoints; i++)
    {
        double x, y;
        in >> x >> y;
        points.push_back(x);
        points.push_back(y);
    }

    for (int i = 0; i < numberTriangles; i++)
    {
        int p1, p2, p3, p4;
        in >> p1 >> p2 >> p3 >> p4;
        elements.push_back(p1);
        elements.push_back(p2);
        elements.push_back(p3);
        elements.push_back(p4);
    }
    in.close();

    auto *che = new CHE(points, elements, 4, 2);
    che->print();

    return che;
}



void insertInterfaceElements(CHE *che)
{
    CHEOperations cheOperations(che);
    cheOperations.addInterfaceElements({35, 32, 33});
    // cheOperations.addInterfaceElements({29, 18, 5, 21, 37, 36, 17, 13, 14, 40});

    // constexpr unsigned int he = 2;
    // const std::vector<unsigned int> nv = cheOperations.geNeighbourVertices(he);
    // printf("%u: ", che->heVertexIndex(he));
    // for (const unsigned int v: nv)
    // {
    //     printf("%u ", v);
    // }
    // printf("\n");
}



int main(int argc, char **argv)
{
    CHE *che = readMesh("../malha2.txt");
    insertInterfaceElements(che);

    delete che;

    return 0;
}

