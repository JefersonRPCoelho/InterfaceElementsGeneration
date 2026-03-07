//
// Created by Jeferson Coelho on 12/07/25.
//

#define GL_SILENCE_DEPRECATION

#include <QOpenGLShaderProgram>
#include <filesystem>
#include <QtEvents>

#include "Utilities.h"
#include "Canvas.h"



Canvas::Canvas(QWidget *parent): QOpenGLWidget(parent)
{
}



Canvas::~Canvas()
{
    makeCurrent();

    if (_meshVAO)
    {
        glDeleteVertexArrays(1, &_meshVAO);
    }
    if (_meshVBO)
    {
        glDeleteBuffers(1, &_meshVBO);
    }
    if (_meshEBO)
    {
        glDeleteBuffers(1, &_meshEBO);
    }
    if (_wireframeTexture)
    {
        glDeleteTextures(1, &_wireframeTexture);
    }

    delete _meshProgram;

    doneCurrent();
}



void Canvas::fitViewToBoundingBox(const float minX, const float minY, const float maxX, const float maxY)
{
    // AABB dimensions.
    const float boxWidth = maxX - minX;
    const float boxHeight = maxY - minY;

    // AABB center.
    const float cx = (minX + maxX) * 0.5f;
    const float cy = (minY + maxY) * 0.5f;

    // View port aspect ratio.
    const float aspect = static_cast<float>(width()) / static_cast<float>(height());

    float halfWidth = 1.0f, halfHeight = 1.0f;

    if ((boxWidth / boxHeight) > aspect)
    {
        halfWidth = boxWidth * 0.5f;
        halfHeight = (boxWidth / aspect) * 0.5f;
    }
    else
    {
        halfHeight = boxHeight * 0.5f;
        halfWidth = boxHeight * aspect * 0.5f;
    }

    proj.setToIdentity();
    proj.ortho(cx - halfWidth, cx + halfWidth, cy - halfHeight, cy + halfHeight, -1.0f, 1.0f);

    // Redraw.
    update();
}



void Canvas::adjustCanvas(const float factorW, const float factorH)
{
    // Get the world center.
    const float cx = (_min.x() + _max.x()) * 0.5f;
    const float cy = (_min.y() + _max.y()) * 0.5f;

    // Get the current world dimensions.
    float widthWorld = fabs(_max.x() - _min.x());
    float heightWorld = fabs(_max.y() - _min.y());

    // Adjust for the new dimensions.
    widthWorld *= factorW;
    heightWorld *= factorH;

    // Recompute the world limits.
    _min.setX(cx - 0.5f * widthWorld);
    _max.setX(cx + 0.5f * widthWorld);
    _min.setY(cy - 0.5f * heightWorld);
    _max.setY(cy + 0.5f * heightWorld);

    // Adjust projection.
    proj.setToIdentity();
    proj.ortho(_min.x(), _max.x(), _min.y(), _max.y(), -1, 1);
}



void Canvas::initializeGL()
{
    QOpenGLWidget::initializeGL();
    initializeOpenGLFunctions();
    makeCurrent();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, width(), height());

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    createMeshProgram();
    createPointsProgram();
    createLinesProgram();
}



