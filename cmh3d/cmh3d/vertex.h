#ifndef VERTEX_H
#define VERTEX_H

#include "vector.h"

typedef struct {
  float x,y,z,h;
  int clipped;
  vector normal;
  float red,green,blue;
  unsigned int fncount;
  int ix,iy;
  float u,v;
} vertex;

#endif /* VERTEX_H */
