#ifndef WORLD_H
#define WORLD_H

#include "object.h"
#include "camera.h"
#include "light.h"
#include "vertex.h"
#include "triangle.h"
#include "material.h"
#include "engineerror.h"

typedef struct {
  object objectList[1000];
  vertex* vlist;
  triangle* tlist;
  camera cameraList[10];
  light lightList[10];
  material materials[1000];
  unsigned int objectCount,vcount,tcount,cameraCount,lightCount,matCount,currentCamIndex;
} world;

#ifdef __cplusplus
extern "C" {
#endif

void FreeWorld(world* w);
/* loads fn (or sets up an empty world when fn is NULL). returns NULL on
   failure with the reason in GetLastEngineError() */
world* InitializeWorld(char *fn);
/* renders one frame through the pixel callback. returns 0 on failure */
int DrawScene(world* w);
void SetShadeState(int st);
int GetShadeState();
int GetScreenW();
int GetScreenH();
void SetScreenW(int w);
void SetScreenH(int h);
unsigned int GetVCount();
unsigned int GetTCount();
unsigned int GetCVCount();
unsigned int GetCTCount();
unsigned int GetCullTCount();
camera* GetCam();
void SetCam(int i);
float GetTNLen();
void SetTNLen(float l);
float GetVNLen();
void SetVNLen(float l);

#ifdef __cplusplus
}
#endif

#endif /* WORLD_H */
