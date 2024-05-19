#include "shade.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include "shade.h"
#include "vector.h"
#include "vertex.h"
#include "triangle.h"
#include "light.h"
#include "camera.h"
#include "3d.h"
#include "world.h"
#include "material.h"

static void (*ppixel)(int x, int y, int r, int g, int b);

static int m_pleftEdgeX[10000];
static int m_prightEdgeX[10000];
static float m_pleftEdgeRed[10000];
static float m_pleftEdgeGreen[10000];
static float m_pleftEdgeBlue[10000];
static float m_prightEdgeRed[10000];
static float m_prightEdgeGreen[10000];
static float m_prightEdgeBlue[10000];
static float m_pleftEdgeNX[10000];
static float m_pleftEdgeNY[10000];
static float m_pleftEdgeNZ[10000];
static float m_prightEdgeNX[10000];
static float m_prightEdgeNY[10000];
static float m_prightEdgeNZ[10000];
static float m_pleftEdgeU[10000];
static float m_prightEdgeU[10000];
static float m_pleftEdgeV[10000];
static float m_prightEdgeV[10000];
static float m_pleftEdge1OverZ[10000];
static float m_prightEdge1OverZ[10000];
static float m_hRed[20000];
static float m_hGreen[20000];
static float m_hBlue[20000];
static float m_hNX[20000];
static float m_hNY[20000];
static float m_hNZ[20000];
static float m_hU[20000];
static float m_hV[20000];
static float m_h1OverZ[20000];

static triangle* m_ptri;
static int m_ystart;
static light* m_plight;
static float* m_zbuffer;
static int m_tritype;
static int m_color;
static int m_nhlines;
static vector m_toLight;
static int m_shadeState;

static float m_ambientRed;
static float m_ambientGreen;
static float m_ambientBlue;
static float m_red;
static float m_green;
static float m_blue;

static material* m_pmat;

static float m_specComp;

static int m_screenwidth, m_screenheight;
static camera* m_pcam;

static void pixel(int x, int y, int r, int g, int b) {
  if(x<0 || y<0) return;
  if(x>=m_screenwidth || y>=m_screenheight) return;
  ppixel(x,y,r,g,b);
}

static void InterpolateFloat(float start, float stop, float* buffer, int num) {
  int i;
  float inc = (stop-start)/(float)num;
  buffer[0] = start;
  for(i=1;i<num;i++) buffer[i] = buffer[i-1]+inc;
}

