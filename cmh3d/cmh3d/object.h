#ifndef OBJECT_H
#define OBJECT_H

#include "vertex.h"
#include "triangle.h"

typedef struct {
  vertex* vlist;
  triangle* tlist;
  unsigned int vcount,tcount;
} object;

#endif /* OBJECT_H */
