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



void readMesh()
{
    int trash = 0;
    std::string name;

    name = "../malha.txt";

    printf("Reading file!...\n");
    std::ifstream in(name.c_str());
    if (in.is_open())
    {
        std::cout << "File found." << std::endl;
    }
    else
    {
        std::cout << "File not found." << std::endl;
        return;
    }
    int numberPoints = 0, numberTriangles = 0;
    in >> numberPoints >> numberTriangles;

    std::vector<double> points;
    std::vector<unsigned int> triangles;

    printf("Reading points...\n");
    for (int i = 0; i < numberPoints; i++)
    {
        double x, y, z;
        in >> trash >> x >> y >> z;
        points.push_back(x);
        points.push_back(y);
        points.push_back(z);
    }

    for (int i = 0; i < numberTriangles; i++)
    {
        int p1, p2, p3;
        in >> trash >> p1 >> p2 >> p3;
        triangles.push_back(p1);
        triangles.push_back(p2);
        triangles.push_back(p3);
    }
    in.close();

    auto *che = new CHE(points, triangles, 3, 3);

    const CHEOperations cheOperations(che);

    const std::vector<unsigned int> vertices = cheOperations.getNeighbourFaces(3);
    printf("Neighbour faces: %llu\n", vertices.size());
    for (unsigned int i = 0; i < vertices.size(); i++)
    {
        printf("%u: %d\n", i, vertices[i]);
    }

    delete che;
}



int main(int argc, char **argv)
{
    readMesh();
    return 0;
}

