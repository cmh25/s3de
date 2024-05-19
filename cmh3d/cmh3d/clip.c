#include "clip.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "3d.h"
#include "camera.h"
#include "vertex.h"
#include "triangle.h"

static vertex m_clippedVList[100];
static camera* m_pcam;

static void copyVertex(vertex* v0, vertex* v1);

int ClipTriangleToView(triangle* tri, camera* pcam, vertex* clippedVList) {
  int clippedVCount;
  int i;

  m_pcam = pcam;

  /* initial vertices */
  copyVertex(&clippedVList[0], &tri->pvlist[tri->v0]);
  copyVertex(&clippedVList[1], &tri->pvlist[tri->v1]);
  copyVertex(&clippedVList[2], &tri->pvlist[tri->v2]);
  clippedVCount = 3;

  /* check if triangle is completely clipped or not at all */
  clippedVList[0].clipped = 0;
  clippedVList[1].clipped = 0;
  clippedVList[2].clipped = 0;
  for(i=0;i<6;i++) {
    ClipVertexToPlane(&pcam->clipPlanes[i], &clippedVList[0]);
    ClipVertexToPlane(&pcam->clipPlanes[i], &clippedVList[1]);
    ClipVertexToPlane(&pcam->clipPlanes[i], &clippedVList[2]);
  }
  /* none are clipped? */
  if((clippedVList[0].clipped | clippedVList[1].clipped | clippedVList[2].clipped) == CLIPPED_NONE) {
    /* this triangle is not clipped at all */
  }
  /* all share a common clip plane? */
  else if(clippedVList[0].clipped & clippedVList[1].clipped & clippedVList[2].clipped) {
    /* this triangle is completely clipped */
    clippedVCount = 0;
  }
  /* clip to all planes? */
  else {
    clippedVCount = ClipPolyToPlane(&pcam->clipPlanes[LEFT], clippedVList, clippedVCount);
    clippedVCount = ClipPolyToPlane(&pcam->clipPlanes[RIGHT], clippedVList, clippedVCount);
    clippedVCount = ClipPolyToPlane(&pcam->clipPlanes[TOP], clippedVList, clippedVCount);
    clippedVCount = ClipPolyToPlane(&pcam->clipPlanes[BOTTOM], clippedVList, clippedVCount);
    clippedVCount = ClipPolyToPlane(&pcam->clipPlanes[NEAR], clippedVList, clippedVCount);
    clippedVCount = ClipPolyToPlane(&pcam->clipPlanes[FAR], clippedVList, clippedVCount);
  }

  return clippedVCount;
}

void ClipLineToView(vertex* v0, vertex* v1, camera *pcam) {

  ClipVertexToPlane(&pcam->clipPlanes[LEFT], v0);
  ClipVertexToPlane(&pcam->clipPlanes[RIGHT], v0);
  ClipVertexToPlane(&pcam->clipPlanes[TOP], v0);
  ClipVertexToPlane(&pcam->clipPlanes[BOTTOM], v0);
  ClipVertexToPlane(&pcam->clipPlanes[NEAR], v0);
  ClipVertexToPlane(&pcam->clipPlanes[FAR], v0);

  ClipVertexToPlane(&pcam->clipPlanes[LEFT], v1);
  ClipVertexToPlane(&pcam->clipPlanes[RIGHT], v1);
  ClipVertexToPlane(&pcam->clipPlanes[TOP], v1);
  ClipVertexToPlane(&pcam->clipPlanes[BOTTOM], v1);
  ClipVertexToPlane(&pcam->clipPlanes[NEAR], v1);
  ClipVertexToPlane(&pcam->clipPlanes[FAR], v1);

  if ((v1->clipped & v0->clipped) != CLIPPED_NONE) return;

  if (v1->clipped & CLIPPED_LEFT) ClipLineToPlane(&pcam->clipPlanes[LEFT], v0, v1);
  if (v1->clipped & CLIPPED_RIGHT) ClipLineToPlane(&pcam->clipPlanes[RIGHT], v0, v1);
  if (v1->clipped & CLIPPED_TOP) ClipLineToPlane(&pcam->clipPlanes[TOP], v0, v1);
  if (v1->clipped & CLIPPED_BOTTOM) ClipLineToPlane(&pcam->clipPlanes[BOTTOM], v0, v1);
  if (v1->clipped & CLIPPED_NEAR) ClipLineToPlane(&pcam->clipPlanes[NEAR], v0, v1);
  if (v1->clipped & CLIPPED_FAR) ClipLineToPlane(&pcam->clipPlanes[FAR], v0, v1);

  if (v0->clipped & CLIPPED_LEFT) ClipLineToPlane(&pcam->clipPlanes[LEFT], v1, v0);
  if (v0->clipped & CLIPPED_RIGHT) ClipLineToPlane(&pcam->clipPlanes[RIGHT], v1, v0);
  if (v0->clipped & CLIPPED_TOP) ClipLineToPlane(&pcam->clipPlanes[TOP], v1, v0);
  if (v0->clipped & CLIPPED_BOTTOM) ClipLineToPlane(&pcam->clipPlanes[BOTTOM], v1, v0);
  if (v0->clipped & CLIPPED_NEAR) ClipLineToPlane(&pcam->clipPlanes[NEAR], v1, v0);
  if (v0->clipped & CLIPPED_FAR) ClipLineToPlane(&pcam->clipPlanes[FAR], v1, v0);
}

