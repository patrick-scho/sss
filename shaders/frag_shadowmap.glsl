
#version 330 core

in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos;

void main()
{
  float lightDist = 1 - (length(lightPos - FragPos) - 5.5);
  FragColor = vec4(vec3(lightDist), 1);
}
