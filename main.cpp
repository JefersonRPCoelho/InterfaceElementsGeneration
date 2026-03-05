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
#include "IBHM.h"
#include "CHEInterfaceElementBuilder.h"



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
        int trash, p1, p2, p3, p4;
        in >> trash >> p1 >> p2 >> p3 >> p4;
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



IBHM *readHybridMesh(const std::string &filename)
{
    printf("Reading hybrid mesh file!...\n");
    std::ifstream in(filename.c_str());
    if (!in.is_open())
    {
        std::cerr << "Error opening the file " << filename << std::endl;
        return nullptr;
    }

    // Mesh data.
    std::vector<double> coordinates;
    std::vector<unsigned int> elements;
    std::vector<unsigned int> offset;

    unsigned int numberPoints = 0, numberElements = 0;
    in >> numberPoints >> numberElements;

    coordinates.resize(numberPoints * 2);

    // Read the coordinates.
    for (unsigned int i = 0; i < numberPoints; i++)
    {
        in >> coordinates[i * 2 + 0] >> coordinates[i * 2 + 1];
    }

    // Read the elements.
    offset.resize(numberElements + 1, 0);
    for (unsigned int i = 0; i < numberElements; i++)
    {
        unsigned int elementSize = 0;
        in >> elementSize;
        offset[i + 1] = offset[i] + elementSize;

        for (unsigned int j = 0; j < elementSize; j++)
        {
            unsigned int vertex;
            in >> vertex;
            elements.push_back(vertex);
        }
    }

    for (unsigned int i = 0; i < elements.size(); i++)
    {
        printf("%u: %u\n", i, elements[i]);
    }
    std::cout << std::endl;

    // Print the mesh.
    for (unsigned int i = 0; i < numberElements; i++)
    {
        printf("%u: ", i);
        for (unsigned int j = offset[i]; j < offset[i + 1]; j++)
        {
            printf("%u ", elements[j]);
        }
        std::cout << std::endl;
    }

    return new IBHM(coordinates, elements, offset, 2);
}



void insertInterfaceElementsCHE(CHE *che)
{
    if (che == nullptr)
    {
        return;
    }

    const std::vector<unsigned int> edges = {28, 29, 32, 19, 36, 23, 11, 40, 27, 43};
    CHEInterfaceElementBuilder op(che);
    unsigned int count = 0;
    for (const unsigned int edge: edges)
    {
        const unsigned int numNewVertices = op.computeNumberOfNewVertices({edge});
        count += numNewVertices;
        printf("Number of new vertices: %u\n", numNewVertices);
        op.insertInterfaceElements({edge});
    }
    che->print();
    printf("Total number of new vertices: %u\n", count);

    std::cout << std::endl;
}



//
// void insertInterfaceElementsIBHM(IBHM *ibhm)
// {
//     if (ibhm == nullptr)
//     {
//         return;
//     }
//
//     const std::vector<unsigned int> edges = {28, 29, 32, 19, 36, 23, 11, 40, 27, 43};
//     InterfaceElementOperators op(ibhm);
//     unsigned int count = 0;
//     for (const unsigned int edge: edges)
//     {
//         const unsigned int numNewVertices = op.computeNumberOfNewVertices({edge});
//         count += numNewVertices;
//         printf("Number of new vertices: %u\n", numNewVertices);
//         op.insertInterfaceElements({edge});
//     }
//     ibhm->print();
//     printf("Total number of new vertices: %u\n", count);
//
//     std::cout << std::endl;
// }



int main(int argc, char **argv)
{
    CHE *che = readMesh("../malha2.txt");
    insertInterfaceElementsCHE(che);
    delete che;


    // IBHM *ibhm = readHybridMesh("../malha2.txt");
    // insertInterfaceElementsIBHM(ibhm);

    // delete ibhm;

    return 0;
}

