#include "world.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
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
#include "engineerror.h"

static vertex *vlist0,*vlist2,*tnvlist,*vnvlist;   /* the concatenated world, its clipped copy, and the normal lines */
static triangle *tlist0,*tlist2,**tpa;
static unsigned int vcount0,vcount2,tcount0,tcount2,clippedvcount,clippedtcount,culltcount,tnvcount,vnvcount;
static unsigned int vcap2,tcap2; /* capacities of vlist2 and tlist2 */
static world* pw;
static camera* pcam;
static light viewLight; /* lightList[0] transformed into view space for the current frame */
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

/* frees what a world owns: its materials' names and textures, its objects' lists, and itself */
static void FreeWorldContents(world* w) {
  unsigned int i;
  for(i=0;i<w->matCount;i++) {
    if(w->materials[i].ptexture) free(w->materials[i].ptexture);
    if(w->materials[i].name) free(w->materials[i].name);
    if(w->materials[i].specTable) free(w->materials[i].specTable);
  }
  for(i=0;i<w->objectCount;i++) {
    if(w->objectList[i].vlist) free(w->objectList[i].vlist);
    if(w->objectList[i].tlist) free(w->objectList[i].tlist);
  }
  free(w);
}

void FreeWorld(world* w) {
  if(!w) return;
  if(vlist0) { free(vlist0); vlist0 = 0; }
  if(vlist2) { free(vlist2); vlist2 = 0; }
  if(tlist0) { free(tlist0); tlist0 = 0; }
  if(tlist2) { free(tlist2); tlist2 = 0; }
  vcount0 = vcount2 = tcount0 = tcount2 = clippedvcount = 0;
  vcap2 = tcap2 = 0;
  FreeWorldContents(w);
  /* a host may load a new world before freeing the old one, so only forget
  the current world when it is the one being freed */
  if(pw == w) pw = 0;
}

world* InitializeWorld(char *fn) {
  world* w;
  ClearEngineError();
  w = (world*)calloc(1,sizeof(world));
  if(!w) { SetEngineError("out of memory allocating the world"); return NULL; }
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
  SetShadeState(SHADE_FLAT | SHADE_AMBIENT | SHADE_DIFFUSE | SHADE_TEXTURE);

  if(fn && !Read3dsFile(fn,w)) {
    /* Read3dsFile() has set the error message */
    FreeWorldContents(w);
    return NULL;
  }
  pw = w;
  /* derive the view plane height and the clip planes from the current screen
  size, so the world renders even if the host never calls SetScreenW/SetScreenH */
  SetScreenH(screenheight);
  return w;
}

static int Concatenate() {
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

  if(pw->vcount && (!vlist0 || vcount0 < pw->vcount)) {
    vertex* grown = (vertex*)realloc(vlist0, pw->vcount * sizeof(vertex));
    if(!grown) { SetEngineError("out of memory for %u vertices in Concatenate()", pw->vcount); return 0; }
    vlist0 = grown;
  }
  if(pw->tcount && (!tlist0 || tcount0 < pw->tcount)) {
    triangle* grown = (triangle*)realloc(tlist0, pw->tcount * sizeof(triangle));
    if(!grown) { SetEngineError("out of memory for %u triangles in Concatenate()", pw->tcount); return 0; }
    tlist0 = grown;
  }

  pw->vlist = vlist0;
  pw->tlist = tlist0;
  vcount0 = pw->vcount;
  tcount0 = pw->tcount;

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
  return 1;
}

static void WorldToView() {
  unsigned int i;
  SetViewMatrix(pcam);
  for(i=0;i<pw->vcount;i++)
    MatrixMultiply1x4_4x4(&pw->vlist[i], pcam->viewMatrix);

  /* the light lives in world space like everything else, but shading happens
  in view space, so bring it along. without this the light followed the camera */
  viewLight = pw->lightList[0];
  MatrixMultiply1x4_4x4(&viewLight.locationVertex, pcam->viewMatrix);
  MatrixMultiply1x4_4x4(&viewLight.shineAtVertex, pcam->viewMatrix);
}

/* marks a triangle whose three vertices all lie outside the same clip plane as
not visible, so the later stages can skip it. Clip() makes the real decision;
this is only a cheap early out, plus the counts shown in the status bar */
static void PreClip() {
  unsigned int i;
  vertex* vlist = pw->vlist;
  triangle* tlist = pw->tlist;
  unsigned int cvc = 0, ctc = 0;

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

  for(i=0;i<pw->tcount;i++) {
    tlist[i].visible = !(vlist[tlist[i].v0].clipped & vlist[tlist[i].v1].clipped & vlist[tlist[i].v2].clipped);
    if(!tlist[i].visible) ctc++;
  }
  clippedtcount = ctc;
}

