#include "3d.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include "vector.h"
#include "vertex.h"

void NormalizeVector(vector* v) {
  float hypotenuseInv;
  float mag;

  mag = MagnitudeOfVector(v);
  if(mag == 0.0)mag = 0.0000001f;
  hypotenuseInv = 1.0f / mag;
  v->x *= hypotenuseInv;
  v->y *= hypotenuseInv;
  v->z *= hypotenuseInv;
}

float MagnitudeOfVector(vector* v) {
  return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

void VectorCrossProduct(vector* result, vector* v0, vector* v1) {
  result->x = v0->y * v1->z - v0->z * v1->y;
  result->y = v0->z * v1->x - v0->x * v1->z;
  result->z = v0->x * v1->y - v0->y * v1->x;
  result->h = 1.0;
}

float VectorDotProduct(vector* v0, vector* v1) {
  return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

void VectorFromTo(vector* result, vertex* v0, vertex* v1) {
  result->x = v1->x - v0->x;
  result->y = v1->y - v0->y;
  result->z = v1->z - v0->z;
  result->h = 1.0;
}

void CalculatePlane(plane* p, vertex* vv0, vertex* vv1, vertex* vv2, int t) {
  vector vec0,vec1;

  p->v0 = vv0;
  p->v1 = vv1;
  p->v2 = vv2;

  VectorFromTo(&vec0, p->v1, p->v2);
  VectorFromTo(&vec1, p->v1, p->v0);

  VectorCrossProduct(&p->normal, &vec0, &vec1);
  NormalizeVector(&p->normal);

  p->A = p->normal.x;
  p->B = p->normal.y;
  p->C = p->normal.z;
  p->D = -p->A * p->v0->x - p->B * p->v0->y - p->C * p->v0->z;

  p->type = t;
}

float PointToPlaneZ(float x, float y, plane* p) {
  return (-p->D - p->B * y - p->A * x) / p->C;
}

void PrintVertex(vertex* v) {
  printf("x: %f\n", v->x);
  printf("y: %f\n", v->y);
  printf("z: %f\n", v->z);
}

void PrintVector(vector* v) {
  printf("x: %f\n", v->x);
  printf("y: %f\n", v->y);
  printf("z: %f\n", v->z);
}

float Slope(float x0, float y0, float x1, float y1) {
  return (y1 - y0) / (x1 - x0);
}

int PointInTriangle(float x, float y, float x0, float y0,
            float x1, float y1, float x2, float y2) {
  int pointIn = 0;
  float minX,maxX,minY,maxY,slopePoint,slope1,slope2;

  //find bounding rectangle
  minX = x0 < x1 ? x0 : x1;
  if(minX > x2) minX = x2;

  maxX = x0 > x1 ? x0 : x1;
  if(maxX < x2) maxX = x2;

  minY = y0 < y1 ? y0 : y1;
  if(minY > y2) minY = y2;

  maxY = y0 > y1 ? y0 : y1;
  if(maxY < y2) maxY = y2;

  //see if the point is outside the bounding rectangle
  if(x<minX||x>maxX||y<minY||y>maxY) pointIn = 0;
  else {
    //we can't be sure the point is completely out yet,
    //but if it lies between the slopes of two different vertices
    //then we know it is in
    slopePoint = Slope(x0, y0, x, y);
    slope1 = Slope(x0, y0, x1, y1);
    slope2 = Slope(x0, y0, x2, y2);
    if( ( (slopePoint < slope1) && (slopePoint > slope2) ) ||
      ( (slopePoint > slope1) && (slopePoint < slope2) ) ) {
      //test one more and we are done
      slopePoint = Slope(x1, y1, x, y);
      slope1 = Slope(x1, y1, x0, y0);
      slope2 = Slope(x1, y1, x2, y2);
      if( ( (slopePoint < slope1) && (slopePoint > slope2) ) ||
          ( (slopePoint > slope1) && (slopePoint < slope2) ) ) {
        pointIn = 1;
      }
    }
  }

  return pointIn;
}
