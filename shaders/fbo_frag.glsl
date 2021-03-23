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

vec4 blur(sampler2D tex, vec2 uv, vec2 res) {
  float Pi = 6.28318530718; // Pi*2
    
  // GAUSSIAN BLUR SETTINGS {{{
  float Directions = 16.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
  float Quality = 4.0; // BLUR QUALITY (Default 4.0 - More is better but slower)
  float Size = 8.0; // BLUR SIZE (Radius)
  // GAUSSIAN BLUR SETTINGS }}}
  
  vec2 Radius = Size/res;
  
  // Pixel colour
  vec4 Color = texture(tex, uv);
  
  // Blur calculations
  for( float d=0.0; d<Pi; d+=Pi/Directions) {
    for(float i=1.0/Quality; i<=1.0; i+=1.0/Quality) {
      Color += texture( tex, uv+vec2(cos(d),sin(d))*Radius*i);		
    }
  }
  
  // Output to screen
  Color /= Quality * Directions - 15.0;
  return Color;
}

void main()
{
    if (renderState == 0) {
        FragColor = blur(shadowmapTexture, TexCoords, vec2(screenWidth, screenHeight));
    }
    else if (renderState == 1) {
        FragColor = texture(shadowmapTexture, TexCoords);
    }
    // stencil buffer
    else if (renderState == 2) {
        FragColor = texture(irradianceTexture, TexCoords);
    }
    else if (renderState == 3) {
        vec4 result = vec4(0, 0, 0, 1);
        for (int i = 0; i < 13; i++) {
            vec2 sampleCoords = TexCoords + samplePositions[i] * vec2(1.0/screenWidth, 1.0/screenHeight);
            //vec4 sample = texture(irradianceTexture, sampleCoords)
            //            * texture(shadowmapTexture, sampleCoords);
            vec4 sample = texture(irradianceTexture, sampleCoords);
            vec4 weight = vec4(sampleWeights[i], 1);
            result += sample * weight;
        }
        FragColor = result;
    }
}
