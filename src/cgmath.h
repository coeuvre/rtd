#ifndef RTD_CGMATH_H
#define RTD_CGMATH_H

#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

typedef float F;

//
// Scalar
//
static inline F AbsF(F x) {
    F result = fabsf(x);
    return result;
}

static inline F FloorF(F x) {
    F result = floorf(x);
    return result;
}

static inline F CeilF(F x) {
    F result = ceilf(x);
    return result;
}

static inline F ClampF(F x, F min, F max) {
    F result = x;

    if (x < min) {
        result = min;
    } else if (x > max) {
        result = max;
    }

    return result;
}

static inline F Clamp01F(F x) {
    F result = ClampF(x, 0.0f, 1.0f);
    return result;
}

static inline F LerpF(F a, F t, F b) {
    F result = (1.0f - t) * a + t * b;
    return result;
}

//
// 2D Vector
//
typedef struct V2 {
    union {
        struct {
            F x;
            F y;
        };

        struct {
            F w;
            F h;
        };

        struct {
            F dx;
            F dy;
        };

        struct {
            F u;
            F v;
        };
    };
} V2;

static inline V2 MakeV2(F x, F y) {
    V2 v = { x, y };
    return v;
}

static inline V2 ZeroV2(void) {
    return MakeV2(0.0f, 0.0f);
}

static inline V2 NegV2(V2 v) {
    V2 result = { -v.x, -v.y };
    return result;
}

