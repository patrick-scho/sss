#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int applySSSSS;
uniform int N;

void main()
{
    if (applySSSSS == 1) {
        float x = 1.0/1600.0;
        float y = 1.0/900.0;

        float maxDist = N*N + N*N;

        vec4 color = vec4(0, 0, 0, 1);
        for (int i = -N; i <= N; i++) {
            for (int j = -N; j <= N; j++) {
                float dist = i*i + j*j;
                vec4 newC = texture(screenTexture, TexCoords + vec2(i*x, j*y)) / (2*N*N);
                float factor = 1 - (dist / maxDist);
                factor = pow(factor, 2);
                color += newC * factor;
            }
        }
        FragColor = color;
    }
    else {
        FragColor = texture(screenTexture, TexCoords);
    }
}
