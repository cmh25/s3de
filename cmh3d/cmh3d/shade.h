#ifndef SHADE_H
#define SHADE_H

#include "triangle.h"
#include "light.h"
#include "vertex.h"

#define SHADE_VERTEX            0x0001
#define SHADE_WIREFRAME         0x0002
#define SHADE_FLAT              0x0004
#define SHADE_GOURAUD           0x0008
#define SHADE_PHONG             0x0010
#define SHADE_ZBUFFER           0x0020
#define SHADE_AMBIENT           0x0040
#define SHADE_DIFFUSE           0x0080
#define SHADE_SPECULAR          0x0100
#define SHADE_BILINEAR          0x0200
#define SHADE_TEXTURE           0x0400
#define SHADE_TNORMAL           0x0800
#define SHADE_VNORMAL           0x1000

#ifdef __cplusplus
extern "C" {
#endif

void ShadeTriangle(triangle* ptri, light* plight, float* pZBuffer, int shadeState);
void SetAmbient(float red, float green, float blue);
void Setppixel(void (*f)(int x, int y, int r, int g, int b));
void line(int x0, int y0, int x1, int y1, int r, int g, int b);
void line3dz(vertex* v0, vertex* v1, float *zb);

#ifdef __cplusplus
}
#endif

#endif /* SHADE_H */
