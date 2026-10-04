/* headless.c - the smallest host for the cmh3d renderer.

   Renders a .3ds file to a binary PPM image using nothing but the C runtime,
   which also demonstrates that the engine does not depend on Windows: it draws
   through the pixel callback given to Setppixel() and knows nothing about
   where the pixels end up.

   usage: headless model.3ds out.ppm [shade] [rotx] [roty] [zoom] [px py pz]

     shade      shade state flags as a hex number, see cmh3d/shade.h
                (default 4e4: flat, ambient, diffuse, z-buffer, textured)
     rotx roty  rotation about the x and y axes in degrees (default 25, -30)
     zoom       1.0 fits the model's bounding sphere to the view (default 1.2)
     px py pz   if given, the camera is placed at this world position looking
                along its rotated -z axis (first person) instead of orbiting

   The engine's y axis points up, so rows are flipped when the image is written. */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmh3d/world.h"
#include "cmh3d/camera.h"
#include "cmh3d/matrix.h"
#include "cmh3d/shade.h"

#define WIDTH  640
#define HEIGHT 480

static unsigned char framebuffer[WIDTH * HEIGHT * 3];

/* the one function the engine needs from its host */
static void putPixel(int x, int y, int r, int g, int b) {
  unsigned char* p;
  if(x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) return;
  p = &framebuffer[((HEIGHT - 1 - y) * WIDTH + x) * 3];
  p[0] = (unsigned char)r;
  p[1] = (unsigned char)g;
  p[2] = (unsigned char)b;
}

static int writePpm(const char* fileName) {
  FILE* fp = fopen(fileName, "wb");
  if(!fp) { fprintf(stderr, "cannot write %s\n", fileName); return 0; }
  fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
  fwrite(framebuffer, 1, sizeof(framebuffer), fp);
  fclose(fp);
  return 1;
}

int main(int argc, char** argv) {
  const char* model = argc > 1 ? argv[1] : NULL;
  const char* out   = argc > 2 ? argv[2] : NULL;
  int shade   = argc > 3 ? (int)strtol(argv[3], NULL, 16) : (SHADE_FLAT | SHADE_AMBIENT | SHADE_DIFFUSE | SHADE_ZBUFFER | SHADE_TEXTURE);
  float rotx  = argc > 4 ? (float)atof(argv[4]) : 25.0f;
  float roty  = argc > 5 ? (float)atof(argv[5]) : -30.0f;
  float zoom  = argc > 6 ? (float)atof(argv[6]) : 1.2f;
  int firstPerson = argc > 9;
  world* w;
  camera* cam;
  matrix4x4 m;
  unsigned int i, j;
  float minx = 1e30f, miny = 1e30f, minz = 1e30f, maxx = -1e30f, maxy = -1e30f, maxz = -1e30f;
  float cx = 0.0f, cy = 0.0f, cz = 0.0f, radius = 1.0f, distance;

  if(!model || !out) {
    fprintf(stderr, "usage: headless model.3ds out.ppm [shade] [rotx] [roty] [zoom] [px py pz]\n");
    return 2;
  }

  memset(framebuffer, 0x66, sizeof(framebuffer));

  /* 1. tell the engine where to draw, 2. load the world, 3. tell it the output size */
  Setppixel(putPixel);
  w = InitializeWorld((char*)model);
  if(!w) { fprintf(stderr, "out of memory\n"); return 1; }
  SetScreenW(WIDTH);
  SetScreenH(HEIGHT);

  /* bounding box of everything, to frame the model */
  for(i = 0; i < w->objectCount; i++) {
    for(j = 0; j < w->objectList[i].vcount; j++) {
      vertex* v = &w->objectList[i].vlist[j];
      if(v->x < minx) minx = v->x;
      if(v->x > maxx) maxx = v->x;
      if(v->y < miny) miny = v->y;
      if(v->y > maxy) maxy = v->y;
      if(v->z < minz) minz = v->z;
      if(v->z > maxz) maxz = v->z;
    }
  }
  if(w->objectCount) {
    cx = (minx + maxx) / 2.0f;
    cy = (miny + maxy) / 2.0f;
    cz = (minz + maxz) / 2.0f;
    radius = 0.5f * sqrtf((maxx - minx) * (maxx - minx) + (maxy - miny) * (maxy - miny) + (maxz - minz) * (maxz - minz));
  }

  /* place the camera with the same ApplyMatrix() sequence the Win32 host uses in ProcessInput() */
  cam = &w->cameraList[w->currentCamIndex];
  distance = radius * zoom * cam->distanceToViewPlane / (cam->viewPlaneHeight / 2.0f);
  Reset(cam);
  if(firstPerson) {
    GetRotateMatrix(m, rotx, roty, 0.0f);
    ApplyMatrix(cam, m);
    GetTranslateMatrix(m, (float)atof(argv[7]), (float)atof(argv[8]), (float)atof(argv[9]));
    ApplyMatrix(cam, m);
  }
  else {
    GetTranslateMatrix(m, 0.0f, 0.0f, distance);
    ApplyMatrix(cam, m);
    GetRotateMatrix(m, rotx, roty, 0.0f);
    ApplyMatrix(cam, m);
    GetTranslateMatrix(m, cx, cy, cz);
    ApplyMatrix(cam, m);
  }

  SetShadeState(shade);
  DrawScene(w);

  if(!writePpm(out)) { FreeWorld(w); return 1; }
  printf("%s: %u objects, %u vertices, %u triangles (%u clipped, %u culled) -> %s\n",
    model, w->objectCount, GetVCount(), GetTCount(), GetCTCount(), GetCullTCount(), out);
  FreeWorld(w);
  return 0;
}
