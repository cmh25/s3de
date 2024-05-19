#ifndef CLIP_H
#define CLIP_H

#include "vertex.h"
#include "triangle.h"
#include "3d.h"
#include "camera.h"

#define LEFT   0
#define RIGHT  1
#define TOP    2
#define BOTTOM 3
#define NEAR   4
#define FAR    5

#define CLIPPED_NONE   0x0000
#define CLIPPED_LEFT   0x0001
#define CLIPPED_RIGHT  0x0002
#define CLIPPED_TOP    0x0004
#define CLIPPED_BOTTOM 0x0008
#define CLIPPED_NEAR   0x0010
#define CLIPPED_FAR    0x0020

#ifdef __cplusplus
extern "C" {
#endif

int ClipTriangleToView(triangle* tri, camera* pcam, vertex* clippedVList);
void ClipLineToView(vertex* v0, vertex* v1, camera *pcam);
int ClipPolyToPlane(plane* p, vertex* vList, int count);
void ClipLineToPlane(plane* p, vertex* v0, vertex* v1);
void ClipVertexToPlane(plane* p, vertex* v);

#ifdef __cplusplus
}
#endif

#endif /* CLIP_H */