int ClipPolyToPlane(plane* p, vertex* vList, int count) {
  int i;
  int clippedVCount = 0;
  int lastV1Changed = 1;
  int zeroWasChanged = 0;
  int polyClipAll;
  int polyClipNone;

  /* clip all the vertices
  it's possible for more vertices to be added each time we clip to a plane */
  for(i=0;i<count;i++) {
    vList[i].clipped = 0;
    ClipVertexToPlane(p, &vList[i]);
  }

  /* check if the polygon is completely clipped or not at all clipped */
  polyClipNone = 0;
  polyClipAll = 0;
  for(i=0;i<count;i++) {
    polyClipNone |= vList[i].clipped;
    polyClipAll &= vList[i].clipped;
  }

  if(polyClipNone == CLIPPED_NONE) clippedVCount = count;
  else if(polyClipAll) clippedVCount = 0;
  else {
    /* clip each line v0->v1 */
    for(i=0;i<count-1;i++) {
      if(vList[i].clipped) {
        if(i == 0) zeroWasChanged = 1;
        if(vList[i+1].clipped) {
          /* both: do nothing */
          lastV1Changed = 1;
        }
        else {
          /* v0: copy both */
          copyVertex(&m_clippedVList[clippedVCount], &vList[i]);
          copyVertex(&m_clippedVList[clippedVCount+1], &vList[i+1]);
          /* clip line */
          ClipLineToPlane(p, &m_clippedVList[clippedVCount+1], &m_clippedVList[clippedVCount]);
          clippedVCount += 2;
          lastV1Changed = 0;
        }
      }
      else {
        if(vList[i+1].clipped) {
          if(lastV1Changed) {
            /* v1: copy both */
            copyVertex(&m_clippedVList[clippedVCount], &vList[i]);
            copyVertex(&m_clippedVList[clippedVCount+1], &vList[i+1]);
            /* clip line */
            ClipLineToPlane(p, &m_clippedVList[clippedVCount], &m_clippedVList[clippedVCount+1]);
            clippedVCount += 2;
          }
          else {
            /* copy v1 only */
            copyVertex(&m_clippedVList[clippedVCount], &vList[i+1]);
            /* clip line */
            ClipLineToPlane(p, &m_clippedVList[clippedVCount-1], &m_clippedVList[clippedVCount]);
            clippedVCount += 1;
          }
          lastV1Changed = 1;
        }
        else {
          /* neither */
          if(lastV1Changed) {
            /* copy both */
            copyVertex(&m_clippedVList[clippedVCount++], &vList[i]);
            copyVertex(&m_clippedVList[clippedVCount++], &vList[i+1]);
          }
          else {
            /* copy v1 only */
            copyVertex(&m_clippedVList[clippedVCount++], &vList[i+1]);
          }
          lastV1Changed = 0;
        }
      }
    }
    /* go back and do vn->v0 */
    if(vList[i].clipped) {
      if(vList[0].clipped) {
        /* both: do nothing */
      }
      else {
        /* vn: copy v0 */
        copyVertex(&m_clippedVList[clippedVCount++], &vList[i]);
        if(zeroWasChanged) {
          /* copy zero */
          copyVertex(&m_clippedVList[clippedVCount++], &vList[0]);
          /* clip line vn->vn+1 */
          ClipLineToPlane(p, &m_clippedVList[clippedVCount-1], &m_clippedVList[clippedVCount-2]);
        }
        else {
          /* clip line vn->v0 */
          ClipLineToPlane(p, &m_clippedVList[0], &m_clippedVList[clippedVCount-1]);
        }
      }
    }
    else {
      if(vList[0].clipped) {
        /* v0 */
        if(lastV1Changed) {
          /* copy vn */
          copyVertex(&m_clippedVList[clippedVCount++], &vList[i]);
        }
        if(zeroWasChanged) {
          /* copy zero */
          copyVertex(&m_clippedVList[clippedVCount++], &vList[0]);
          /* clip line vn->vn+1 */
          ClipLineToPlane(p, &m_clippedVList[clippedVCount-2], &m_clippedVList[clippedVCount-1]);
        }
        else {
          /* clip line vn->v0 */
          ClipLineToPlane(p, &m_clippedVList[clippedVCount-1], &m_clippedVList[0]);
        }
      }
      else {
        /* neither */
        if(lastV1Changed) {
          /* copy v0 */
          copyVertex(&m_clippedVList[clippedVCount++], &vList[i]);
        }
        if(zeroWasChanged) {
          /* copy zero */
          copyVertex(&m_clippedVList[clippedVCount++], &vList[0]);
        }
      }
    }
    for(i=0;i<clippedVCount;i++) copyVertex(&vList[i], &m_clippedVList[i]);
  }

  return clippedVCount;
}

