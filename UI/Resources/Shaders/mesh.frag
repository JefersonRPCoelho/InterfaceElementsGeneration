#version 330 core
uniform sampler2D wireFrameTexture;
uniform vec3 color;
uniform vec3 colorwire;

out vec4 finalColor;
in float wireFrameTexCoord00;
in float wireFrameTexCoord01;
in float wireFrameTexCoord10;
in float wireFrameTexCoord20;

float wireFrameAlpha[3];

void main()
{
    finalColor = vec4(color, 0.0);
    wireFrameAlpha[0] = texture(wireFrameTexture, vec2(max(wireFrameTexCoord00, wireFrameTexCoord01), 0)).r;
    wireFrameAlpha[1] = texture(wireFrameTexture, vec2(wireFrameTexCoord10), 0).r;
    wireFrameAlpha[2] = texture(wireFrameTexture, vec2(wireFrameTexCoord20), 0).r;

    vec4 wireFrameColor = vec4(colorwire, 0); //Preto
    finalColor = mix(finalColor, wireFrameColor, wireFrameAlpha[0]);
    finalColor = mix(finalColor, wireFrameColor, wireFrameAlpha[1]);
    finalColor = mix(finalColor, wireFrameColor, wireFrameAlpha[2]);

    finalColor.a = mix(finalColor.a, 1, wireFrameAlpha[0]);
    finalColor.a = mix(finalColor.a, 1, wireFrameAlpha[1]);
    finalColor.a = mix(finalColor.a, 1, wireFrameAlpha[2]);

    //    finalColor = vec4(1, 0, 0, 1);
}


