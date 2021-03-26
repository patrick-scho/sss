#version 330 core

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

void main()
{
  // color the XY-plane according to the distance
  // of the fragment in world space
  vec3 result = vec3(length(FragPos - lightPos));
    
  FragColor = vec4(result, 1.0f);
}
