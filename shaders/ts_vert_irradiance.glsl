#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
  // lay out the model in the XY-plane according to it's UV coordinates
  gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
  // pass fragment position in world coordinates
  FragPos = vec3(model * vec4(pos, 1.0));
  Normal = normal;
}
