#include "triangle.h"
#include "vertex.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#define SWAP(x,y) do { int t=x;x=y;y=t; } while(0)

static triangle* m_ptri;

/*
  sorts a triangle by rearranging the vertices and returns the type.
  after sorting, the vertices are arranged such that:
    v0 is on top
    v1 is on bottom
    if v2 is on the top or bottom it is put to the right
  the return type is one of the following:
    TT_FLAT_BOTTOM
    TT_FLAT_TOP
    TT_TWO_EDGE_LEFT
    TT_TWO_EDGE_RIGHT
    TT_HORIZONTAL_LINE
    TT_PIXEL */
static int SortTriangle() {
  int type;
  vertex* vlist = m_ptri->pvlist;
  int v0 = m_ptri->v0;
  int v1 = m_ptri->v1;
  int v2 = m_ptri->v2;

  /* put v0 on top and v1 on bottom */
  if((vlist[v1].iy <= vlist[v0].iy) && (vlist[v1].iy <= vlist[v2].iy)) SWAP(v0,v1);
  else if((vlist[v2].iy <= vlist[v0].iy) && (vlist[v2].iy <= vlist[v1].iy)) SWAP(v0,v2);
  if(vlist[v2].iy > vlist[v1].iy) SWAP(v1,v2);

  if(vlist[v0].iy < vlist[v2].iy) {
    /* v0 is on top by itself */
    if(vlist[v1].ix < vlist[v0].ix) {
      if(vlist[v2].iy < vlist[v1].iy) {
        if(vlist[v2].ix <= vlist[v1].ix) type = TT_TWO_EDGE_LEFT;
        else if(vlist[v2].ix >= vlist[v0].ix) type = TT_TWO_EDGE_RIGHT;
        else if(((float)(vlist[v2].iy - vlist[v0].iy) / (float)(vlist[v2].ix - vlist[v0].ix)) >
          ((float)(vlist[v1].iy - vlist[v0].iy) / (float)(vlist[v1].ix - vlist[v0].ix))) {
          type = TT_TWO_EDGE_LEFT;
        }
        else type = TT_TWO_EDGE_RIGHT;
      }
      else if(vlist[v2].ix < vlist[v1].ix) {
        SWAP(v1,v2);
        type = TT_FLAT_BOTTOM;
      }
      else type = TT_FLAT_BOTTOM;
    }
    else if(vlist[v2].iy < vlist[v1].iy) {
      if(vlist[v2].ix <= vlist[v0].ix) type = TT_TWO_EDGE_LEFT;
      else if(vlist[v2].ix >= vlist[v1].ix) type = TT_TWO_EDGE_RIGHT;
      else if(((float)(vlist[v2].iy - vlist[v0].iy) / (float)(vlist[v2].ix - vlist[v0].ix)) <
        ((float)(vlist[v1].iy - vlist[v0].iy) / (float)(vlist[v1].ix - vlist[v0].ix))) {
        type = TT_TWO_EDGE_RIGHT;
      }
      else type = TT_TWO_EDGE_LEFT;
    }
    else if(vlist[v2].ix < vlist[v1].ix) {
      SWAP(v1, v2);
      type = TT_FLAT_BOTTOM;
    }
    else type = TT_FLAT_BOTTOM;
  }
  else {
    /* v0 is not on top by itself
    check for horizontal line */
    if(vlist[v0].iy == vlist[v1].iy) type = TT_HORIZONTAL_LINE;
    else if(vlist[v0].ix < vlist[v2].ix) type = TT_FLAT_TOP;
    else {
      SWAP(v0, v2);
      type = TT_FLAT_TOP;
    }
  }

  m_ptri->v0 = v0;
  m_ptri->v1 = v1;
  m_ptri->v2 = v2;

  return type;
}

