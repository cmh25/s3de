#ifndef CAMERA_H
#define CAMERA_H

#include "vector.h"
#include "vertex.h"
#include "matrix.h"
#include "3d.h"

typedef struct {
  vertex locationVertex;
  vertex lookAtVertex;
  vertex U;
  vertex V;
  vertex N;
  vector Uvector;
  vector Vvector;
  vector Nvector;
  float distanceToViewPlane;
  float viewPlaneWidth;
  float viewPlaneHeight;
  float clipNear;
  float clipFar;
  float clipUnit;
  matrix4x4 viewMatrix;
  matrix4x4 viewLookAtMatrix;
  vertex ltn, rtn, lbn, rbn, ltf, rtf, lbf, rbf;
  plane clipPlanes[6];
} camera;

#ifdef __cplusplus
extern "C" {
#endif

void SetViewMatrix(camera* pcam);
void SetViewLookAtMatrix(camera* pcam);
void Reset(camera* pcam);
void ApplyMatrix(camera* pcam, matrix4x4 m1);
void SetViewClipPlanes(camera* pcam);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_H */
