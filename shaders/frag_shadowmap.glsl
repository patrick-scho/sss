
#version 330 core

in vec3 FragPos;
in vec3 LocalPos;

out vec4 FragColor;

uniform vec3 lightPos;

void main()
{
  float lightDist = length(lightPos - FragPos);
  float c1 = lightDist;
  float c2 = lightDist;
  float c3 = lightDist;
  FragColor = vec4(c1, c2, c3, 1);
  //FragColor = vec4(LocalPos/10, 1);
}
