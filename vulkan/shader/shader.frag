#version 440

#define VIEWPOINTS 8

layout (location=0) in vec2 texCoord;
layout (location=0) out vec4 fragColor;

layout (binding=0) uniform sampler2D bgndTexture; //Tecture to use when hasAlpha is true
layout (binding=1) uniform sampler2D srcTextures[VIEWPOINTS]; //VIEWPOINTS match the number of Viewpoints and is defined has a GLOBAL

layout (binding=2) uniform block {
    vec2 position; //Quad current position
    float displayHeight; //Window Height
    bool hasAlpha; //True if the pixel has alpha value
    float brightPos;
};

void main()
{
    /////////////////////////////////////
    //   SWITCH BETWEEN THIS EFFECTS   //
    /////////////////////////////////////
    // Original
    //fragColor = textureLod(bgndTexture, texCoord, 0);
    //fragColor.xy += texCoord * 0.2;
    fragColor = texture(srcTextures[int(gl_FragCoord.x + gl_FragCoord.y) % VIEWPOINTS], texCoord) * max(1, 1.2 - abs(gl_FragCoord.y - brightPos) * 0.003);

    // Color Overlay
    //fragColor = texture(texImage, fragCoord) * vec4(colorOverlay, 1.0);

    // Negate image
    //vec4 col = texture(texImage, fragCoord);
    //fragColor = vec4(1 -col.r, 1-col.g, 1-col.b, col.a);
}
