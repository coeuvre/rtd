#version 330 core

uniform sampler2D texture0;

in vec2 vTexCoord;
in vec4 vColor;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(texture0, vTexCoord);
    // Pre-multiply alpha
    texColor = vec4(texColor.rgb * texColor.a, texColor.a);

    fragColor = texColor * vColor;
}