static int GenerateNormals() {
  unsigned int i;
  vector v0, v1;
  vertex* vlist = pw->vlist;
  triangle* tlist = pw->tlist;
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

    /* the sum of unit face normals is shorter than one wherever faces diverge,
    so normalize rather than average: n dot l needs a unit normal, or corners
    come out too dark */
    for(i=0;i<pw->vcount;i++) {
      if(vlist[i].fncount != 0) {
        NormalizeVector(&vlist[i].normal);
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
    if(pw->tcount && (!tnvlist || tnvcount < 2 * pw->tcount)) {
      vertex* grown = (vertex*)realloc(tnvlist, 2 * pw->tcount * sizeof(vertex));
      if(!grown) { SetEngineError("out of memory for normal lines in GenerateNormals()"); return 0; }
      tnvlist = grown;
    }

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
      nv->x += tlist[i].normal.x * m_tnlen;
      nv->y += tlist[i].normal.y * m_tnlen;
      nv->z += tlist[i].normal.z * m_tnlen;
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
    if(pw->vcount && (!vnvlist || vnvcount < 2 * pw->vcount)) {
      vertex* grown = (vertex*)realloc(vnvlist, 2 * pw->vcount * sizeof(vertex));
      if(!grown) { SetEngineError("out of memory for normal lines in GenerateNormals()"); return 0; }
      vnvlist = grown;
    }

    vlist = pw->vlist;
    vnvcount = 0;
    for(i=0;i<pw->vcount;i++) {
      cv = &vnvlist[vnvcount++];
      nv = &vnvlist[vnvcount++];
      memcpy(cv, &vlist[i], sizeof(vertex));
      memcpy(nv, &vlist[i], sizeof(vertex));
      nv->x += cv->normal.x * m_vnlen;
      nv->y += cv->normal.y * m_vnlen;
      nv->z += cv->normal.z * m_vnlen;

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
  return 1;
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

static int Clip() {
  unsigned int i,j,cvc,v0i,v1i,v2i;
  triangle* tlist = pw->tlist;
  vertex* vlist = pw->vlist;
  vertex clippedVList[MAX_CLIPPED_VERTICES];

  vcount2 = tcount2 = 0;
  for(i = 0; i < pw->tcount; i++) {
    tlist[i].pvlist = vlist;
    cvc = ClipTriangleToView(&tlist[i], pcam, clippedVList);

    /* fully clipped? */
    if(!cvc) continue;

    /* each of the six clip planes can add one vertex, so a triangle comes back
    with up to MAX_CLIPPED_VERTICES vertices, fanned into two fewer triangles.
    the capacities persist across frames and are reset by FreeWorld() */
    if(vcount2 + MAX_CLIPPED_VERTICES > vcap2) {
      unsigned int cap = vcap2 ? vcap2 * 2 : 256;
      vertex* grown;
      while(vcount2 + MAX_CLIPPED_VERTICES > cap) cap *= 2;
      grown = (vertex*)realloc(vlist2, sizeof(vertex) * cap);
      if(!grown) { SetEngineError("out of memory for %u clipped vertices in Clip()", cap); return 0; }
      vlist2 = grown;
      vcap2 = cap;
    }
    if(tcount2 + MAX_CLIPPED_VERTICES - 2 > tcap2) {
      unsigned int cap = tcap2 ? tcap2 * 2 : 256;
      triangle* grown;
      while(tcount2 + MAX_CLIPPED_VERTICES - 2 > cap) cap *= 2;
      grown = (triangle*)realloc(tlist2, sizeof(triangle) * cap);
      if(!grown) { SetEngineError("out of memory for %u clipped triangles in Clip()", cap); return 0; }
      tlist2 = grown;
      tcap2 = cap;
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
  return 1;
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
static int DepthSort() {
  unsigned int i;
  triangle* tlist = pw->tlist;
  vertex* vlist = pw->vlist;
  static unsigned int mt = 0; /* capacity of tpa */

  if(pw->tcount > mt) {
    triangle** grown = (triangle**)realloc(tpa, pw->tcount * sizeof(triangle*));
    if(!grown) { SetEngineError("out of memory for %u triangle pointers in DepthSort()", pw->tcount); return 0; }
    tpa = grown;
    mt = pw->tcount;
  }

  /* set up the pointers and farz's */
  for(i=0;i<pw->tcount;i++) {
    tlist[i].farz = vlist[tlist[i].v0].z;
    tpa[i] = &tlist[i];
  }

  /* sort the array of triangle pointers */
  if(pw->tcount > 1) qsort(tpa, pw->tcount, sizeof(triangle*), compare);
  return 1;
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

static int Render() {
  unsigned int i;
  vertex* vlist = pw->vlist;
  unsigned int zbuffsize = screenwidth * screenheight;
  static unsigned int zz = 0; /* capacity of zbuffer */

  if(zbuffsize > zz) {
    float* grown = (float*)realloc(zbuffer, sizeof(float) * zbuffsize);
    if(!grown) { SetEngineError("out of memory for a %dx%d z-buffer in Render()", screenwidth, screenheight); return 0; }
    zbuffer = grown;
    zz = zbuffsize;
  }
  if(zbuffer) memset(zbuffer, 0, sizeof(float) * zbuffsize);

  /* set integer vertices */
  for(i=0;i<pw->vcount;i++) {
    vlist[i].ix = (int)(vlist[i].x+0.5);
    vlist[i].iy = (int)(vlist[i].y+0.5);
  }

  SetShadeLight(&viewLight);
  for(i=0;i<pw->tcount;i++) {
    if(!tpa[i]->visible) break;
    tpa[i]->pvlist = vlist;
    ShadeTriangle(tpa[i], zbuffer, shadeState);
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
  return 1;
}

int DrawScene(world* w) {
  ClearEngineError();
  if(!w) { SetEngineError("DrawScene() called without a world"); return 0; }
  pw = w;
  pcam = &w->cameraList[w->currentCamIndex];
  if(!Concatenate()) return 0;
  WorldToView();
  PreClip();
  if(!GenerateNormals()) return 0;
  BackFaceRemove();
  if(!Clip()) return 0;
  Perspective();
  if(!DepthSort()) return 0;
  ViewToScreen();
  return Render();
}
