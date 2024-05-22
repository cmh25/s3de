#ifndef CAMERA_H
#define CAMERA_H

#include "vector.h"
#include "vertex.h"
#include "matrix.h"
#include "3d.h"

typedef struct {
  vertex locationVertex,lookAtVertex;
  vertex U,V,N;
  vector Uvector,Vvector,Nvector;
  float distanceToViewPlane;
  float viewPlaneWidth,viewPlaneHeight;
  float clipNear,clipFar,clipUnit;
  matrix4x4 viewMatrix,viewLookAtMatrix;
  vertex ltn,rtn,lbn,rbn,ltf,rtf,lbf,rbf;
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