void ClipLineToPlane(plane* p, vertex* v0, vertex* v1) {
  /* v1 should be the clipped vertex */
  float tv = 1.0;
  vector n,w,u;

  n.x = p->A;
  n.y = p->B;
  n.z = p->C;

  w.x = v0->x - p->v0->x;
  w.y = v0->y - p->v0->y;
  w.z = v0->z - p->v0->z;

  u.x = v1->x - v0->x;
  u.y = v1->y - v0->y;
  u.z = v1->z - v0->z;

  tv = (-VectorDotProduct(&n, &w)) / (VectorDotProduct(&n, &u));

  v1->x = v0->x + tv * (v1->x - v0->x);
  v1->y = v0->y + tv * (v1->y - v0->y);
  v1->z = v0->z + tv * (v1->z - v0->z);
  v1->normal.x = v0->normal.x + tv * (v1->normal.x - v0->normal.x);
  v1->normal.y = v0->normal.y + tv * (v1->normal.y - v0->normal.y);
  v1->normal.z = v0->normal.z + tv * (v1->normal.z - v0->normal.z);
  v1->normal.h = 1.0;
  v1->u = v0->u + tv * (v1->u - v0->u);
  v1->v = v0->v + tv * (v1->v - v0->v);
  v1->red = v0->red + tv * (v1->red - v0->red);
  v1->green = v0->green + tv * (v1->green - v0->green);
  v1->blue = v0->blue + tv * (v1->blue - v0->blue);
}

void ClipVertexToPlane(plane* p, vertex* v) {
  float r = p->A * v->x + p->B * v->y + p->C * v->z + p->D;
  if(r < 0.0f) {
    switch(p->type) {
    case LEFT: v->clipped |= CLIPPED_LEFT; break;
    case RIGHT: v->clipped |= CLIPPED_RIGHT; break;
    case TOP: v->clipped |= CLIPPED_TOP; break;
    case BOTTOM: v->clipped |= CLIPPED_BOTTOM; break;
    case NEAR: v->clipped |= CLIPPED_NEAR; break;
    case FAR: v->clipped |= CLIPPED_FAR; break;
    }
  }
}

void copyVertex(vertex* v0, vertex* v1) {
  memcpy(v0, v1, sizeof(vertex));
}
