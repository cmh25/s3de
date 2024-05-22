#ifndef THREED_H
#define THREED_H

#include "vector.h"
#include "vertex.h"
#include "light.h"

typedef struct {
  vertex *v0, *v1, *v2;
  vector normal;
  float A, B, C, D;
  int type; /* l r t b n f - useful for viewport clipping */
} plane;

#ifdef __cplusplus
extern "C" {
#endif

void NormalizeVector(vector* v);
float MagnitudeOfVector(vector* v);
void VectorCrossProduct(vector* result, vector* v1, vector* v2);
float VectorDotProduct(vector* v1, vector* v2);
void VectorFromTo(vector* result, vertex* v1, vertex* v2);
void CalculatePlane(plane* p, vertex* vv0, vertex* vv1, vertex* vv2, int t);
float PointToPlaneZ(float x, float y, plane* p);
void PrintVertex(vertex* v);
void PrintVector(vector* v);
float Slope(float x0, float y0, float x1, float y1);
int PointInTriangle(float x, float y, float x0, float y0, float x1, float y1, float x2, float y2);

#ifdef __cplusplus
}
#endif

#endif /* THREED_H */