#version 330 core

in vec2 vTexCoord;
in vec4 vColor;
in vec2 vRoundRadius;
in vec2 vThickness;
in vec4 vBorderColor;

out vec4 fragColor;

void main() {
    vec4 color = vColor;
    vec4 borderColor = vBorderColor;

    if (vTexCoord.x <= vRoundRadius.x && vTexCoord.y <= vRoundRadius.y) {
        float x = vTexCoord.x - vRoundRadius.x;
        float y = vTexCoord.y - vRoundRadius.y;
        float a = vRoundRadius.x;
        float b = vRoundRadius.y;
        float dist = (x / a) * (x / a) + (y / b) * (y / b);
        float delta = fwidth(dist);
        float alpha = smoothstep(1 + delta, 1 - delta, dist);
        color.a = alpha;
        borderColor.a = alpha;
    }

    // Pre-multiply alpha
    color = vec4(color.rgb * color.a, color.a);
    borderColor = vec4(borderColor.rgb * borderColor.a, borderColor.a);

    if (vTexCoord.x <= vThickness.x || vTexCoord.x >= 1.0 - vThickness.x || vTexCoord.y <= vThickness.y || vTexCoord.y >= 1.0 - vThickness.y) {
        fragColor = borderColor;
    } else {
        fragColor = color;
    }
}
