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

uniform float translucencySampleVariances[6];
uniform vec3 translucencySampleWeights[6];

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

  //float distanceToBackside = length(clamp(FragPos - Backside, vec3(0), vec3(1000)));
  float distanceToBackside = length(FragPos - Backside);
  //distanceToBackside = distance(Backside, LocalPos);
  vec3 result = (ambient + diffuse + specular) * objectColor;

  if (renderState == 3) {
    if (distanceToBackside != 0) {
      result += objectColor * pow(powBase, powFactor / pow(distanceToBackside, 0.6)) * transmittanceScale * (1 - diff);
      // vec3 translucency = vec3(0);
      // for (int i = 0; i < 6; i++) {
      //   translucency += objectColor * translucencySampleWeights[i] * exp(-pow(distanceToBackside, 2.0) / translucencySampleVariances[i]);
      // }

      // result += translucency * transmittanceScale;
    }
  }
      //result += objectColor * pow(powBase, -pow(distanceToBackside, 2)) * transmittanceScale * (1 - diff);
  // if (renderState == 3) {
  //   //result = Backside;
  //   //result = LocalPos;
  //   result = vec3(distanceToBackside);
  // }

    
  FragColor = vec4(result, 1.0f);
  //FragColor = vec4(vec3(distanceToBackside), 1);
}
