#ifndef THREEDS_H
#define THREEDS_H

#include "world.h"

#ifdef __cplusplus
extern "C" {
#endif

int Read3dsFile(char* fileName, world* w);

#ifdef __cplusplus
}
#endif

#endif /* THREEDS_H */
