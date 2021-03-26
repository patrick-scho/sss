#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;

out vec3 FragPos;
out vec3 LocalPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 lightView;
uniform mat4 projection;

void main()
{
  gl_Position = projection * lightView * model * vec4(pos, 1.0);
  // pass fragment position in world coordinates
  FragPos = vec3(model * vec4(pos, 1));
  LocalPos = pos;
  Normal = normal;
}
