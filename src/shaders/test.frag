#version 330 core

uniform sampler2D texture0;

in vec2 vTexCoord;
in vec4 vColor;

out vec4 fragColor;

void main() {
    fragColor = texture(texture0, vTexCoord) * vColor;
}
