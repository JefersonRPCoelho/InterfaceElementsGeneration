//
// Created by Jeferson Coelho on 21/08/25.
//

#pragma once
#include <string>
#include <QOpenGLShader>

class QOpenGLShaderProgram;

#ifdef _WIN32
#define RESET_COLOR "\033[0m"
#define RED_COLOR   "\033[31m"
#define GREEN_COLOR "\033[32m"
#else
#define RESET_COLOR
#define RED_COLOR
#define GREEN_COLOR
#endif

namespace Utilities
{
    /**
    * Add a shader file to a program.
    * @param program - The program the shader should be added.
    * @param shaderFile - The file containing the shader program to be added.
    */
    void addShader(QOpenGLShaderProgram *program, const std::string &shaderFile);

    /**
     * Create texture to render wireframe.
     */
    GLuint createWireFrameTexture();
}