static int ScanEdge(int v0x, int v0y, int v1x, int v1y, int* vlist, int edge) {
  int i,j,errorinc,errordec,error,xadd,save_last_pixel=0;
  int deltax = v1x - v0x;
  int deltay = v1y - v0y;
  int abs_deltax = abs(deltax);

  if(deltax == 0) { /* vertical line */
    if(edge) v0x--;
    for(i=0;i<=deltay;i++) vlist[i] = v0x;
  }

  else if(abs_deltax == deltay) { /* diagonal line */
    xadd = (deltax < 0) ? -1 : 1;
    if(edge) v0x--;
    for(i=0;i<=deltay;i++) {
      vlist[i] = v0x;
      v0x += xadd;
    }
  }

  else if(abs_deltax < deltay) { /* y major line */
    errorinc = abs_deltax * 2;
    errordec = errorinc - deltay * 2;
    error = (edge == 0) ? errorinc - 1 : errordec;

    if(deltax < 0) { /* going in backwards */
      i = deltay;
      vlist[i--] = v1x;
      for(;i>=0;i--) {
        if(error >= 0) {
          v1x++;
          error += errordec;
          if((edge) && (error == errordec)) {
            vlist[i] = v1x - 1;
            continue;
          }
        }
        else error += errorinc;
        vlist[i] = v1x;
      }
    }
    else { /* going in forwards */
      i = 0;
      vlist[i++] = (edge) ? v0x - 1 : v0x;
      for(;i<=deltay;i++) {
        if(error >= 0) {
          v0x++;
          error += errordec;
          if((edge) && (error == errordec)) {
            vlist[i] = v0x - 1;
            continue;
          }
        }
        else error += errorinc;
        vlist[i] = v0x;
      }
    }
  }

  else { /* (abs_deltax > deltay) // x major line */
    errorinc = deltay * 2;
    errordec = errorinc - abs_deltax * 2;
    j = 0;
    xadd = (deltax < 0) ? -1 : 1;
    if((deltax < 0 && !edge) || (deltax > 0 && edge)) {
      error = errorinc - 1;
      for(i=0;i<=abs_deltax;i++) {
        if(error >= 0) {
          if((edge) && (error == errorinc - 1)) {
            vlist[j++] = v0x - 1;
            error += errordec;
            v0x += xadd;
            vlist[j] = v0x;
            continue;
          }
          vlist[j++] = v0x;
          error += errordec;
        }
        else {
          error += errorinc;
          if(i == abs_deltax) return save_last_pixel;
        }
        v0x += xadd;
        vlist[j] = v0x;
      }
    }
    else {
      error = errordec;
      save_last_pixel = 1;
      vlist[j] = (edge) ? v0x - 1 : v0x;
      for(i=0;i<=abs(deltax);i++) {
        if(error >= 0) {
          error += errordec;
          v0x += xadd;
          if((edge) && (error == errordec)) {
            vlist[++j] = v0x - 1;
            continue;
          }
          vlist[++j] = v0x;
        }
        else {
          v0x += xadd;
          error += errorinc;
        }
      }
    }
  }
  return save_last_pixel;
}

int ScanTriangleEdges(triangle* ptri, int* pleftedge, int* prightedge) {
  int last_scanned_x;
  m_ptri = ptri;
  vertex* vlist = m_ptri->pvlist;
  int type = SortTriangle();
  int v0x = vlist[m_ptri->v0].ix;
  int v0y = vlist[m_ptri->v0].iy;
  int v1x = vlist[m_ptri->v1].ix;
  int v1y = vlist[m_ptri->v1].iy;
  int v2x = vlist[m_ptri->v2].ix;
  int v2y = vlist[m_ptri->v2].iy;

  switch(type) {
  case TT_FLAT_BOTTOM:
    ScanEdge(v0x, v0y, v1x, v1y, pleftedge, LEFT);
    ScanEdge(v0x, v0y, v2x, v2y, prightedge, RIGHT);
    break;
  case TT_FLAT_TOP:
    ScanEdge(v0x, v0y, v1x, v1y, pleftedge, LEFT);
    ScanEdge(v2x, v2y, v1x, v1y, prightedge, RIGHT);
    break;
  case TT_TWO_EDGE_LEFT:
    if(ScanEdge(v0x, v0y, v2x, v2y, pleftedge, LEFT)) {
      last_scanned_x = pleftedge[v2y - v0y];
      ScanEdge(v2x, v2y, v1x, v1y, (pleftedge + (v2y - v0y)), LEFT);
      pleftedge[v2y - v0y] = last_scanned_x;
    }
    else ScanEdge(v2x, v2y, v1x, v1y, (pleftedge + (v2y - v0y)), LEFT);
    ScanEdge(v0x, v0y, v1x, v1y, prightedge, RIGHT);
    break;
  case TT_TWO_EDGE_RIGHT:
    ScanEdge(v0x, v0y, v1x, v1y, pleftedge, LEFT);
    if(ScanEdge(v0x, v0y, v2x, v2y, prightedge, RIGHT)) {
      last_scanned_x = prightedge[v2y - v0y];
      ScanEdge(v2x, v2y, v1x, v1y, (prightedge + (v2y - v0y)), RIGHT);
      prightedge[v2y - v0y] = last_scanned_x;
    }
    else ScanEdge(v2x, v2y, v1x, v1y, (prightedge + (v2y - v0y)), RIGHT);
    break;
  default: break;
  }
  return type;
}
