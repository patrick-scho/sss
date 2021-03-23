#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;

out vec3 FragPos;
out vec3 LocalPos;
out vec3 Backside;
out float BacksideIrradiance;
out vec3 Normal;

uniform sampler2D shadowmapTexture;
uniform vec3 lightPos;
uniform vec2 samplePositions[13];
uniform vec3 sampleWeights[13];
uniform int screenWidth;
uniform int screenHeight;

uniform mat4 model;
uniform mat4 view;
uniform mat4 lightView;
uniform mat4 lightViewInv;
uniform mat4 projection;
uniform mat4 lightProjection;

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
  gl_Position = projection * view * model * vec4(pos, 1.0);
  FragPos = vec3(model * vec4(pos, 1));
  LocalPos = pos;
  Normal = normal;
  
  vec4 lightSpace = lightProjection * lightView * model * vec4(pos, 1.0);
  lightSpace = lightSpace / lightSpace.w;
  vec2 shadowmapCoords = lightSpace.xy;
  shadowmapCoords = vec2(
    (shadowmapCoords.x * 0.99 + 1) / 2,
    (shadowmapCoords.y * 0.99 + 1) / 2
  );


  // blur
  vec4 t = texture(shadowmapTexture, shadowmapCoords);
  //vec4 t = blur(shadowmapTexture, shadowmapCoords, vec2(screenWidth, screenHeight));

  BacksideIrradiance = t.r + t.g*10 + t.b*100;
  
  vec3 lightDir = (vec3(0, 0, 0) - lightPos);
  Backside = (lightPos + (lightDir * BacksideIrradiance));
  //Backside = texture(shadowmapTexture, shadowmapCoords).xyz*10;
}
