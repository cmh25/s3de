#include "world.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include "world.h"
#include "3d.h"
#include "vertex.h"
#include "vector.h"
#include "triangle.h"
#include "object.h"
#include "matrix.h"
#include "shade.h"
#include "clip.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "3ds.h"

static vertex *vlist0,*vlist1,*vlist2,*tnvlist,*vnvlist;
static triangle *tlist0,*tlist1,*tlist2,*tlist3,**tpa;
static unsigned int vcount0,vcount1,vcount2,tcount0,tcount1,tcount2,clippedvcount,clippedtcount,culltcount,tnvcount,vnvcount;
static world* pw;
static camera* pcam;
static float* zbuffer,m_vnlen=5.0f,m_tnlen=5.0f;
static int shadeState,screenwidth=640,screenheight=480;

int GetShadeState() { return shadeState; }
void SetShadeState(int st) { shadeState = st; }
int GetScreenW() { return screenwidth; }
void SetScreenW(int w) {
  unsigned int i;
  screenwidth = w;
  if(pw) {
    for(i=0;i<pw->cameraCount;i++) {
      pw->cameraList[i].viewPlaneHeight = pw->cameraList[i].viewPlaneWidth * screenheight / screenwidth;
      SetViewClipPlanes(&pw->cameraList[i]);
    }
  }
}
int GetScreenH() { return screenheight; }
void SetScreenH(int h) {
  unsigned int i;
  screenheight = h;
  if(pw) {
    for(i=0;i<pw->cameraCount;i++) {
      pw->cameraList[i].viewPlaneHeight = pw->cameraList[i].viewPlaneWidth * screenheight / screenwidth;
      SetViewClipPlanes(&pw->cameraList[i]);
    }
  }
}

unsigned int GetVCount() { return vcount0; }
unsigned int GetTCount() { return tcount0; }
unsigned int GetCVCount() { return clippedvcount; }
unsigned int GetCTCount() { return clippedtcount; }
unsigned int GetCullTCount() { return culltcount;  }
camera* GetCam() { return pcam;  }
void SetCam(int i) { pw->currentCamIndex = i; }
float GetTNLen() { return m_tnlen; }
void SetTNLen(float l) { m_tnlen = l; }
float GetVNLen() { return m_vnlen; }
void SetVNLen(float l) { m_vnlen = l; }

void FreeWorld(world* w) {
  unsigned int i;
  if(vlist0) { free(vlist0); vlist0 = 0; }
  if(vlist1) { free(vlist1); vlist1 = 0; }
  if(vlist2) { free(vlist2); vlist2 = 0; }
  if(tlist0) { free(tlist0); tlist0 = 0; }
  if(tlist1) { free(tlist1); tlist1 = 0; }
  if(tlist2) { free(tlist2); tlist2 = 0; }
  vcount0 = vcount1 = vcount2 = tcount0 = tcount1 = tcount2 = clippedvcount = 0;
  for(i=0;i<w->matCount;i++) {
    if(w->materials[i].ptexture) {
      free(w->materials[i].ptexture);
    }
  }
  for(i=0;i<w->objectCount;i++) {
    if(w->objectList[i].vlist)
      free(w->objectList[i].vlist);
    if(w->objectList[i].tlist)
      free(w->objectList[i].tlist);
  }
  free(w);
  pw = 0;
}

