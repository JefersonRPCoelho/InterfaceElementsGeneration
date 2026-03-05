//
// Created by Jeferson Coelho on 21/08/25.
//

#define GL_SILENCE_DEPRECATION

#include "Utilities.h"
#include <QOpenGLShader>
#include <filesystem>

namespace Utilities
{
    void addShader(QOpenGLShaderProgram *program, const std::string &shaderFile)
    {
        // Get the file extension.
        const std::filesystem::path p(shaderFile);
        const std::string ext = p.extension().string();

        // Get the shader type.
        QOpenGLShader::ShaderTypeBit shaderType = QOpenGLShader::Vertex;

        if(ext == ".frag")
        {
            shaderType = QOpenGLShader::Fragment;
        }
        else if(ext == ".geom")
        {
            shaderType = QOpenGLShader::Geometry;
        }

        // Add the shader to the program.
        if(program->addShaderFromSourceFile(shaderType, shaderFile.c_str()))
        {
            printf(GREEN_COLOR "Shader %s was successfully added.\n" RESET_COLOR, shaderFile.c_str());
        }
        else
        {
            printf(RED_COLOR "Error adding shader %s!\n" RESET_COLOR, shaderFile.c_str());
        }
    }



    GLuint createWireFrameTexture()
    {
        // Create a texture slot.
        GLuint wireframeId;
        glGenTextures(1, &wireframeId);

        // Bind the texture slot.
        glBindTexture(GL_TEXTURE_2D, wireframeId);

        std::vector<unsigned char> texture(1024, 0);
        texture[1022] = 80;
        texture[1023] = 255;

        //Send the image of a level i to OpenGL.
        int i = 0, j = 0;
        const int TAM = static_cast<int>(texture.size());
        for(i = 0, j = TAM; j > 4; j /= 2, i++)
        {
            glTexImage2D(GL_TEXTURE_2D, i, GL_RED, j, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &texture[0] + (TAM - j));
        }

        //To avoid saturation.
        texture[TAM - 2] /= 3;
        texture[TAM - 1] /= 3;

        for(; j > 0; j /= 2, i++)
        {
            glTexImage2D(GL_TEXTURE_2D, i, GL_RED, j, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &texture[0] + (TAM - j));
        }

        // Define filters.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

        return wireframeId;
    }
}
