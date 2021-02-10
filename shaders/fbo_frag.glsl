#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D shadowmapTexture;
uniform sampler2D irradianceTexture;
uniform int screenWidth;
uniform int screenHeight;
uniform int renderState;
uniform vec2 samplePositions[13];
uniform vec3 sampleWeights[13];

void main()
{
    if (renderState == 0) {
        FragColor = texture(shadowmapTexture, TexCoords);
    }
    // stencil buffer
    else if (renderState == 1 || texture(irradianceTexture, TexCoords).rgb == vec3(0, 0, 0)) {
        FragColor = texture(irradianceTexture, TexCoords);
    }
    else if (renderState == 2) {
        FragColor = texture(shadowmapTexture, TexCoords) * texture(irradianceTexture, TexCoords);
    }
    else if (renderState == 3) {
        vec4 result = vec4(0, 0, 0, 1);
        for (int i = 0; i < 13; i++) {
            float oneX = 1.0/screenWidth;
            float oneY = 1.0/screenHeight;
            vec4 sample = texture(irradianceTexture, TexCoords + samplePositions[i] * vec2(oneX, oneY));
            vec4 weight = vec4(sampleWeights[i], 1);
            result += sample * weight;
        }
        FragColor = result;
    }
}
