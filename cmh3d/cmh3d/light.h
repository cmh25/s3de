#ifndef LIGHT_H
#define LIGHT_H

#include "vertex.h"

typedef struct {
  vertex locationVertex,shineAtVertex;
  float red,green,blue;
} light;

#endif /* LIGHT_H */
