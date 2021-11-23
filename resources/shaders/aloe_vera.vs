#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in mat4 aInstanceMatrix;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 TangentLightPos[4];
    vec3 TangentLightDirs[3];
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 lightDirs[3];
uniform vec3 lightPos[4]; // we have 4 lights on the scene
uniform vec3 viewPos;

void main() {
    vs_out.FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));
    for(int i = 0; i < 4; i++) {
            vs_out.TangentLightPos[i] = TBN * lightPos[i];
    }
    vs_out.TangentViewPos = TBN * viewPos;
    vs_out.TangentFragPos = TBN * vs_out.FragPos;

    vs_out.Normal = aNormal;

    for(int i = 0; i < 3; i++) {
            vs_out.TangentLightDirs[i] = TBN * lightDirs[i];
        }

    gl_Position = projection * view * aInstanceMatrix * vec4(aPos, 1.0);
}
