#version 330 core

uniform mat3 MVP;

layout (location = 0) in mat3 aTransform;
layout (location = 3) in vec2 aPos;
layout (location = 4) in vec2 aTexCoord;
layout (location = 5) in vec4 aColor;
layout (location = 6) in vec4 aTint;

out vec2 vTexCoord;
out vec4 vColor;
out vec4 vTint;

void main() {
    gl_Position = vec4(MVP * aTransform * vec3(aPos, 1), 1);
    vTexCoord = aTexCoord;
    vColor = aColor;
    vTint = aTint;
}