world* InitializeWorld(char *fn) {
  world* w = (world*)calloc(1,sizeof(world));
  if(!w) return 0;
  w->cameraCount = 3;
  w->currentCamIndex = 0;
  w->cameraList[0].locationVertex.x = 0.0;
  w->cameraList[0].locationVertex.y = 0.0;
  w->cameraList[0].locationVertex.z = 0.0;
  w->cameraList[0].locationVertex.h = 1.0;
  w->cameraList[0].lookAtVertex.x = 0.0;
  w->cameraList[0].lookAtVertex.y = 0.0;
  w->cameraList[0].lookAtVertex.z = -50.0;
  w->cameraList[0].lookAtVertex.h = 1.0;
  w->cameraList[0].distanceToViewPlane = 30.0;
  w->cameraList[0].viewPlaneWidth = 40.0;
  w->cameraList[0].viewPlaneHeight = 0.0; /* derived from viewPlaneWidth */
  w->cameraList[0].clipNear = -1.0;
  w->cameraList[0].clipFar = -20000.0;
  w->cameraList[0].clipUnit = 1.0f;
  w->cameraList[0].U.x = 1.0;
  w->cameraList[0].U.y = 0.0;
  w->cameraList[0].U.z = 0.0;
  w->cameraList[0].U.h = 1.0;
  w->cameraList[0].V.x = 0.0;
  w->cameraList[0].V.y = 1.0;
  w->cameraList[0].V.z = 0.0;
  w->cameraList[0].V.h = 1.0;
  w->cameraList[0].N.x = 0.0;
  w->cameraList[0].N.y = 0.0;
  w->cameraList[0].N.z = 1.0;
  w->cameraList[0].N.h = 1.0;
  memcpy(&w->cameraList[1], &w->cameraList[0], sizeof(camera));
  memcpy(&w->cameraList[2], &w->cameraList[0], sizeof(camera));

  w->lightCount = 2;
  w->lightList[0].locationVertex.x = 100.0;
  w->lightList[0].locationVertex.y = 100.0;
  w->lightList[0].locationVertex.z = 100.0;
  w->lightList[0].locationVertex.h = 1.0;
  w->lightList[0].shineAtVertex.x = 0.0;
  w->lightList[0].shineAtVertex.y = 0.0;
  w->lightList[0].shineAtVertex.z = 0.0;
  w->lightList[0].shineAtVertex.h = 1.0;
  w->lightList[0].red = 1.0;
  w->lightList[0].green = 1.0;
  w->lightList[0].blue = 1.0;

  w->lightList[1].locationVertex.x = 0.0;
  w->lightList[1].locationVertex.y = 0.0;
  w->lightList[1].locationVertex.z = 0.0;
  w->lightList[1].locationVertex.h = 1.0;
  w->lightList[1].shineAtVertex.x = 0.0;
  w->lightList[1].shineAtVertex.y = 0.0;
  w->lightList[1].shineAtVertex.z = -20.0;
  w->lightList[1].shineAtVertex.h = 1.0;

  SetAmbient(0.3f, 0.3f, 0.3f);
  SetShadeState(SHADE_FLAT | SHADE_AMBIENT | SHADE_DIFFUSE | SHADE_ZBUFFER | SHADE_TEXTURE);

  if(fn && !Read3dsFile(fn,w)) {
    fprintf(stderr, "Failed to read: %s\n", fn);
    exit(1);
  }
  pw = w;
  return w;
}

static void Concatenate() {
  unsigned int i, j, vi=0, ti=0;

  /* concatenate all the objects' vlists and tlists into the world vlist and tlist
  the world vlist and tlist get culled and clipped so this resets everything to
  starting state on each render */
  pw->vcount = 0;
  pw->tcount = 0;
  for(i=0;i<pw->objectCount;i++) {
    pw->vcount += pw->objectList[i].vcount;
    pw->tcount += pw->objectList[i].tcount;
  }

  if(!vlist0 || vcount0 < pw->vcount) vlist0 = realloc(vlist0, pw->vcount * sizeof(vertex));
  if(!vlist0) { printf("malloc failed in Concatenate()\n"); exit(1); }
  if(!tlist0 || tcount0 < pw->tcount) tlist0 = realloc(tlist0, pw->tcount * sizeof(triangle));
  if(!tlist0) { printf("malloc failed in Concatenate()\n"); exit(1); }

  pw->vlist = vlist0;
  pw->tlist = tlist0;
  vcount0 = pw->vcount;
  tcount0 = pw->tcount;
  if(vcount1 < vcount0) vcount1 = vcount0;

  for(i=0;i<pw->objectCount;i++) {
    memcpy(&pw->vlist[vi], pw->objectList[i].vlist, sizeof(vertex)* pw->objectList[i].vcount);
    memcpy(&pw->tlist[ti], pw->objectList[i].tlist, sizeof(triangle)* pw->objectList[i].tcount);
    for(j=0;j<pw->objectList[i].tcount;j++) {
      pw->tlist[ti+j].v0 += vi;
      pw->tlist[ti+j].v1 += vi;
      pw->tlist[ti+j].v2 += vi;
    }
    vi += pw->objectList[i].vcount;
    ti += pw->objectList[i].tcount;
  }
}

