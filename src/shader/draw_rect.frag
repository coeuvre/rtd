#version 330 core

in vec2 vTexCoord;
in vec4 vColor;
in vec2 vRoundRadius;
in vec2 vThickness;
in vec4 vBorderColor;

out vec4 fragColor;

float CalcEllipseDelta(vec2 p, vec2 r) {
    if (r == 0) {
        return 0;
    }

    float d = (p.x / r.x) * (p.x / r.x) + (p.y / r.y) * (p.y / r.y);
    float e = fwidth(d);
    return 1 - smoothstep(1 - e, 1, d);
}

vec4 CalcBorderColor(vec2 p, vec2 roundRadius, vec2 thickness, vec4 borderColor, vec4 color) {
     // Outer ellipse
    float t = CalcEllipseDelta(p, roundRadius);
    // Inner ellipse
    float t2 = CalcEllipseDelta(p, roundRadius - thickness);
    vec4 c = borderColor * (1 - t2);
    // Blend between borderColor and color
    c = c + color * (1 - c.a);
    c = c * t;
    return c;
}

void main() {
    // Pre-multiply alpha
    vec4 borderColor = vec4(vBorderColor.rgb * vBorderColor.a, vBorderColor.a);
    vec4 color = vec4(vColor.rgb * vColor.a, vColor.a);

    if (vTexCoord.x <= vRoundRadius.x && vTexCoord.y <= vRoundRadius.y) {
        fragColor = CalcBorderColor(vTexCoord - vRoundRadius, vRoundRadius, vThickness, borderColor, color);
    } else if (vTexCoord.x >= 1 - vRoundRadius.x && vTexCoord.y <= vRoundRadius.y) {
        fragColor = CalcBorderColor(vec2(1 - vTexCoord.x, vTexCoord.y) - vRoundRadius, vRoundRadius, vThickness, borderColor, color);
    } else if (vTexCoord.x <= vRoundRadius.x && vTexCoord.y >= 1 - vRoundRadius.y) {
        fragColor = CalcBorderColor(vec2(vTexCoord.x, 1 - vTexCoord.y) - vRoundRadius, vRoundRadius, vThickness, borderColor, color);
    } else if (vTexCoord.x >= 1 - vRoundRadius.x && vTexCoord.y >= 1 - vRoundRadius.y) {
        fragColor = CalcBorderColor(vec2(1) - vTexCoord - vRoundRadius, vRoundRadius, vThickness, borderColor, color);
    } else if (vTexCoord.x <= vThickness.x || vTexCoord.x >= 1.0 - vThickness.x || vTexCoord.y <= vThickness.y || vTexCoord.y >= 1.0 - vThickness.y) {
        fragColor = borderColor;
    } else {
        fragColor = color;
    }
}