#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec2 UV;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;
uniform sampler2D irradianceTexture;
uniform int screenWidth;
uniform int screenHeight;
uniform int renderState;
uniform vec2 samplePositions[13];
uniform vec3 sampleWeights[13];
uniform float transmittanceScale;

void main()
{
  if (renderState == 0) {
    FragColor = texture(irradianceTexture, UV);
  }
  else if (renderState == 1) {
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
      
    FragColor = vec4(result, 1.0);
  }
  else if (renderState == 2) {
    vec4 result = vec4(0, 0, 0, 1);
    for (int i = 0; i < 13; i++) {
        vec2 sampleCoords = UV + samplePositions[i] * vec2(1.0/screenWidth, 1.0/screenHeight);
        //vec4 sample = texture(irradianceTexture, sampleCoords)
        //            * texture(shadowmapTexture, sampleCoords);
        vec4 sample = texture(irradianceTexture, sampleCoords);
        vec4 weight = vec4(sampleWeights[i], 1);
        result += sample * weight;
    }
    FragColor = result;
  }
  else if (renderState == 3) {
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

    vec3 result = vec3((ambient + diffuse + specular) * objectColor);

    vec3 result2 = vec3(0, 0, 0);
    for (int i = 0; i < 13; i++) {
        vec2 sampleCoords = UV + samplePositions[i] * vec2(1.0/screenWidth, 1.0/screenHeight);
        //vec4 sample = texture(irradianceTexture, sampleCoords)
        //            * texture(shadowmapTexture, sampleCoords);
        vec3 sample = vec3(texture(irradianceTexture, sampleCoords));
        vec3 weight = sampleWeights[i];
        result2 += sample * weight;
    }

    result = sqrt(result * result2);
    
    vec4 t = texture(irradianceTexture, UV);

    float BacksideIrradiance = t.r; //*100 + t.g + t.b/100;
    
    vec3 Backside = (lightPos + (normalize(FragPos - lightPos) * BacksideIrradiance));
    
    float distanceToBackside = length(FragPos - Backside);
    if (distanceToBackside != 0)
      result += objectColor * exp(2 / pow(distanceToBackside, 0.6)) * transmittanceScale * (1 - diff);

    FragColor = vec4(result, 1);
  }
}