static inline V2 AddV2(V2 a, V2 b) {
    V2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

static inline V2 SubV2(V2 a, V2 b) {
    V2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

static inline V2 MulV2(F a, V2 b) {
    V2 result;
    result.x = a * b.x;
    result.y = a * b.y;
    return result;
}

static inline F DotV2(V2 a, V2 b) {
    F result = a.x * b.x + a.y * b.y;
    return result;
}

static inline F CrossV2(V2 a, V2 b) {
    F result = a.x * b.y - a.y * b.x;
    return result;
}

static inline V2 PerpV2(V2 v) {
    V2 result;
    result.x = -v.y;
    result.y = v.x;
    return result;
}

static inline V2 PerpRV2(V2 v) {
    V2 result;
    result.x = v.y;
    result.y = -v.x;
    return result;
}

static inline V2 HadamardV2(V2 a, V2 b) {
    V2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

static inline V2 Clamp01V2(V2 v) {
    V2 result;
    result.x = Clamp01F(v.x);
    result.y = Clamp01F(v.y);

    return result;
}

static inline F GetV2LenSq(V2 v) {
    F result = DotV2(v, v);
    return result;
}

static inline F GetV2Len(V2 v) {
    F result = sqrtf(GetV2LenSq(v));
    return result;
}

static inline F GetV2Rad(V2 v) {
    F result = atan2f(v.y, v.x);
    return result;
}

static inline F GetV2RadBetween(V2 a, V2 b) {
    F result = GetV2Rad(a) - GetV2Rad(b);
    result = AbsF(result);
    return result;
}

static inline V2 NormalizeV2(V2 v) {
    F len = GetV2Len(v);
    F invLen = 0.0f;
    if (len != 0.0f) {
        invLen = 1.0f / len;
    }
    V2 result = MulV2(invLen, v);
    return result;
}

// Is vector b at the left side of vector a. (Right Hand Coordinate System)
static inline int IsV2Left(V2 a, V2 b) {
    F cross = CrossV2(a, b);
    int result = cross > 0;
    return result;
}

// Is vector b collinear with vector a. (Right Hand Coordinate System)
static inline int IsV2Collinear(V2 a, V2 b) {
    F cross = CrossV2(a, b);
    int result = cross == 0;
    return result;
}

// Is vector b at the left side of or collinear with vector a. (Right Hand Coordinate System)
static inline int IsV2LeftOrCollinear(V2 a, V2 b) {
    F cross = CrossV2(a, b);
    int result = cross >= 0;
    return result;
}

static inline int IsV2Equal(V2 a, V2 b) {
    int result = (a.x == b.x && a.y == b.y);
    return result;
}

// 2D Bounding Box
typedef struct BBox2 {
    V2 min;
    V2 max;
} BBox2;


static inline BBox2 MakeBBox(V2 min, V2 max) {
    BBox2 result;
    result.min = min;
    result.max = max;
    return result;
}


//
// 2D Linear System
//

// Solve 2D linear system.
//
// Return 0 means there is NOT a solution, otherwise, there is a solution.
static inline int SolveLinearSystem2(V2 a, V2 b, V2 c, V2 *solution) {
    F d = a.x * b.y - a.y * b.x;
    if (d != 0.0f) {
        *solution = MakeV2((c.x * b.y - c.y * b.x) / d,
                           (a.x * c.y - a.y * c.x) / d);
        return 1;
    } else {
        return 0;
    }
}

//
// 2D Affine Transform Matrix
//
typedef union {
    // The affine transform matrix:
    //
    //     | a c x |    | x y o |
    //     | b d y | or | x y o |
    //     | 0 0 1 |    | x y o |
    //
    // This matrix is used to multiply by column vector:
    //
    //     | a c x |   | x |
    //     | b d y | * | y |
    //     | 0 0 1 |   | 1 |
    //
    // This matrix use column-major order to store elements

    struct {
        F a; F b;
        F c; F d;
        F x; F y;
    };

    struct {
        F m00; F m10;
        F m01; F m11;
        F m02; F m12;
    };

    struct {
        V2 xAxis;
        V2 yAxis;
        V2 origin;
    };
} T2;

static inline T2 IdentityT2(void) {
    T2 result = {0};
    result.a = 1;
    result.d = 1;
    return result;
}

// If the given transform cannot be inverted, return the unchanged t.
static inline T2 InvertT2(T2 t) {
    F det = t.m00 * t.m11 - t.m01 * t.m10;

    if (det == 0.0f) {
        return t;
    }

    F invDet = 1.0f / det;
    T2 result;

    result.m00 = invDet * (t.m11);
    result.m01 = invDet * (-t.m01);
    result.m02 = invDet * (t.m01 * t.m12 - t.m02 * t.m11);
    result.m10 = invDet * (-t.m10);
    result.m11 = invDet * (t.m00);
    result.m12 = invDet * (t.m02 * t.m10 - t.m00 * t.m12);

    return result;
}

static inline T2 DotT2(T2 t1, T2 t2) {
    T2 result;

    result.m00 = t1.m00 * t2.m00 + t1.m01 * t2.m10 + t1.m02 * 0;
    result.m01 = t1.m00 * t2.m01 + t1.m01 * t2.m11 + t1.m02 * 0;
    result.m02 = t1.m00 * t2.m02 + t1.m01 * t2.m12 + t1.m02 * 1;

    result.m10 = t1.m10 * t2.m00 + t1.m11 * t2.m10 + t1.m12 * 0;
    result.m11 = t1.m10 * t2.m01 + t1.m11 * t2.m11 + t1.m12 * 0;
    result.m12 = t1.m10 * t2.m02 + t1.m11 * t2.m12 + t1.m12 * 1;

    return result;
}

static inline T2 MakeT2FromRotation(F rad) {
    T2 result = {0};
    result.a = cosf(rad);
    result.b = sinf(rad);
    result.c = -result.b;
    result.d = result.a;
    return result;
}

static inline T2 MakeT2FromScale(V2 s) {
    T2 result = IdentityT2();
    result.a = s.x;
    result.d = s.y;
    return result;
}

static inline T2 MakeT2FromTranslation(V2 t) {
    T2 result = IdentityT2();
    result.x = t.x;
    result.y = t.y;
    return result;
}

static inline V2 ApplyT2(T2 t, V2 v) {
    V2 result;
    result.x = v.x * t.a + v.y * t.c + t.x;
    result.y = v.x * t.b + v.y * t.d + t.y;
    return result;
}

static inline T2 RotateT2(F rad, T2 t) {
    T2 result = DotT2(MakeT2FromRotation(rad), t);
    return result;
}

static inline T2 ScaleT2(V2 s, T2 t) {
    T2 result = DotT2(MakeT2FromScale(s), t);
    return result;
}

static inline T2 TranslateT2(V2 offset, T2 t) {
    T2 result = DotT2(MakeT2FromTranslation(offset), t);
    return result;
}

static inline V2 GetT2Scale(T2 t) {
    V2 result = MakeV2(GetV2Len(t.xAxis), GetV2Len(t.yAxis));
    return result;
}


typedef union GLM4 {
    struct {
        F m00; F m10; F m20; F m30;
        F m01; F m11; F m21; F m31;
        F m02; F m12; F m22; F m32;
        F m03; F m13; F m23; F m33;
    };

    F m[9];
} GLM4;

static inline GLM4 MakeGLM3FromT2(T2 t) {
    GLM4 result;
    result.m00 = t.m00;
    result.m10 = t.m10;
    result.m20 = 0.0f;
    result.m30 = 0.0f;
    result.m01 = t.m01;
    result.m11 = t.m11;
    result.m21 = 0.0f;
    result.m31 = 0.0f;
    result.m02 = 0.0f;
    result.m12 = 0.0f;
    result.m22 = 1.0f;
    result.m32 = 0.0f;
    result.m03 = t.m02;
    result.m13 = t.m12;
    result.m23 = 0.0f;
    result.m33 = 1.0f;
    return result;
}

#endif // RTD_CGMATH_H
