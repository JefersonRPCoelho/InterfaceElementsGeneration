//
// Created by Jeferson Coelho on 12/07/25.
//

#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMenuBar>
#include <QFileDialog>
#include <QSettings>
#include <QCheckBox>

#include <iostream>
#include <fstream>

#include "Canvas.h"
#include "MainWindow.h"
#include "IBHM.h"



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto *topLayout = new QHBoxLayout();

    auto *topControls = new QWidget();
    topControls->setLayout(topLayout);

    // OpenGL Canvas
    // _glCanvas = new Canvas();
    // _glCanvas->setMinimumSize(600, 600);

    auto *runButton = new QPushButton("Run");
    connect(runButton, &QPushButton::clicked, this, &MainWindow::runSlot);


    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(runButton);

    // Main layout
    auto *centralWidget = new QWidget();
    auto *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(topControls);
    // mainLayout->addWidget(_glCanvas, 1);
    mainLayout->addLayout(buttonsLayout);

    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    setWindowTitle("Solver Interface with OpenGL");

    // Menu to open a mesh file.
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    // Open action
    auto *openAction = new QAction(tr("&Open Mesh"), this);
    fileMenu->addAction(openAction);

    // Connect signal to slot.
    connect(openAction, &QAction::triggered, this, &MainWindow::openMesh);
}



MainWindow::~MainWindow()
{
    delete _ibhm;
}



void MainWindow::runSlot() const
{
}



void MainWindow::openMesh()
{
    // Use QSettings to store the last used directory.
    QSettings settings("MeshResearch", "SliceQuadMesh");

    // Get the last directory or se the system standard.
    QString lastDir = settings.value("lastCoordsDir", QDir::homePath()).toString();
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Mesh File"),
        QString(),
        tr("Coords Files (*.txt);;All files (*.*)")
    );

    if (fileName.isEmpty())
    {
        printf("Invalid mesh file %s!\n", fileName.toStdString().c_str());
        return;
    }

    // Save teh used directory.
    settings.setValue("lastCoordsDir", QFileInfo(fileName).absolutePath());

    printf("Reading mesh file: %s \n", fileName.toStdString().c_str());

    std::ifstream in(fileName.toStdString().c_str());
    if (!in.is_open())
    {
        std::cerr << "Error opening the file " << fileName.toStdString() << std::endl;
        return;
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

    delete _ibhm;
    _ibhm = new IBHM(coordinates, elements, offset, 2);
    _ibhm->print();
}