void Canvas::paintGL()
{
    glClearColor(0.5, 0.5, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    _meshProgram->bind();
    glBindVertexArray(_meshVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _wireframeTexture);
    _meshProgram->setUniformValue("transformMatrix", proj * view);

    _meshProgram->release();

    _linesProgram->bind();
    glBindVertexArray(_linesVAO);
    _linesProgram->setUniformValue("transformMatrix", proj * view);

    _linesProgram->release();

    if (_numPolyPoints > 0)
    {
        _pointsProgram->bind();
        glBindVertexArray(_pointsVAO);

        _pointsProgram->setUniformValue("transformMatrix", proj * view);
        _pointsProgram->setUniformValue("color", 1.0f, 0.0f, 0.0f);

        glPointSize(6.0f);
        glDrawArrays(GL_POINTS, 0, _numPolyPoints);

        glBindVertexArray(0);
        _pointsProgram->release();
    }


    checkRenderingError();
}



void Canvas::resizeGL(const int width, const int height)
{
    // Define the new view port.
    glViewport(0, 0, width, height);

    // Get the window aspect ratio.
    const float aspect = static_cast<float>(width) / static_cast<float>(height);

    // Compute the world dimensions to keep the aspect ratio in the new window dimensions.
    float worldWidth = fabs(_max.x() - _min.x());
    float worldHeight = fabs(_max.y() - _min.y());

    // Adjust the world dimension.
    if (aspect > 1.0f)
    {
        // Window increases the width. Increases the world as well.
        worldWidth = worldHeight * aspect;
    }
    else
    {
        // Window increases the height. Increases the world as well.
        worldHeight = worldWidth / aspect;
    }

    // Recompute the center.
    const float cx = (_min.x() + _max.x()) * 0.5f;
    const float cy = (_min.y() + _max.y()) * 0.5f;

    _min.setX(cx - 0.5f * worldWidth);
    _max.setX(cx + 0.5f * worldWidth);
    _min.setY(cy - 0.5f * worldHeight);
    _max.setY(cy + 0.5f * worldHeight);

    // Update the projection matrix.
    proj.setToIdentity();
    proj.ortho(_min.x(), _max.x(), _min.y(), _max.y(), -1, 1);
}



void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::MiddleButton)
    {
        std::cout << "Mouse move event: " << event->buttons() << std::endl;

        // Displacement in pixels.
        const QPoint delta = event->pos() - _lastMousePos;
        _lastMousePos = event->pos();

        // Convert the displacement to the world coordinates.
        const float widthWorld = _max.x() - _min.x();
        const float heightWorld = _max.y() - _min.y();

        const float dx = -delta.x() * (widthWorld / static_cast<float>(width()));
        const float dy = delta.y() * (heightWorld / static_cast<float>(height()));

        // Adjust world dimensions.
        _min.setX(_min.x() + dx);
        _max.setX(_max.x() + dx);
        _min.setY(_min.y() + dy);
        _max.setY(_max.y() + dy);

        // Update projection.
        proj.setToIdentity();
        proj.ortho(_min.x(), _max.x(), _min.y(), _max.y(), -1, 1);
    }

    else if (event->buttons() & Qt::LeftButton)
    {
    }

    // Redraw.
    update();
}



Point2Df Canvas::convertFromScreenToWorld(const Point2Df &screen) const
{
    //Get parameters.
    const float tx = screen.x() / (static_cast<float>(width()) - 1.0f);
    const float ty = 1.0f - screen.y() / (static_cast<float>(height()) - 1.0f);

    Point2Df result;
    result[0] = (1.0f - tx) * _min.x() + tx * _max.x();
    result[1] = (1.0f - ty) * _min.y() + ty * _max.y();

    return result;
}



bool Canvas::createMeshProgram()
{
    _meshProgram = new QOpenGLShaderProgram();

    // Add shader files to the mesh program.
    Utilities::addShader(_meshProgram, ":/shaders/mesh.vert");
    Utilities::addShader(_meshProgram, ":/shaders/mesh.geom");
    Utilities::addShader(_meshProgram, ":/shaders/mesh.frag");

    // Link the mesh program.
    _meshProgram->link();
    if (_meshProgram->isLinked())
    {
        printf(GREEN_COLOR "Mesh program was successfully linked.\n" RESET_COLOR);
    }
    else
    {
        printf(RED_COLOR "Error linking mesh program!\n" RESET_COLOR);
        return false;
    }

    _meshProgram->bind();
    _wireframeTexture = Utilities::createWireFrameTexture();
    _meshProgram->setUniformValue("wireframeTexture", 0);
    _meshProgram->release();

    return true;
}



