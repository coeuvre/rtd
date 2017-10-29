#version 330 core

uniform mat3 MVP;

layout (location = 0) in mat3 aTransform;
layout (location = 3) in vec2 aPos;
layout (location = 4) in vec2 aTexCoord;
layout (location = 5) in vec4 aColor;
layout (location = 6) in vec2 aRoundRadius;
layout (location = 7) in vec2 aThickness;
layout (location = 8) in vec4 aBorderColor;

out vec2 vTexCoord;
out vec4 vColor;
out vec2 vRoundRadius;
out vec2 vThickness;
out vec4 vBorderColor;

void main() {
    gl_Position = vec4(MVP * aTransform * vec3(aPos, 1), 1);
    vTexCoord = aTexCoord;
    vColor = aColor;
    vRoundRadius = aRoundRadius;
    vThickness = aThickness;
    vBorderColor = aBorderColor;
}