#version 330 core

uniform sampler2D texture0;

in vec2 vTexCoord;
in vec4 vColor;
in vec4 vTint;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(texture0, vTexCoord);
    // Pre-multiply alpha
    texColor = vec4(texColor.rgb * texColor.a, texColor.a);
    vec4 tint = vec4(vTint.rgb * vTint.a, vTint.a);

    fragColor = vec4(texColor.rgb + tint.rgb * texColor.a, texColor.a) * vColor;
}
