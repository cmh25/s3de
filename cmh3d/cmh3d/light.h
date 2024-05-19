#ifndef LIGHT_H
#define LIGHT_H

#include "vertex.h"

typedef struct {
  vertex locationVertex;
  vertex shineAtVertex;
  float red;
  float green;
  float blue;
} light;

#endif /* LIGHT_H */
