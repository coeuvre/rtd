#ifndef RTD_CGMATH_H
#define RTD_CGMATH_H

//
// Scalar
//
float LerpF(float f1, float t, float f2);

//
// 2D Vector
//
typedef struct V2 {
    union {
        struct {
            float x;
            float y;
        };

        struct {
            float dx;
            float dy;
        };

        struct {
            float u;
            float v;
        };
    };
} V2;

V2 MakeV2(float x, float y);
V2 NegV2(V2 v);
V2 AddV2(V2 v1, V2 v2);
V2 SubV2(V2 v1, V2 v2);
float DotV2(V2 v1, V2 v2);
float CrossV2(V2 v1, V2 v2);
V2 MulV2(V2 v, float f);
V2 DivV2(V2 v, float f);
V2 MulFV2(float f, V2 v);
V2 PerpV2(V2 v);
V2 LerpV2(V2 v1, float t, V2 v2);

#endif // RTD_CGMATH_H
