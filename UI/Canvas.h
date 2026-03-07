//
// Created by Jeferson Coelho on 12/07/25.
//

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QMatrix4x4>
#include <QVector2D>
#include <QPoint>
#include "Vector2D.h"
class QuadMesh;
class QOpenGLShaderProgram;


class Canvas final : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
public:
    explicit Canvas(QWidget *parent = nullptr);

    ~Canvas() override;

    void initializeGL() override;

    void paintGL() override;

    void resizeGL(int width, int height) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /**
     * Adjust canvas based on W and H factors.
     * @param factorW - W factor.
     * @param factorH - H factor.
     */
    void adjustCanvas(float factorW, float factorH);

    /**
     * Fit the world to an object AABB.
     * @param minX - Object x minimum.
     * @param minY - Object y minimum.
     * @param maxX - Object x maximum.
     * @param maxY - Object y maximum.
     */
    void fitViewToBoundingBox(float minX, float minY, float maxX, float maxY);

private:
    /**
     * @brief Converts from the canvas coordinate system (screen) to the world coordinate system.
     * @param screen Point on canvas coordinate system.
     * @return point to the world coordinate system.
     * @see Point2Df
     */
    [[nodiscard]] Point2Df convertFromScreenToWorld(const Point2Df &screen) const;

    /**
     * Create a program to render a mesh.
     * @return - True if the program could be created, compiled and linked.
     */
    bool createMeshProgram();

    /**
     * Create a program to render a point set.
     * @return - True if the program could be created, compiled and linked.
     */
    bool createPointsProgram();

    /**
     * Create a program to render lines.
     * @return - True if the program could be created, compiled and linked.
     */
    bool createLinesProgram();

    /**
     * Set up the point set to be rendered.
     * @param points - The points' coordinates to be rendered.
     */
    void setupPointsToRender(const std::vector<Point2Df> &points);

    /**
     * Set up the polylines to be rendered.
     * @param points - The points' coordinates to be rendered as polyline.
     */
    void setupPolylinesToRender(const std::vector<Point2Df> &points);

    /**
     * Check OpenGL errors.
     */
    void checkRenderingError();

private:
    /**
     * Render the mesh.
     */
    QOpenGLShaderProgram *_meshProgram = nullptr;

    /**
     * Render the points.
     */
    QOpenGLShaderProgram *_pointsProgram = nullptr;

    /**
     * Render lines.
     */
    QOpenGLShaderProgram *_linesProgram = nullptr;

    /**
     * OpenGL resources ID.
     */
    GLuint _meshVAO = 0, _meshVBO = 0, _meshEBO = 0;
    GLuint _pointsVAO = 0, _pointsVBO = 0;
    GLuint _linesVAO = 0, _linesVBO = 0;
    GLsizei _numPolyPoints = 0;

    /**
     * Wireframe texture id.
     */
    GLuint _wireframeTexture = 0;

    /**
     * Matrices to control the projection.
     */
    QMatrix4x4 view;
    QMatrix4x4 proj;

    /**
     * World window dimensions.
     */
    QVector2D _min = {-1.0f, -1.0f}, _max = {+1.0f, +1.0f};

    /**
     * Stores the last mouse position.
     */
    QPoint _lastMousePos;

    /**
     * Stores the index of the selected point to move it.
     */
    std::pair<int, int> _selectedPointIndex = {-1, -1};

private:
};

