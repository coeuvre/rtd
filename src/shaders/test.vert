#version 330 core

uniform mat4 MVP;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec4 aTint;

out vec2 vTexCoord;
out vec4 vColor;
out vec4 vTint;

void main() {
    gl_Position = MVP * vec4(aPos, 1);
    vTexCoord = aTexCoord;
    vColor = aColor;
    vTint = aTint;
}