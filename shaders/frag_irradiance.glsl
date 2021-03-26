#version 330 core

in vec3 FragPos;
in vec3 LocalPos;
in vec3 Backside;
in float BacksideIrradiance;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float transmittanceScale;
uniform int renderState;
uniform float powBase;
uniform float powFactor;

void main()
{
  // phong lighting
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

  vec3 result = (ambient + diffuse + specular) * objectColor;

  // thickness
  float distanceToBackside = length(FragPos - Backside);

  if (distanceToBackside != 0) {
    // add translucency by amplifying color inverse to the thickness
    // (1 - diff) is part of the irradiance term,
    // if the light hits the object straight at 90°
    // most light is received
    result += objectColor * pow(powBase, powFactor / pow(distanceToBackside, 0.6)) * transmittanceScale * (1 - diff);
  }
    
  FragColor = vec4(result, 1.0f);
}
