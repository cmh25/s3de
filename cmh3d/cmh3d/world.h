#ifndef WORLD_H
#define WORLD_H

#include "object.h"
#include "camera.h"
#include "light.h"
#include "vertex.h"
#include "triangle.h"
#include "material.h"

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
world* InitializeWorld(char *fn);
void DrawScene(world* w);
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
