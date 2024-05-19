#ifndef VERTEX_H
#define VERTEX_H

#include "vector.h"

typedef struct {
  float x,y,z,h;
  int clipped;
  vector normal;
  float red,green,blue;
  int color;
  unsigned int fncount;
  int ix,iy,iz;
  float u,v;
} vertex;

#endif /* VERTEX_H */