static void WorldToView() {
  unsigned int i;
  SetViewMatrix(pcam);
  for(i=0;i<pw->vcount;i++)
    MatrixMultiply1x4_4x4(&pw->vlist[i], pcam->viewMatrix);
}

static void PreClip() {
  unsigned int i;
  vertex* vlist = pw->vlist;
  triangle* tlist = pw->tlist;
  unsigned int cvc = 0, ctc = 0;
  static unsigned int mt = 32;

  /* clip vertex list */
  for(i=0;i<pw->vcount;i++) {
    vlist[i].clipped = 0;
    ClipVertexToPlane(&pcam->clipPlanes[0], &vlist[i]);
    ClipVertexToPlane(&pcam->clipPlanes[1], &vlist[i]);
    ClipVertexToPlane(&pcam->clipPlanes[2], &vlist[i]);
    ClipVertexToPlane(&pcam->clipPlanes[3], &vlist[i]);
    ClipVertexToPlane(&pcam->clipPlanes[4], &vlist[i]);
    ClipVertexToPlane(&pcam->clipPlanes[5], &vlist[i]);
    if(vlist[i].clipped) cvc++;
  }
  clippedvcount = cvc;

  tcount1 = 0;
  /* pass 1 - clip triangle list */
  for(i=0;i<pw->tcount;i++) {
    tlist[i].clipped = vlist[tlist[i].v0].clipped
             | vlist[tlist[i].v1].clipped
             | vlist[tlist[i].v2].clipped;

    if(vlist[tlist[i].v0].clipped & vlist[tlist[i].v1].clipped & vlist[tlist[i].v2].clipped) {
      /* all vertices share at least one common clip plane */
      tlist[i].visible = 0;
    }
    else {
      /* this triangle may be visible. all its vertices are clipped,
      but part of it may intersect the view volume */
      tlist[i].visible = 1;
      tcount1++;
    }
  }

  /* pass 2 */
  for(i=0;i<pw->tcount;i++) {
    /* if a triangle is visible, none of its vertices can be clipped */
    if(tlist[i].visible) {
      vlist[tlist[i].v0].clipped = CLIPPED_NONE;
      vlist[tlist[i].v1].clipped = CLIPPED_NONE;
      vlist[tlist[i].v2].clipped = CLIPPED_NONE;
    }
  }

  /* pass 3 */
  for(i=0;i<pw->tcount;i++) {
    /* if any triangle that is not visible has a vertex
    that is CLIPPED_NONE, it must be visible. */
    if(!tlist[i].visible) {
      if(!(vlist[tlist[i].v0].clipped & vlist[tlist[i].v1].clipped & vlist[tlist[i].v2].clipped)) {
        tlist[i].visible = 1;
      }
      else ctc++;
    }
  }
  clippedtcount = ctc;
}

