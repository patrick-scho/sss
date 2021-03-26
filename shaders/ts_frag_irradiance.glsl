#version 330 core

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

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
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(lightPos - FragPos);

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * lightColor;

  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * lightColor;

  float specularStrength = 0.5;
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
  vec3 specular = specularStrength * spec * lightColor;

  vec3 result = (ambient + diffuse) * objectColor;

  result = vec3(length(FragPos - lightPos));
    
  FragColor = vec4(result, 1.0f);
}
