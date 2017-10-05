#version 330 core

uniform sampler2D texture0;

in vec2 vTexCoord;

out vec4 fragColor;

void main() {
    fragColor = texture(texture0, vTexCoord);
}