static void GenerateNormals() {
  unsigned int i;
  vector v0, v1;
  vertex* vlist = pw->vlist;
  triangle* tlist = pw->tlist;
  float fncountinv;
  vertex *cv, *nv;

  if(shadeState & (SHADE_GOURAUD | SHADE_PHONG | SHADE_VNORMAL)) {
    for(i=0;i<pw->tcount;i++) {
      VectorFromTo(&v0, &vlist[tlist[i].v0], &vlist[tlist[i].v1]);
      VectorFromTo(&v1, &vlist[tlist[i].v0], &vlist[tlist[i].v2]);
      VectorCrossProduct(&tlist[i].normal, &v0, &v1);
      NormalizeVector(&tlist[i].normal);

      /* accumulate vertex normals */
      vlist[tlist[i].v0].normal.x += tlist[i].normal.x;
      vlist[tlist[i].v0].normal.y += tlist[i].normal.y;
      vlist[tlist[i].v0].normal.z += tlist[i].normal.z;
      vlist[tlist[i].v0].fncount++;
      vlist[tlist[i].v1].normal.x += tlist[i].normal.x;
      vlist[tlist[i].v1].normal.y += tlist[i].normal.y;
      vlist[tlist[i].v1].normal.z += tlist[i].normal.z;
      vlist[tlist[i].v1].fncount++;
      vlist[tlist[i].v2].normal.x += tlist[i].normal.x;
      vlist[tlist[i].v2].normal.y += tlist[i].normal.y;
      vlist[tlist[i].v2].normal.z += tlist[i].normal.z;
      vlist[tlist[i].v2].fncount++;
    }

    /* average vertex normals */
    for(i=0;i<pw->vcount;i++) {
      if(vlist[i].fncount != 0) {
        fncountinv = 1.0f / vlist[i].fncount;
        vlist[i].normal.x *= fncountinv;
        vlist[i].normal.y *= fncountinv;
        vlist[i].normal.z *= fncountinv;
        vlist[i].normal.h = 1.0f;
      }
    }
  }
  else {
    for(i=0;i<pw->tcount;i++) {
      if(!tlist[i].visible) continue;
      VectorFromTo(&v0, &vlist[tlist[i].v0], &vlist[tlist[i].v1]);
      VectorFromTo(&v1, &vlist[tlist[i].v0], &vlist[tlist[i].v2]);
      VectorCrossProduct(&tlist[i].normal, &v0, &v1);
      NormalizeVector(&tlist[i].normal);
    }
  }

  if(shadeState & SHADE_TNORMAL) {
    if(!tnvlist || tnvcount < 2 * pw->tcount) tnvlist = realloc(tnvlist, 2 * pw->tcount * sizeof(vertex));
    if(!tnvlist) { printf("malloc failed in GenerateNormals()\n"); exit(1); }

    vlist = pw->vlist;
    tlist = pw->tlist;
    tnvcount = 0;
    for(i=0;i<pw->tcount;i++) {
      tlist[i].pvlist = pw->vlist;
      tlist[i].cv = tnvcount++;
      tlist[i].nv = tnvcount++;
      cv = &tnvlist[tlist[i].cv];
      nv = &tnvlist[tlist[i].nv];
      cv->x = vlist[tlist[i].v0].x;
      cv->y = vlist[tlist[i].v0].y;
      cv->z = vlist[tlist[i].v0].z;
      cv->h = 1.0f;
      cv->x += vlist[tlist[i].v1].x;
      cv->y += vlist[tlist[i].v1].y;
      cv->z += vlist[tlist[i].v1].z;
      cv->x += vlist[tlist[i].v2].x;
      cv->y += vlist[tlist[i].v2].y;
      cv->z += vlist[tlist[i].v2].z;
      cv->x /= 3.0f;
      cv->y /= 3.0f;
      cv->z /= 3.0f;
      memcpy(nv, cv, sizeof(vertex));
      nv->x += tlist[i].normal.x * 5.0f;
      nv->y += tlist[i].normal.y * 5.0f;
      nv->z += tlist[i].normal.z * 5.0f;
      nv->h = 1.0f;

      cv->red = 255.0f;
      cv->green = 0.0f;
      cv->blue = 0.0f;
      nv->red = 255.0f;
      nv->green = 255.0f;
      nv->blue = 255.0f;

      cv->clipped = 0;
      nv->clipped = 0;
    }
  }

  if(shadeState & SHADE_VNORMAL) {
    if(!vnvlist || vnvcount < 2 * pw->vcount) vnvlist = realloc(vnvlist, 2 * pw->vcount * sizeof(vertex));
    if(!vnvlist) { printf("malloc failed in GenerateNormals()\n"); exit(1); }

    vlist = pw->vlist;
    vnvcount = 0;
    for(i=0;i<pw->vcount;i++) {
      cv = &vnvlist[vnvcount++];
      nv = &vnvlist[vnvcount++];
      memcpy(cv, &vlist[i], sizeof(vertex));
      memcpy(nv, &vlist[i], sizeof(vertex));
      nv->x += cv->normal.x * 5.0f;
      nv->y += cv->normal.y * 5.0f;
      nv->z += cv->normal.z * 5.0f;

      cv->red = 255.0f;
      cv->green = 0.0f;
      cv->blue = 0.0f;
      nv->red = 255.0f;
      nv->green = 255.0f;
      nv->blue = 255.0f;

      cv->clipped = 0;
      nv->clipped = 0;
    }
  }
}

