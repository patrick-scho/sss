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

void main()
{
  gl_Position = projection * view * model * vec4(pos, 1.0);
  // calculate fragment position in world coordinates
  FragPos = vec3(model * vec4(pos, 1));
  // and local coordinates
  LocalPos = pos;

  Normal = normal;
  
  // get fragment position in the light's projection space
  vec4 lightSpace = lightProjection * lightView * model * vec4(pos, 1.0);
  // and transform them to 2D coordinates
  // (this is usually done by OpenGL after applying the vertex shader,
  // so to get them here, we have to divide by w manually)
  lightSpace = lightSpace / lightSpace.w;
  vec2 shadowmapCoords = lightSpace.xy;
  // map coordinates from [0 1] to [-1 +1]
  // multiply by 0.99 first to shift coordinates towards the center slightly
  // to prevent artifacts at the edges
  shadowmapCoords = vec2(
    (shadowmapCoords.x * 0.99 + 1) / 2,
    (shadowmapCoords.y * 0.99 + 1) / 2
  );

  // sample shadowmap (brightness encodes distance of fragment to light)
  vec4 t = texture(shadowmapTexture, shadowmapCoords);

  BacksideIrradiance = t.r;
  
  // calculate backside with distance(BacksideIrradiance) and lightDir
  vec3 lightDir = normalize(FragPos - lightPos);
  Backside = (lightPos + (lightDir * BacksideIrradiance));
}
