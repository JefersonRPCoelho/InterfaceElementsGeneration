#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
out float wireFrameTexCoord00;
out float wireFrameTexCoord01;
out float wireFrameTexCoord10;
out float wireFrameTexCoord20;

void main()
{
    gl_Position = gl_in[0].gl_Position;
    wireFrameTexCoord00 = 1.0;
    wireFrameTexCoord01 = 0.0;
    wireFrameTexCoord10 = 1.0;
    wireFrameTexCoord20 = 0.0;
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    wireFrameTexCoord00 = 0.0;
    wireFrameTexCoord01 = 0.0;
    wireFrameTexCoord10 = 1.0;
    wireFrameTexCoord20 = 1.0;
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    wireFrameTexCoord00 = 0.0;
    wireFrameTexCoord01 = 1.0;
    wireFrameTexCoord10 = 0.0;
    wireFrameTexCoord20 = 1.0;
    EmitVertex();

    EndPrimitive();
}