static void BackFaceRemove() {
  unsigned int i,ctc=0;
  vector n;
  vertex* vlist = pw->vlist;
  triangle* tlist = pw->tlist;
  float dot;

  for(i=0;i<pw->tcount;i++) {
    if(!tlist[i].visible) continue;
    n.x = vlist[tlist[i].v0].x;
    n.y = vlist[tlist[i].v0].y;
    n.z = vlist[tlist[i].v0].z;
    n.h = 1.0;
    NormalizeVector(&n);
    dot = VectorDotProduct(&n, &tlist[i].normal);
    tlist[i].visible = dot > 0.0 ? 0 : 1;
    if(!tlist[i].visible) ctc++;
  }
  culltcount = ctc;
}

static void Clip() {
  unsigned int i,j,cvc,v0i,v1i,v2i;
  triangle* tlist = pw->tlist;
  vertex* vlist = pw->vlist;
  vertex clippedVList[7];
  static unsigned int mv=32,mt=32;

  vcount2 = tcount2 = 0;
  for(i = 0; i < pw->tcount; i++) {
    tlist[i].pvlist = vlist;
    cvc = ClipTriangleToView(&tlist[i], pcam, clippedVList);

    /* fully clipped? */
    if(!cvc) continue;

    /* a partially clipped triangle (three vertices) may result in up to
    five triangles (seven vertices) */
    if(!vlist2 || vcount2 + 7 > mv) {
      mv <<= 1;
      vlist2 = (vertex*)realloc(vlist2, sizeof(vertex) * mv);
      if(!vlist2) { fprintf(stderr, "malloc failed for newVList in Clip()\n"); exit(1); }
    }
    if(!tlist2 || tcount2 + 5 > mt) {
      mt <<= 1;
      tlist2 = (triangle*)realloc(tlist2, sizeof(triangle) * mt);
      if(!tlist2) { fprintf(stderr, "malloc failed for newTList in Clip()\n"); exit(1); }
    }

    /* add visible vertices and triangles */
    memcpy(&vlist2[vcount2], &clippedVList[0], sizeof(vertex));
    v0i = vcount2++;
    for(j=0;j<cvc-2;j++) {
      memcpy(&vlist2[vcount2], &clippedVList[j + 1], sizeof(vertex));
      v1i = vcount2++;
      memcpy(&vlist2[vcount2], &clippedVList[j + 2], sizeof(vertex));
      v2i = vcount2++;
      memcpy(&tlist2[tcount2], &tlist[i], sizeof(triangle));
      tlist2[tcount2].v0 = v0i;
      tlist2[tcount2].v1 = v1i;
      tlist2[tcount2].v2 = v2i;
      tcount2++;
    }
  }

  pw->vlist = vlist2;
  pw->tlist = tlist2;
  pw->vcount = vcount2;
  pw->tcount = tcount2;

  if(shadeState & SHADE_TNORMAL) {
    for(i = 0; i < tnvcount; i += 2)
      ClipLineToView(&tnvlist[i], &tnvlist[i + 1], pcam);
  }

  if(shadeState & SHADE_VNORMAL) {
    for(i = 0; i < vnvcount; i += 2)
      ClipLineToView(&vnvlist[i], &vnvlist[i + 1], pcam);
  }
}

static void Perspective() {
  unsigned int i;
  float d = pcam->distanceToViewPlane;
  vertex* vlist = pw->vlist;
  for(i=0;i<pw->vcount;i++) {
    vlist[i].x *= d / -vlist[i].z;
    vlist[i].y *= d / -vlist[i].z;
  }

  if(shadeState & SHADE_TNORMAL) {
    for(i=0;i<tnvcount;i++) {
      tnvlist[i].x *= d / -tnvlist[i].z;
      tnvlist[i].y *= d / -tnvlist[i].z;
    }
  }

  if(shadeState & SHADE_VNORMAL) {
    for(i=0;i<vnvcount;i++) {
      vnvlist[i].x *= d / -vnvlist[i].z;
      vnvlist[i].y *= d / -vnvlist[i].z;
    }
  }
}

