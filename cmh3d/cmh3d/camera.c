#include "camera.h"
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include "3d.h"
#include "matrix.h"
#include "vector.h"
#include "vertex.h"

void SetViewMatrix(camera* pcam) {
  matrix4x4 coordTranslate;
  matrix4x4 coordRotate;

  VectorFromTo(&pcam->Uvector, &pcam->locationVertex, &pcam->U);
  VectorFromTo(&pcam->Vvector, &pcam->locationVertex, &pcam->V);
  VectorFromTo(&pcam->Nvector, &pcam->locationVertex, &pcam->N);
  NormalizeVector(&pcam->Uvector);
  NormalizeVector(&pcam->Vvector);
  NormalizeVector(&pcam->Nvector);

  SetIdentity(coordTranslate);
  coordTranslate[0][3] = -pcam->locationVertex.x;
  coordTranslate[1][3] = -pcam->locationVertex.y;
  coordTranslate[2][3] = -pcam->locationVertex.z;

  SetIdentity(coordRotate);
  coordRotate[0][0] = pcam->Uvector.x;
  coordRotate[0][1] = pcam->Uvector.y;
  coordRotate[0][2] = pcam->Uvector.z;
  coordRotate[1][0] = pcam->Vvector.x;
  coordRotate[1][1] = pcam->Vvector.y;
  coordRotate[1][2] = pcam->Vvector.z;
  coordRotate[2][0] = pcam->Nvector.x;
  coordRotate[2][1] = pcam->Nvector.y;
  coordRotate[2][2] = pcam->Nvector.z;

  SetIdentity(pcam->viewMatrix);
  MatrixMultiply4x4_4x4(pcam->viewMatrix, coordTranslate);
  MatrixMultiply4x4_4x4(pcam->viewMatrix, coordRotate);
}

void SetViewLookAtMatrix(camera* pcam) {
  matrix4x4 coordTranslate;
  matrix4x4 coordRotate;

  VectorFromTo(&pcam->Uvector, &pcam->locationVertex, &pcam->U);
  VectorFromTo(&pcam->Vvector, &pcam->locationVertex, &pcam->V);
  VectorFromTo(&pcam->Nvector, &pcam->locationVertex, &pcam->N);
  NormalizeVector(&pcam->Uvector);
  NormalizeVector(&pcam->Vvector);
  NormalizeVector(&pcam->Nvector);

  SetIdentity(coordTranslate);
  coordTranslate[0][3] = pcam->locationVertex.x;
  coordTranslate[1][3] = pcam->locationVertex.y;
  coordTranslate[2][3] = pcam->locationVertex.z;

  SetIdentity(coordRotate);
  coordRotate[0][0] = pcam->Uvector.x;
  coordRotate[0][1] = pcam->Vvector.x;
  coordRotate[0][2] = pcam->Nvector.x;
  coordRotate[1][0] = pcam->Uvector.y;
  coordRotate[1][1] = pcam->Vvector.y;
  coordRotate[1][2] = pcam->Nvector.y;
  coordRotate[2][0] = pcam->Uvector.z;
  coordRotate[2][1] = pcam->Vvector.z;
  coordRotate[2][2] = pcam->Nvector.z;

  SetIdentity(pcam->viewLookAtMatrix);
  MatrixMultiply4x4_4x4(pcam->viewLookAtMatrix, coordRotate);
  MatrixMultiply4x4_4x4(pcam->viewLookAtMatrix, coordTranslate);
}

void Reset(camera* pcam) {
  vertex newLocationVertex = {0.0, 0.0, 0.0, 1.0};
  vertex newLookAtVertex = {0.0, 0.0, 0.0, 1.0};
  vector zVector;

  VectorFromTo(&zVector, &pcam->locationVertex, &pcam->lookAtVertex);

  newLookAtVertex.z = -MagnitudeOfVector(&zVector);

  memcpy(&pcam->lookAtVertex, &newLookAtVertex, sizeof(vertex));
  memcpy(&pcam->locationVertex, &newLocationVertex, sizeof(vertex));

  pcam->U.x = 1.0;
  pcam->U.y = 0.0;
  pcam->U.z = 0.0;
  pcam->U.h = 1.0;
  pcam->V.x = 0.0;
  pcam->V.y = 1.0;
  pcam->V.z = 0.0;
  pcam->V.y = 1.0;
  pcam->N.x = 0.0;
  pcam->N.y = 0.0;
  pcam->N.z = 1.0;
  pcam->N.h = 1.0;
}

void ApplyMatrix(camera* pcam, matrix4x4 m1) {
  MatrixMultiply1x4_4x4(&pcam->locationVertex, m1);
  MatrixMultiply1x4_4x4(&pcam->lookAtVertex, m1);
  MatrixMultiply1x4_4x4(&pcam->U, m1);
  MatrixMultiply1x4_4x4(&pcam->V, m1);
  MatrixMultiply1x4_4x4(&pcam->N, m1);
}

void SetViewClipPlanes(camera* pcam) {
  float xratio = (pcam->viewPlaneWidth / 2.0f / pcam->distanceToViewPlane) * pcam->clipUnit;
  float yratio = (pcam->viewPlaneHeight / 2.0f / pcam->distanceToViewPlane) * pcam->clipUnit;

  pcam->ltn.x = pcam->clipNear * xratio;
  pcam->ltn.y = -pcam->clipNear * yratio;
  pcam->ltn.z = pcam->clipNear;

  pcam->rtn.x = -pcam->clipNear * xratio;
  pcam->rtn.y = -pcam->clipNear * yratio;
  pcam->rtn.z = pcam->clipNear;

  pcam->rbn.x = -pcam->clipNear * xratio;
  pcam->rbn.y = pcam->clipNear * yratio;
  pcam->rbn.z = pcam->clipNear;

  pcam->lbn.x = pcam->clipNear * xratio;
  pcam->lbn.y = pcam->clipNear * yratio;
  pcam->lbn.z = pcam->clipNear;

  pcam->ltf.x = pcam->clipFar * xratio;
  pcam->ltf.y = -pcam->clipFar * yratio;
  pcam->ltf.z = pcam->clipFar;

  pcam->rtf.x = -pcam->clipFar * xratio;
  pcam->rtf.y = -pcam->clipFar * yratio;
  pcam->rtf.z = pcam->clipFar;

  pcam->rbf.x = -pcam->clipFar * xratio;
  pcam->rbf.y = pcam->clipFar * yratio;
  pcam->rbf.z = pcam->clipFar;

  pcam->lbf.x = pcam->clipFar * xratio;
  pcam->lbf.y = pcam->clipFar * yratio;
  pcam->lbf.z = pcam->clipFar;

  CalculatePlane(&pcam->clipPlanes[0], &pcam->ltf, &pcam->ltn, &pcam->lbn, 0);
  CalculatePlane(&pcam->clipPlanes[1], &pcam->rtn, &pcam->rtf, &pcam->rbf, 1);
  CalculatePlane(&pcam->clipPlanes[2], &pcam->rtn, &pcam->ltn, &pcam->ltf, 2);
  CalculatePlane(&pcam->clipPlanes[3], &pcam->rbf, &pcam->lbf, &pcam->lbn, 3);
  CalculatePlane(&pcam->clipPlanes[4], &pcam->ltn, &pcam->rtn, &pcam->rbn, 4);
  CalculatePlane(&pcam->clipPlanes[5], &pcam->rtf, &pcam->ltf, &pcam->lbf, 5);
}
