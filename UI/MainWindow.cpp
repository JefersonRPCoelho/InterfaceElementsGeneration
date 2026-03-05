//
// Created by Jeferson Coelho on 12/07/25.
//

#include "MainWindow.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <iostream>
#include <QMenuBar>
#include <QFileDialog>
#include <QSettings>
#include <QCheckBox>
#include "Canvas.h"



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

    printf("Opening mesh file: %s \n", fileName.toStdString().c_str());
}