static int compare(const void* v0, const void* v1) {
  triangle* t0 = *(triangle**)v0;
  triangle* t1 = *(triangle**)v1;
  if(t0->visible && !t1->visible) return -1;
  else if(!t0->visible && t1->visible) return 1;
  return t0->farz > t1->farz ? -1 : 1;
}
static void DepthSort() {
  unsigned int i;
  triangle* tlist = pw->tlist;
  vertex* vlist = pw->vlist;
  static unsigned int mt = 0;

  if(!tpa || pw->tcount > mt) tpa = realloc(tpa, pw->tcount * sizeof(triangle*));
  if(!tpa) { fprintf(stderr, "malloc failed for tpa in DepthSort()\n"); exit(1); }

  mt = pw->tcount;

  /* set up the pointers and farz's */
  for(i=0;i<pw->tcount;i++) {
    tlist[i].farz = vlist[tlist[i].v0].z;
    tpa[i] = &tlist[i];
  }

  /* sort the array of triangle pointers */
  qsort(tpa, pw->tcount, sizeof(triangle*), compare);
}

static void ViewToScreen() {
  unsigned int i;
  vertex* vlist = pw->vlist;
  float swOvpw = screenwidth / pcam->viewPlaneWidth;
  float shOvph = screenheight / pcam->viewPlaneHeight;
  for(i=0;i<pw->vcount;i++) {
    vlist[i].x *= swOvpw;
    vlist[i].y *= shOvph;
    vlist[i].x += screenwidth / 2.0f;
    vlist[i].y += screenheight / 2.0f;
  }

  if(shadeState & SHADE_TNORMAL) {
    for(i=0;i<tnvcount;i++) {
      tnvlist[i].x *= swOvpw;
      tnvlist[i].y *= shOvph;
      tnvlist[i].x += screenwidth / 2.0f;
      tnvlist[i].y += screenheight / 2.0f;
    }
  }

  if(shadeState & SHADE_VNORMAL) {
    for(i=0;i<vnvcount;i++) {
      vnvlist[i].x *= swOvpw;
      vnvlist[i].y *= shOvph;
      vnvlist[i].x += screenwidth / 2.0f;
      vnvlist[i].y += screenheight / 2.0f;
    }
  }
}

static void Render() {
  unsigned int i;
  vertex* vlist = pw->vlist;
  triangle* tlist = pw->tlist;
  extern void* g_pScreen;
  unsigned int zbuffsize = screenwidth * screenheight;
  static unsigned int zz = 0;

  if(!zbuffer || zz < zbuffsize) { zz = zbuffsize; zbuffer = realloc(zbuffer, sizeof(float)*zz); }
  if(!zbuffer) { fprintf(stderr, "malloc failed for zbuffer in Render()\n"); exit(1); }
  memset(zbuffer, 0, sizeof(float) * zbuffsize);
  zz = zbuffsize;

  /* set integer vertices */
  for(i=0;i<pw->vcount;i++) {
    vlist[i].ix = (int)(vlist[i].x+0.5);
    vlist[i].iy = (int)(vlist[i].y+0.5);
  }

  for(i=0;i<pw->tcount;i++) {
    if(!tpa[i]->visible) break;
    tpa[i]->pvlist = vlist;
    ShadeTriangle(tpa[i], &pw->lightList[0], zbuffer, shadeState);
  }

  if(shadeState & SHADE_TNORMAL) {
    for(i=0;i<tnvcount;i+=2) {
      if((tnvlist[i].clipped & tnvlist[i + 1].clipped) != CLIPPED_NONE) continue;
      line3dz(&tnvlist[i], &tnvlist[i + 1], zbuffer);
    }
  }

  if(shadeState & SHADE_VNORMAL) {
    for(i=0;i<vnvcount;i+=2) {
      if((vnvlist[i].clipped & vnvlist[i + 1].clipped) != CLIPPED_NONE) continue;
      line3dz(&vnvlist[i], &vnvlist[i+1], zbuffer);
    }
  }
}

void DrawScene(world* w) {
  pw = w;
  pcam = &w->cameraList[w->currentCamIndex];
  Concatenate();
  WorldToView();
  PreClip();
  GenerateNormals();
  BackFaceRemove();
  Clip();
  Perspective();
  DepthSort();
  ViewToScreen();
  Render();
}