static void ScanEdge1OverZ() {
  vertex* vlist = m_ptri->pvlist;
  vertex* v0 = &vlist[m_ptri->v0];
  vertex* v1 = &vlist[m_ptri->v1];
  vertex* v2 = &vlist[m_ptri->v2];

  float v0z = 1.0f / v0->z;
  float v1z = 1.0f / v1->z;
  float v2z = 1.0f / v2->z;

  switch(m_tritype) {
  case TT_FLAT_BOTTOM:
    InterpolateFloat(v0z, v1z, m_pleftEdge1OverZ, m_nhlines);
    InterpolateFloat(v0z, v2z, m_prightEdge1OverZ, m_nhlines);
    break;
  case TT_FLAT_TOP:
    InterpolateFloat(v0z, v1z, m_pleftEdge1OverZ, m_nhlines);
    InterpolateFloat(v2z, v1z, m_prightEdge1OverZ, m_nhlines);
    break;
  case TT_TWO_EDGE_LEFT:
    InterpolateFloat(v0z, v2z, m_pleftEdge1OverZ, v2->iy - v0->iy);
    InterpolateFloat(v2z, v1z, &m_pleftEdge1OverZ[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0z, v1z, m_prightEdge1OverZ, m_nhlines);
    break;
  case TT_TWO_EDGE_RIGHT:
    InterpolateFloat(v0z, v1z, m_pleftEdge1OverZ, m_nhlines);
    InterpolateFloat(v0z, v2z, m_prightEdge1OverZ, v2->iy - v0->iy);
    InterpolateFloat(v2z, v1z, &m_prightEdge1OverZ[v2->iy - v0->iy], v1->iy - v2->iy);
    break;
  case TT_HORIZONTAL_LINE:
    break;
  case TT_PIXEL:
    break;
  default:
    fprintf(stderr, "unknown m_tritype in ScanEdge1OverZ()\n");
    exit(1);
  }
}

static void ScanEdgeColors() {
  vertex* vlist = m_ptri->pvlist;
  vertex* v0 = &vlist[m_ptri->v0];
  vertex* v1 = &vlist[m_ptri->v1];
  vertex* v2 = &vlist[m_ptri->v2];

  switch(m_tritype) {
  case TT_FLAT_BOTTOM:
    InterpolateFloat(v0->red, v1->red, m_pleftEdgeRed, m_nhlines);
    InterpolateFloat(v0->green, v1->green, m_pleftEdgeGreen, m_nhlines);
    InterpolateFloat(v0->blue, v1->blue, m_pleftEdgeBlue, m_nhlines);
    InterpolateFloat(v0->red, v2->red, m_prightEdgeRed, m_nhlines);
    InterpolateFloat(v0->green, v2->green, m_prightEdgeGreen, m_nhlines);
    InterpolateFloat(v0->blue, v2->blue, m_prightEdgeBlue, m_nhlines);
    break;
  case TT_FLAT_TOP:
    InterpolateFloat(v0->red, v1->red, m_pleftEdgeRed, m_nhlines);
    InterpolateFloat(v0->green, v1->green, m_pleftEdgeGreen, m_nhlines);
    InterpolateFloat(v0->blue, v1->blue, m_pleftEdgeBlue, m_nhlines);
    InterpolateFloat(v2->red, v1->red, m_prightEdgeRed, m_nhlines);
    InterpolateFloat(v2->green, v1->green, m_prightEdgeGreen, m_nhlines);
    InterpolateFloat(v2->blue, v1->blue, m_prightEdgeBlue, m_nhlines);
    break;
  case TT_TWO_EDGE_LEFT:
    InterpolateFloat(v0->red, v2->red, m_pleftEdgeRed, v2->iy - v0->iy);
    InterpolateFloat(v2->red, v1->red, &m_pleftEdgeRed[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0->green, v2->green, m_pleftEdgeGreen, v2->iy - v0->iy);
    InterpolateFloat(v2->green, v1->green, &m_pleftEdgeGreen[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0->blue, v2->blue, m_pleftEdgeBlue, v2->iy - v0->iy);
    InterpolateFloat(v2->blue, v1->blue, &m_pleftEdgeBlue[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0->red, v1->red, m_prightEdgeRed, m_nhlines);
    InterpolateFloat(v0->green, v1->green, m_prightEdgeGreen, m_nhlines);
    InterpolateFloat(v0->blue, v1->blue, m_prightEdgeBlue, m_nhlines);
    break;
  case TT_TWO_EDGE_RIGHT:
    InterpolateFloat(v0->red, v1->red, m_pleftEdgeRed, m_nhlines);
    InterpolateFloat(v0->green, v1->green, m_pleftEdgeGreen, m_nhlines);
    InterpolateFloat(v0->blue, v1->blue, m_pleftEdgeBlue, m_nhlines);
    InterpolateFloat(v0->red, v2->red, m_prightEdgeRed, v2->iy - v0->iy);
    InterpolateFloat(v2->red,v1->red,&m_prightEdgeRed[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0->green, v2->green, m_prightEdgeGreen, v2->iy - v0->iy);
    InterpolateFloat(v2->green,v1->green,&m_prightEdgeGreen[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0->blue, v2->blue, m_prightEdgeBlue, v2->iy - v0->iy);
    InterpolateFloat(v2->blue,v1->blue,&m_prightEdgeBlue[v2->iy - v0->iy], v1->iy - v2->iy);
    break;
  case TT_HORIZONTAL_LINE:
    break;
  case TT_PIXEL:
    break;
  default:
    fprintf(stderr, "unknown m_tritype in ScanEdgeColors()\n");
    exit(1);
  }
}

static void ScanEdgeNormals() {
  vertex* vlist = m_ptri->pvlist;
  vertex* v0 = &vlist[m_ptri->v0];
  vertex* v1 = &vlist[m_ptri->v1];
  vertex* v2 = &vlist[m_ptri->v2];
  vector* v0n = &vlist[m_ptri->v0].normal;
  vector* v1n = &vlist[m_ptri->v1].normal;
  vector* v2n = &vlist[m_ptri->v2].normal;

  switch(m_tritype) {
  case TT_FLAT_BOTTOM:
    InterpolateFloat(v0n->x, v1n->x, m_pleftEdgeNX, m_nhlines);
    InterpolateFloat(v0n->y, v1n->y, m_pleftEdgeNY, m_nhlines);
    InterpolateFloat(v0n->z, v1n->z, m_pleftEdgeNZ, m_nhlines);
    InterpolateFloat(v0n->x, v2n->x, m_prightEdgeNX, m_nhlines);
    InterpolateFloat(v0n->y, v2n->y, m_prightEdgeNY, m_nhlines);
    InterpolateFloat(v0n->z, v2n->z, m_prightEdgeNZ, m_nhlines);
    break;
  case TT_FLAT_TOP:
    InterpolateFloat(v0n->x, v1n->x, m_pleftEdgeNX, m_nhlines);
    InterpolateFloat(v0n->y, v1n->y, m_pleftEdgeNY, m_nhlines);
    InterpolateFloat(v0n->z, v1n->z, m_pleftEdgeNZ, m_nhlines);
    InterpolateFloat(v2n->x, v1n->x, m_prightEdgeNX, m_nhlines);
    InterpolateFloat(v2n->y, v1n->y, m_prightEdgeNY, m_nhlines);
    InterpolateFloat(v2n->z, v1n->z, m_prightEdgeNZ, m_nhlines);
    break;
  case TT_TWO_EDGE_LEFT:
    InterpolateFloat(v0n->x, v2n->x, m_pleftEdgeNX, v2->iy - v0->iy);
    InterpolateFloat(v2n->x, v1n->x, &m_pleftEdgeNX[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0n->y, v2n->y, m_pleftEdgeNY, v2->iy - v0->iy);
    InterpolateFloat(v2n->y, v1n->y, &m_pleftEdgeNY[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0n->z, v2n->z, m_pleftEdgeNZ, v2->iy - v0->iy);
    InterpolateFloat(v2n->z, v1n->z, &m_pleftEdgeNZ[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0n->x, v1n->x, m_prightEdgeNX, m_nhlines);
    InterpolateFloat(v0n->y, v1n->y, m_prightEdgeNY, m_nhlines);
    InterpolateFloat(v0n->z, v1n->z, m_prightEdgeNZ, m_nhlines);
    break;
  case TT_TWO_EDGE_RIGHT:
    InterpolateFloat(v0n->x, v1n->x, m_pleftEdgeNX, m_nhlines);
    InterpolateFloat(v0n->y, v1n->y, m_pleftEdgeNY, m_nhlines);
    InterpolateFloat(v0n->z, v1n->z, m_pleftEdgeNZ, m_nhlines);
    InterpolateFloat(v0n->x, v2n->x, m_prightEdgeNX, v2->iy - v0->iy);
    InterpolateFloat(v2n->x, v1n->x, &m_prightEdgeNX[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0n->y, v2n->y, m_prightEdgeNY, v2->iy - v0->iy);
    InterpolateFloat(v2n->y, v1n->y, &m_prightEdgeNY[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0n->z, v2n->z, m_prightEdgeNZ, v2->iy - v0->iy);
    InterpolateFloat(v2n->z, v1n->z, &m_prightEdgeNZ[v2->iy - v0->iy], v1->iy - v2->iy);
    break;
  case TT_HORIZONTAL_LINE:
    break;
  case TT_PIXEL:
    break;
  default:
    fprintf(stderr, "unknown m_tritype in ScanEdgeNormals()\n");
    exit(1);
  }
}

static void ScanEdgeUV() {
  vertex* vlist = m_ptri->pvlist;
  vertex* v0 = &vlist[m_ptri->v0];
  vertex* v1 = &vlist[m_ptri->v1];
  vertex* v2 = &vlist[m_ptri->v2];

  float v0u = -v0->u/v0->z;
  float v0v = -v0->v/v0->z;
  float v1u = -v1->u/v1->z;
  float v1v = -v1->v/v1->z;
  float v2u = -v2->u/v2->z;
  float v2v = -v2->v/v2->z;

  switch(m_tritype) {
  case TT_FLAT_BOTTOM:
    InterpolateFloat(v0u, v1u, m_pleftEdgeU, m_nhlines);
    InterpolateFloat(v0u, v2u, m_prightEdgeU, m_nhlines);
    InterpolateFloat(v0v, v1v, m_pleftEdgeV, m_nhlines);
    InterpolateFloat(v0v, v2v, m_prightEdgeV, m_nhlines);
    break;
  case TT_FLAT_TOP:
    InterpolateFloat(v0u, v1u, m_pleftEdgeU, m_nhlines);
    InterpolateFloat(v2u, v1u, m_prightEdgeU, m_nhlines);
    InterpolateFloat(v0v, v1v, m_pleftEdgeV, m_nhlines);
    InterpolateFloat(v2v, v1v, m_prightEdgeV, m_nhlines);
    break;
  case TT_TWO_EDGE_LEFT:
    InterpolateFloat(v0u, v2u, m_pleftEdgeU, v2->iy - v0->iy);
    InterpolateFloat(v2u, v1u, &m_pleftEdgeU[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0u, v1u, m_prightEdgeU, m_nhlines);
    InterpolateFloat(v0v, v2v, m_pleftEdgeV, v2->iy - v0->iy);
    InterpolateFloat(v2v, v1v, &m_pleftEdgeV[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0v, v1v, m_prightEdgeV, m_nhlines);
    break;
  case TT_TWO_EDGE_RIGHT:
    InterpolateFloat(v0u, v1u, m_pleftEdgeU, m_nhlines);
    InterpolateFloat(v0u, v2u, m_prightEdgeU, v2->iy - v0->iy);
    InterpolateFloat(v2u, v1u, &m_prightEdgeU[v2->iy - v0->iy], v1->iy - v2->iy);
    InterpolateFloat(v0v, v1v, m_pleftEdgeV, m_nhlines);
    InterpolateFloat(v0v, v2v, m_prightEdgeV, v2->iy - v0->iy);
    InterpolateFloat(v2v, v1v, &m_prightEdgeV[v2->iy - v0->iy], v1->iy - v2->iy);
    break;
  case TT_HORIZONTAL_LINE:
    break;
  case TT_PIXEL:
    break;
  default:
    fprintf(stderr, "unknown m_tritype in ScanEdgeUV()\n");
    exit(1);
  }
}

static void CalcAmbient() {
  m_red = m_pmat->ka * m_ambientRed;
  m_green = m_pmat->ka * m_ambientGreen;
  m_blue = m_pmat->ka * m_ambientBlue;
}

static void CalcDiffuse(vector* normal) {
  float dot;

  VectorFromTo(&m_toLight, &m_plight->shineAtVertex, &m_plight->locationVertex);
  NormalizeVector(&m_toLight);

  /* abs(-L dot N) is somewhere between 0 and 1
  multiply that value by each m_color component */
  dot = VectorDotProduct(normal, &m_toLight);
  /* shading normals are not in perspective space, so it is possible to
  have a negative dot product. */
  if(dot<0.0) dot = 0.0;

  m_red += m_pmat->kd * m_plight->red * dot;
  m_green += m_pmat->kd * m_plight->green * dot;
  m_blue += m_pmat->kd * m_plight->blue * dot;
}

static void CalcSpecular(vector* normal) {
  float dot;

  VectorFromTo(&m_toLight, &m_plight->shineAtVertex, &m_plight->locationVertex);
  NormalizeVector(&m_toLight);

  /* abs(-L dot N) is somewhere between 0 and 1
  multiply that value by each m_color component */
  dot = fabsf(VectorDotProduct(normal, &m_toLight));
  m_specComp = m_pmat->ks * powf(dot, m_pmat->ns) * 255.0f;
}

static void CalColorIntensity(vector* normal) {
  m_red = m_green = m_blue = 0.0;
  if(m_shadeState & SHADE_AMBIENT) CalcAmbient();
  if(m_shadeState & SHADE_DIFFUSE) CalcDiffuse(normal);
  if(m_shadeState & SHADE_SPECULAR) CalcSpecular(normal);
}

static void SetVertexColors() {
  vertex* vlist = m_ptri->pvlist;

  VectorFromTo(&m_toLight, &m_plight->shineAtVertex, &m_plight->locationVertex);
  NormalizeVector(&m_toLight);

  CalColorIntensity(&vlist[m_ptri->v0].normal);
  vlist[m_ptri->v0].red = m_red;
  vlist[m_ptri->v0].green = m_green;
  vlist[m_ptri->v0].blue = m_blue;

  CalColorIntensity(&vlist[m_ptri->v1].normal);
  vlist[m_ptri->v1].red = m_red;
  vlist[m_ptri->v1].green = m_green;
  vlist[m_ptri->v1].blue = m_blue;

  CalColorIntensity(&vlist[m_ptri->v2].normal);
  vlist[m_ptri->v2].red = m_red;
  vlist[m_ptri->v2].green = m_green;
  vlist[m_ptri->v2].blue = m_blue;
}

static int CalcColorFromMaterial(int j) {
  int baseIndex, index0, index1, index2, index3;
  float u0, v0, u1, v1;
  float red0, red1, red2, red3, green0, green1, green2, green3, blue0, blue1, blue2, blue3;
  float redTotal, greenTotal, blueTotal;
  float weight00, weight01, weight10, weight11, weightBottom, weightTop, weightLeft, weightRight;
 
  if(m_pmat->ptexture && m_shadeState & SHADE_TEXTURE) {
    m_h1OverZ[j] = 1.0f / m_h1OverZ[j];
    m_hU[j] *= -m_h1OverZ[j];
    m_hV[j] *= -m_h1OverZ[j];

    m_hU[j] *= m_pmat->uScale;
    m_hV[j] *= m_pmat->vScale;

    if(m_hU[j] > 1.0)m_hU[j] -= (int)m_hU[j];
    if(m_hV[j] > 1.0)m_hV[j] -= (int)m_hV[j];

    /* we can apply a procedure to the texture here if we want
    by manipulating m_hU[j] and m_hV[j] */

    /* normalize to [0,1], handle tiling */
    m_hV[j] *= -1;
    while (m_hU[j] < 0.0f) m_hU[j] += 1.0f;
    while (m_hV[j] < 0.0f) m_hV[j] += 1.0f;
    while (m_hU[j] > 1.0f) m_hU[j] -= 1.0f;
    while (m_hV[j] > 1.0f) m_hV[j] -= 1.0f;

    m_hU[j] *= (float)(m_pmat->textureWidth - 1);
    m_hV[j] *= (float)(m_pmat->textureHeight - 1);
    
    if(m_shadeState & SHADE_BILINEAR) {
      u0 = floorf(m_hU[j]);
      v0 = floorf(m_hV[j]);
      u1 = floorf(m_hU[j] + 1.0f);
      v1 = floorf(m_hV[j] + 1.0f);
 
      weightBottom = m_hV[j] - v0;
      weightTop = 1.0f - weightBottom;
      weightRight = m_hU[j] - u0;
      weightLeft = 1.0f - weightRight;
      weight00 = weightTop * weightLeft;
      weight01 = weightTop * weightRight;
      weight10 = weightBottom * weightLeft;
      weight11 = weightBottom * weightRight;
 
      u0 *= 3.0f;
      u1 *= 3.0f;
      v0 *= 3.0f * m_pmat->textureWidth;
      v1 *= 3.0f * m_pmat->textureWidth;

      index0 = (int)v0 + (int)u0;
      index1 = (int)v0 + (int)u1;
      index2 = (int)v1 + (int)u0;
      index3 = (int)v1 + (int)u1;
 
      red0 = m_pmat->ptexture[index0] * weight00;
      red1 = m_pmat->ptexture[index1] * weight01;
      red2 = m_pmat->ptexture[index2] * weight10;
      red3 = m_pmat->ptexture[index3] * weight11;
 
      green0 = m_pmat->ptexture[index0 + 1] * weight00;
      green1 = m_pmat->ptexture[index1 + 1] * weight01;
      green2 = m_pmat->ptexture[index2 + 1] * weight10;
      green3 = m_pmat->ptexture[index3 + 1] * weight11;
 
      blue0 = m_pmat->ptexture[index0 + 2] * weight00;
      blue1 = m_pmat->ptexture[index1 + 2] * weight01;
      blue2 = m_pmat->ptexture[index2 + 2] * weight10;
      blue3 = m_pmat->ptexture[index3 + 2] * weight11;
 
      redTotal = red0 + red1 + red2 + red3;
      greenTotal = green0 + green1 + green2 + green3;
      blueTotal = blue0 + blue1 + blue2 + blue3;
 
      m_hRed[j] *= redTotal;
      m_hGreen[j] *= greenTotal;
      m_hBlue[j] *= blueTotal;
    }
    else {
      baseIndex = (int)(m_hV[j]+0.5) * m_pmat->textureWidth * 3 + (int)(m_hU[j]+0.5) * 3;
      m_hRed[j] *= (float)m_pmat->ptexture[baseIndex];
      m_hGreen[j] *= (float)m_pmat->ptexture[baseIndex + 1];
      m_hBlue[j] *= (float)m_pmat->ptexture[baseIndex + 2];
    }
  }
  /* if there is no texture associated with the material
  the material's color is used */
  else {
    m_hRed[j] *= m_pmat->r;
    m_hGreen[j] *= m_pmat->g;
    m_hBlue[j] *= m_pmat->b;
  }

  /* add the specular component */
  m_hRed[j] += m_specComp;
  m_hGreen[j] += m_specComp;
  m_hBlue[j] += m_specComp;

  /* make sure none of the colors are over 255.0 */
  if(m_hRed[j] > 255.0) m_hRed[j] = 255.0;
  if(m_hGreen[j] > 255.0) m_hGreen[j] = 255.0;
  if(m_hBlue[j] > 255.0) m_hBlue[j] = 255.0;

  return 0;
}

static void line_(int x0, int y0, int x1, int y1,
  int dx, int dy, int d, int r, int g, int b) {
  int e = 2 * dy - dx;
  int sh = GetScreenH();
  for(int i = 0; i <= dx; i++) {
    if(d && x0 < sh) pixel(y0, x0, r, g, b);
    else if(!d && y0 < sh) pixel(x0, y0, r, g, b);
    x0 < x1 ? x0++ : x0--;
    if(e < 0) {
      e = e + 2 * dy;
    }
    else {
      y0 < y1 ? y0++ : y0--;
      e = e + 2 * dy - 2 * dx;
    }
  }
}

static void ShadeVertex() {
  vertex* vlist = m_ptri->pvlist;
  m_red = m_green = m_blue = 255.0;
  pixel(vlist[m_ptri->v0].ix, vlist[m_ptri->v0].iy, (int)m_red, (int)m_green, (int)m_blue);
  pixel(vlist[m_ptri->v1].ix, vlist[m_ptri->v1].iy, (int)m_red, (int)m_green, (int)m_blue);
  pixel(vlist[m_ptri->v2].ix, vlist[m_ptri->v2].iy, (int)m_red, (int)m_green, (int)m_blue);
}

static void ShadeWireframe() {
  vertex* vlist = m_ptri->pvlist;
  m_red = m_green = m_blue = 255.0;
  line(vlist[m_ptri->v0].ix, vlist[m_ptri->v0].iy, vlist[m_ptri->v1].ix, vlist[m_ptri->v1].iy, (int)m_red, (int)m_green, (int)m_blue);
  line(vlist[m_ptri->v1].ix, vlist[m_ptri->v1].iy, vlist[m_ptri->v2].ix, vlist[m_ptri->v2].iy, (int)m_red, (int)m_green, (int)m_blue);
  line(vlist[m_ptri->v2].ix, vlist[m_ptri->v2].iy, vlist[m_ptri->v0].ix, vlist[m_ptri->v0].iy, (int)m_red, (int)m_green, (int)m_blue);
}

static void ShadeFlat() {
  int i, j, zbi;
  int width;

  CalColorIntensity(&m_ptri->normal);
  ScanEdge1OverZ();

  m_hRed[0] = m_red;
  m_hGreen[0] = m_green;
  m_hBlue[0] = m_blue;
  m_color = CalcColorFromMaterial(0);

  for(i=0;i<m_nhlines;i++) {
    if(m_ystart >= m_screenheight) break;;
    if(m_ystart < 0) { m_ystart++;  continue; }
    if(m_prightEdgeX[i] >= m_screenwidth) { m_prightEdgeX[i] = m_screenwidth-1; }
    if(m_pleftEdgeX[i] < 0) m_pleftEdgeX[i] = 0;
    width = m_prightEdgeX[i] - m_pleftEdgeX[i] + 1;
    InterpolateFloat(m_pleftEdgeU[i], m_prightEdgeU[i], m_hU, width);
    InterpolateFloat(m_pleftEdgeV[i], m_prightEdgeV[i], m_hV, width);
    InterpolateFloat(m_pleftEdge1OverZ[i], m_prightEdge1OverZ[i], m_h1OverZ, width);
    zbi = m_ystart * m_screenwidth + m_pleftEdgeX[i];
    for(j=0;j<width;j++,zbi++) {
      if(m_h1OverZ[j] < m_zbuffer[zbi] && m_h1OverZ[j] > 1.0f / m_pcam->clipNear) {
        m_zbuffer[zbi] = m_h1OverZ[j];
        pixel(m_pleftEdgeX[i] + j, m_ystart, (int)m_hRed[0], (int)m_hGreen[0], (int)m_hBlue[0]);
      }
    }

    m_ystart++;
  }
}

static void ShadeFlatT() {
  int i,j,zbi,width;

  CalColorIntensity(&m_ptri->normal);
  ScanEdgeUV();
  ScanEdge1OverZ();

  for(i=0;i<m_nhlines;i++) {
    if(m_ystart >= m_screenheight) break;;
    if(m_ystart < 0) { m_ystart++;  continue; }
    if(m_prightEdgeX[i] >= m_screenwidth) { m_prightEdgeX[i] = m_screenwidth - 1; }
    if(m_pleftEdgeX[i] < 0) m_pleftEdgeX[i] = 0;
    width = m_prightEdgeX[i] - m_pleftEdgeX[i] + 1;
    InterpolateFloat(m_pleftEdgeU[i], m_prightEdgeU[i], m_hU, width);
    InterpolateFloat(m_pleftEdgeV[i], m_prightEdgeV[i], m_hV, width);
    InterpolateFloat(m_pleftEdge1OverZ[i], m_prightEdge1OverZ[i], m_h1OverZ, width);
    zbi = m_ystart * m_screenwidth + m_pleftEdgeX[i];
    for(j=0;j<width;j++,zbi++) {
      if(m_h1OverZ[j] < m_zbuffer[zbi]) {
        m_zbuffer[zbi] = m_h1OverZ[j];
        m_hRed[j] = m_red;
        m_hGreen[j] = m_green;
        m_hBlue[j] = m_blue;
        m_color = CalcColorFromMaterial(j);
        pixel(m_pleftEdgeX[i] + j, m_ystart, (int)m_hRed[j], (int)m_hGreen[j], (int)m_hBlue[j]);
      }
    }
    m_ystart++;
  }
}

static void ShadeGouraud() {
  int i,j,width;

  ScanEdgeUV();
  ScanEdge1OverZ();
  SetVertexColors();
  ScanEdgeColors();

  for(i=0;i<m_nhlines;i++) {
    if(m_ystart >= m_screenheight) break;;
    if(m_ystart < 0) { m_ystart++;  continue; }
    if(m_prightEdgeX[i] >= m_screenwidth) { m_prightEdgeX[i] = m_screenwidth - 1; }
    if(m_pleftEdgeX[i] < 0) m_pleftEdgeX[i] = 0;
    width = m_prightEdgeX[i] - m_pleftEdgeX[i] + 1;
    InterpolateFloat(m_pleftEdgeU[i], m_prightEdgeU[i], m_hU, width);
    InterpolateFloat(m_pleftEdgeV[i], m_prightEdgeV[i], m_hV, width);
    InterpolateFloat(m_pleftEdge1OverZ[i], m_prightEdge1OverZ[i], m_h1OverZ, width);
    InterpolateFloat(m_pleftEdgeRed[i], m_prightEdgeRed[i], m_hRed, width);
    InterpolateFloat(m_pleftEdgeGreen[i], m_prightEdgeGreen[i], m_hGreen, width);
    InterpolateFloat(m_pleftEdgeBlue[i], m_prightEdgeBlue[i], m_hBlue, width);

    for(j=0;j<width;j++) {
      if(m_h1OverZ[j] < m_zbuffer[m_ystart * m_screenwidth + m_pleftEdgeX[i] + j]) {
        m_zbuffer[m_ystart * m_screenwidth + m_pleftEdgeX[i] + j] = m_h1OverZ[j];
        m_color = CalcColorFromMaterial(j);
        pixel(m_pleftEdgeX[i] + j, m_ystart, (int)m_hRed[j], (int)m_hGreen[j], (int)m_hBlue[j]);
      }
    }
    m_ystart++;
  }
}

static void ShadePhong() {
  int i,j,width,m_color;
  vector currentNormal;

  ScanEdgeUV();
  ScanEdge1OverZ();
  ScanEdgeNormals();

  VectorFromTo(&m_toLight, &m_plight->shineAtVertex, &m_plight->locationVertex);
  NormalizeVector(&m_toLight);

  for(i=0;i<m_nhlines;i++) {
    if(m_ystart >= m_screenheight) break;;
    if(m_ystart < 0) { m_ystart++;  continue; }
    if(m_prightEdgeX[i] >= m_screenwidth) { m_prightEdgeX[i] = m_screenwidth - 1; }
    if(m_pleftEdgeX[i] < 0) m_pleftEdgeX[i] = 0;
    width = m_prightEdgeX[i] - m_pleftEdgeX[i] + 1;
    InterpolateFloat(m_pleftEdgeU[i], m_prightEdgeU[i], m_hU, width);
    InterpolateFloat(m_pleftEdgeV[i], m_prightEdgeV[i], m_hV, width);
    InterpolateFloat(m_pleftEdge1OverZ[i], m_prightEdge1OverZ[i], m_h1OverZ, width);
    InterpolateFloat(m_pleftEdgeNX[i], m_prightEdgeNX[i], m_hNX, width);
    InterpolateFloat(m_pleftEdgeNY[i], m_prightEdgeNY[i], m_hNY, width);
    InterpolateFloat(m_pleftEdgeNZ[i], m_prightEdgeNZ[i], m_hNZ, width);

    for(j=0;j<width;j++) {
      if(m_h1OverZ[j] < m_zbuffer[m_ystart * m_screenwidth + m_pleftEdgeX[i] + j]) {
        m_zbuffer[m_ystart * m_screenwidth + m_pleftEdgeX[i] + j] = m_h1OverZ[j];

        currentNormal.x = m_hNX[j];
        currentNormal.y = m_hNY[j];
        currentNormal.z = m_hNZ[j];
        currentNormal.h = 1.0;
        NormalizeVector(&currentNormal);

        CalColorIntensity(&currentNormal);
        m_hRed[j] = m_red;
        m_hGreen[j] = m_green;
        m_hBlue[j] = m_blue;
        m_color = CalcColorFromMaterial(j);
        pixel(m_pleftEdgeX[i] + j, m_ystart, (int)m_hRed[j], (int)m_hGreen[j], (int)m_hBlue[j]);
      }
    }
    m_ystart++;
  }
}

void ShadeTriangle(triangle* ptri, light* plight, float* zbuffer, int shadeState) {
  m_ptri = ptri;
  m_plight = plight;
  m_zbuffer = zbuffer;
  m_shadeState = shadeState;
  m_pmat = ptri->pmat;

  m_screenwidth = GetScreenW();
  m_screenheight = GetScreenH();
  m_pcam = GetCam();

  m_tritype = ScanTriangleEdges(m_ptri, m_pleftEdgeX, m_prightEdgeX);
  m_ystart = ptri->pvlist[ptri->v0].iy;
  m_nhlines = ptri->pvlist[ptri->v1].iy - ptri->pvlist[ptri->v0].iy;

  if(m_shadeState & SHADE_VERTEX) ShadeVertex();
  else if(m_shadeState & SHADE_WIREFRAME) ShadeWireframe();
  else if(m_shadeState & SHADE_FLAT) {
    if(m_shadeState & SHADE_TEXTURE && m_pmat->ptexture) ShadeFlatT();
    else ShadeFlat();
  }
  else if(m_shadeState & SHADE_GOURAUD) ShadeGouraud();
  else if(m_shadeState & SHADE_PHONG) ShadePhong();
}

void SetAmbient(float red, float green, float blue)
{
  m_ambientRed = red;
  m_ambientGreen = green;
  m_ambientBlue = blue;
}

void Setppixel(void (*f)(int x, int y, int r, int g, int b)) {
  ppixel = f;
}

void line(int x0, int y0, int x1, int y1, int r, int g, int b) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  /* slope < 1 ? */
  if(dx > dy) line_(x0, y0, x1, y1, dx, dy, 0, r, g, b);
  else line_(y0, x0, y1, x1, dy, dx, 1, r, g, b);
}

static void line3dz_(int x0, int y0, int x1, int y1,
  int dx, int dy, int d, float *r, float *g, float *b, float *z, float *zb) {
  int i;
  int e = 2 * dy - dx;
  int sw = GetScreenW();
  int sh = GetScreenH();
  m_pcam = GetCam();
  for(i=0;i<=dx;i++) {
    if(d && x0<sh && x0>0 && y0<sw && y0>0) {
      if(z[i] < zb[x0 * sw + y0] && z[i] > 1.0f / m_pcam->clipNear) {
        pixel(y0, x0, (int)r[i], (int)g[i], (int)b[i]);
        zb[x0 * sw + y0] = z[i];
      }
    }
    else if(!d && y0<sh && y0>0 && x0<sw && x0>0) {
      if(z[i] < zb[y0 * sw + x0] && z[i] > 1.0f / m_pcam->clipNear) {
        pixel(x0, y0, (int)r[i], (int)g[i], (int)b[i]);
        zb[y0 * sw + x0] = z[i];
      }
    }
    x0 < x1 ? x0++ : x0--;
    if(e<0) {
      e = e + 2*dy;
    }
    else {
      y0<y1 ? y0++ : y0--;
      e = e + 2*dy - 2*dx;
    }
  }
}

void line3dz(vertex* v0, vertex* v1, float *zb) {
  float z0 = 1.0f / v0->z;
  float z1 = 1.0f / v1->z;
  int x0 = (int)(v0->x + 0.5);
  int y0 = (int)(v0->y + 0.5);
  int x1 = (int)(v1->x + 0.5);
  int y1 = (int)(v1->y + 0.5);
  int n = 2+(int)sqrtf(powf(v1->x-v0->x,2)+powf(v1->y-v0->y,2));
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  InterpolateFloat(z0, z1, m_h1OverZ, n);
  InterpolateFloat(v0->red, v1->red, m_hRed, n);
  InterpolateFloat(v0->green, v1->green, m_hGreen, n);
  InterpolateFloat(v0->blue, v1->blue, m_hBlue, n);
  if(dx>dy) line3dz_(x0, y0, x1, y1, dx, dy, 0, m_hRed, m_hGreen, m_hBlue, m_h1OverZ, zb);
  else line3dz_(y0, x0, y1, x1, dy, dx, 1, m_hRed, m_hGreen, m_hBlue, m_h1OverZ, zb);
}