bool Canvas::createPointsProgram()
{
    // Create a program to render points.
    _pointsProgram = new QOpenGLShaderProgram();

    // Add shader files to the points' program.
    Utilities::addShader(_pointsProgram, ":/shaders/points.vert");
    Utilities::addShader(_pointsProgram, ":/shaders/points.frag");

    // Link the point program.
    _pointsProgram->link();
    if (_pointsProgram->isLinked())
    {
        printf(GREEN_COLOR "Points program was successfully linked.\n" RESET_COLOR);
    }
    else
    {
        printf(RED_COLOR "Error linking points program!\n" RESET_COLOR);
        return false;
    }

    return true;
}



bool Canvas::createLinesProgram()
{
    // Create a program to render lines.
    _linesProgram = new QOpenGLShaderProgram();

    // Add shader files to the lines' program.
    Utilities::addShader(_linesProgram, ":/shaders/points.vert");
    Utilities::addShader(_linesProgram, ":/shaders/points.frag");

    // Link program.
    _linesProgram->link();
    if (_linesProgram->isLinked())
    {
        printf(GREEN_COLOR "Lines program was successfully linked.\n" RESET_COLOR);
    }
    else
    {
        printf(RED_COLOR "Error linking lines program!\n" RESET_COLOR);
        return false;
    }

    _linesProgram->bind();
    _linesProgram->setUniformValue("color", 0.0f, 0.0f, 1.0f);
    _linesProgram->release();

    return true;
}



void Canvas::setupPointsToRender(const std::vector<Point2Df> &points)
{
    makeCurrent();
    if (_pointsVAO == 0)
    {
        glGenVertexArrays(1, &_pointsVAO);
        glGenBuffers(1, &_pointsVBO);
    }

    glBindVertexArray(_pointsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(points.size() * sizeof(Point2Df)), &points[0],
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    doneCurrent();
}



void Canvas::setupPolylinesToRender(const std::vector<Point2Df> &points)
{
    makeCurrent();
    if (_linesVAO == 0)
    {
        glGenVertexArrays(1, &_linesVAO);
        glGenBuffers(1, &_linesVBO);
    }

    glBindVertexArray(_linesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _linesVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(points.size() * sizeof(Point2Df)), &points[0],
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    doneCurrent();
}



void Canvas::checkRenderingError()
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        switch (err)
        {
            case GL_INVALID_ENUM:
                qDebug() << "OpenGL error: GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                qDebug() << "OpenGL error: GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                qDebug() << "OpenGL error: GL_INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                qDebug() << "OpenGL error: GL_OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                qDebug() << "OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:
                qDebug() << "OpenGL error (unknown):" << err;
                break;
        }
    }
}



void Canvas::mousePressEvent(QMouseEvent *event)
{
    _lastMousePos = event->pos();
    if (event->button() == Qt::LeftButton)
    {
        const auto world = convertFromScreenToWorld(Point2Df(static_cast<float>(event->localPos().x()),
                                                             static_cast<float>(event->localPos().y())));
    }
    else if (event->button() == Qt::RightButton)
    {
    }
}



void Canvas::wheelEvent(QWheelEvent *event)
{
    // Get the scroll direction.
    const int delta = event->angleDelta().y();

    // Define the zoom factor.
    constexpr float zoomFactor = 1.1f; // 10% per notch
    if (delta > 0)
    {
        // Scroll up → zoom in
        adjustCanvas(1.0f / zoomFactor, 1.0f / zoomFactor);
    }
    else if (delta < 0)
    {
        // Scroll down → zoom out
        adjustCanvas(zoomFactor, zoomFactor);
    }

    // Redraw.
    update();
}



void Canvas::keyPressEvent(QKeyEvent *event)
{
    QOpenGLWidget::keyPressEvent(event);
}



void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
    }
}

