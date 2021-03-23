
#version 330 core

in vec3 FragPos;
in vec3 LocalPos;

out vec4 FragColor;

uniform vec3 lightPos;

void main()
{
  float lightDist = length(lightPos - FragPos);
  float c1 = mod(lightDist, 10);
  float c2 = mod(lightDist/10, 10);
  float c3 = mod(lightDist/100, 10);
  FragColor = vec4(c1, c2, c3, 1);
  //FragColor = vec4(LocalPos/10, 1);
}
