#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "vertex.h"
#include "vector.h"
#include "material.h"

#define EMPTY   0
#define FILLED  1

typedef struct {
  vertex* pvlist;
  unsigned int v0,v1,v2,cv,nv;
  vector normal;
  int clipped,visible;
  float farz;
  material* pmat;
  float U0,V0,U1,V1,U2,V2;
} triangle;

#define LEFT               0
#define RIGHT              1
#define TT_FLAT_BOTTOM     0
#define TT_FLAT_TOP        1
#define TT_TWO_EDGE_LEFT   2
#define TT_TWO_EDGE_RIGHT  3
#define TT_HORIZONTAL_LINE 4
#define TT_PIXEL           5

#ifdef __cplusplus
extern "C" {
#endif

int ScanTriangleEdges(triangle* ptri, int* pleftedge, int* prightedge);

#ifdef __cplusplus
}
#endif

#endif /* TRIANGLE_H */
