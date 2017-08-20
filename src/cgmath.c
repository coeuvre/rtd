#include "cgmath.h"

float LerpF(float f1, float t, float f2) {
    return f1 * (1 - t) + f2 * t;
}

V2 MakeV2(float x, float y) {
    V2 v = { x, y };
    return v;
}

V2 NegV2(V2 v) {
    V2 result = { -v.x, -v.y };
    return result;
}

V2 AddV2(V2 v1, V2 v2) {
    V2 v = { v1.x + v2.x, v1.y + v2.y };
    return v;
};

V2 SubV2(V2 v1, V2 v2) {
    V2 v = { v1.x - v2.x, v1.y - v2.y };
    return v;
}

float DotV2(V2 v1, V2 v2) {
    return v1.x * v2.x + v1.y + v2.y;
}

float CrossV2(V2 v1, V2 v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

V2 MulV2(V2 v, float f) {
    V2 result = { v.x * f, v.y * f };
    return result;
}

V2 DivV2(V2 v, float f) {
    V2 result = { v.x / f, v.y / f };
    return result;
}

V2 MulFV2(float f, V2 v) {
    V2 result = { f * v.x, f * v.y };
    return result;
}

V2 PerpV2(V2 v) {
    V2 result = { v.y, -v.x };
    return result;
}

V2 LerpV2(V2 v1, float t, V2 v2) {
    V2 result = { LerpF(v1.x, t, v2.x), LerpF(v1.y, t, v2.y) };
    return result;
}
