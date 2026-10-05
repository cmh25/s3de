/* tests.c - regression tests for the cmh3d renderer.

   Builds a few scenes by hand, renders them through the pixel callback into a
   buffer and checks pixels. Each test guards against a bug that has been fixed.

   usage: tests        prints one line per test and exits with the number of failures */

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
#include "cmh3d/clip.h"
#include "cmh3d/bitmap.h"
#include "cmh3d/material.h"

#define WIDTH      640       /* the engine's default screen size */
#define HEIGHT     480
#define TEX        256
#define BACKGROUND 0x66

static unsigned char fb[WIDTH * HEIGHT * 3];
static int failures;

/* ---- framebuffer ------------------------------------------------------- */

static void putPixel(int x, int y, int r, int g, int b) {
  unsigned char* p;
  if(x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) return;
  p = &fb[(y * WIDTH + x) * 3];   /* kept in engine coordinates: y grows upward */
  p[0] = (unsigned char)r; p[1] = (unsigned char)g; p[2] = (unsigned char)b;
}

static const unsigned char* pixelAt(int x, int y) { return &fb[(y * WIDTH + x) * 3]; }

static int isNear(const unsigned char* p, int r, int g, int b) {
  return abs(p[0] - r) <= 2 && abs(p[1] - g) <= 2 && abs(p[2] - b) <= 2;
}

static int isBackground(const unsigned char* p) {
  return p[0] == BACKGROUND && p[1] == BACKGROUND && p[2] == BACKGROUND;
}

static void clearFrame(void) { memset(fb, BACKGROUND, sizeof(fb)); }

static const char* colourName(const unsigned char* p) {
  static char buf[32];
  sprintf(buf, "(%d,%d,%d)", p[0], p[1], p[2]);
  return buf;
}

/* first and last drawn pixel on a row or column; 0 if nothing is drawn there */
static int rowExtent(int y, int* x0, int* x1) {
  int x;
  for(x = 0; x < WIDTH && isBackground(pixelAt(x, y)); x++) ;
  if(x == WIDTH) return 0;
  *x0 = x;
  for(x = WIDTH - 1; isBackground(pixelAt(x, y)); x--) ;
  *x1 = x;
  return 1;
}

static int columnExtent(int x, int* y0, int* y1) {
  int y;
  for(y = 0; y < HEIGHT && isBackground(pixelAt(x, y)); y++) ;
  if(y == HEIGHT) return 0;
  *y0 = y;
  for(y = HEIGHT - 1; isBackground(pixelAt(x, y)); y--) ;
  *y1 = y;
  return 1;
}

static long countDrawn(void) {
  long n = 0, i;
  for(i = 0; i < (long)WIDTH * HEIGHT; i++) if(!isBackground(&fb[i * 3])) n++;
  return n;
}

/* ---- reporting --------------------------------------------------------- */

static void report(const char* name, int passed, const char* detail) {
  printf("%s  %-28s %s\n", passed ? "PASS" : "FAIL", name, detail ? detail : "");
  if(!passed) failures++;
}

static float frand(float lo, float hi) { return lo + (hi - lo) * (float)rand() / (float)RAND_MAX; }

/* ---- scene helpers ----------------------------------------------------- */

/* a 256x256 texture laid out like readBmp() delivers it, top row first:
   the top four rows red, the bottom four green, the left four columns blue,
   the right four yellow, grey inside. rows win over columns in the corners */
static unsigned char* makeEdgeTexture(void) {
  unsigned char* t = (unsigned char*)malloc(TEX * TEX * 3);
  int x, y;
  if(!t) return NULL;
  for(y = 0; y < TEX; y++) for(x = 0; x < TEX; x++) {
    unsigned char* p = &t[(y * TEX + x) * 3];
    if(y < 4)             { p[0] = 255; p[1] = 0;   p[2] = 0;   }
    else if(y >= TEX - 4) { p[0] = 0;   p[1] = 255; p[2] = 0;   }
    else if(x < 4)        { p[0] = 0;   p[1] = 0;   p[2] = 255; }
    else if(x >= TEX - 4) { p[0] = 255; p[1] = 255; p[2] = 0;   }
    else                  { p[0] = p[1] = p[2] = 200; }
  }
  return t;
}

/* the usual layout: (0,0) at the bottom-left corner, u to the right, v upward */
static const float UV_NORMAL[4][2]  = { {0,0}, {1,0}, {1,1}, {0,1} };
/* a quarter turn: v=0 runs along the quad's left edge, u=0 along its top */
static const float UV_ROTATED[4][2] = { {1,0}, {1,1}, {0,1}, {0,0} };

/* a 40x30 quad centred on (0,0,-45) facing the camera at the origin, tilted
   about y then x. material 0 is grey 200 lit by ambient only, so a texture's
   colours come through unchanged. the texture becomes the world's to free */
static world* makeQuad(const float uv[4][2], float tiltY, float tiltX, unsigned char* texture) {
  static const float xyz[4][3] = { {-20,-15,0}, {20,-15,0}, {20,15,0}, {-20,15,0} };
  world* w = InitializeWorld(NULL);
  material* m;
  object* o;
  vertex origin;
  int i;

  if(!w) { free(texture); return NULL; }
  m = &w->materials[0];
  m->name = NULL;
  m->r = m->g = m->b = 200.0f;
  m->ka = 1.0f; m->kd = 0.0f; m->ks = 0.0f; m->ns = 1.0f;
  m->uScale = m->vScale = 1.0f;
  m->ptexture = texture;
  m->textureWidth = m->textureHeight = texture ? TEX : 0;
  w->matCount = 1;

  o = &w->objectList[0];
  o->vlist = (vertex*)calloc(4, sizeof(vertex));
  o->tlist = (triangle*)calloc(2, sizeof(triangle));
  if(!o->vlist || !o->tlist) { free(o->vlist); free(o->tlist); FreeWorld(w); return NULL; }
  o->vcount = 4; o->tcount = 2;
  w->objectCount = 1;

  memset(&origin, 0, sizeof origin);
  origin.h = 1.0f;
  for(i = 0; i < 4; i++) {
    vertex* v = &o->vlist[i];
    v->x = xyz[i][0]; v->y = xyz[i][1]; v->z = xyz[i][2]; v->h = 1.0f;
    v->u = uv[i][0]; v->v = uv[i][1];
    Rotatey(v, &origin, tiltY);
    Rotatex(v, &origin, tiltX);
    v->z -= 45.0f;
  }
  o->tlist[0].v0 = 0; o->tlist[0].v1 = 1; o->tlist[0].v2 = 2; o->tlist[0].pmat = m;
  o->tlist[1].v0 = 0; o->tlist[1].v1 = 2; o->tlist[1].v2 = 3; o->tlist[1].pmat = m;

  SetAmbient(1.0f, 1.0f, 1.0f);
  return w;
}

static int render(world* w, int shade) {
  clearFrame();
  SetShadeState(shade);
  return DrawScene(w);
}

/* ---- tests ------------------------------------------------------------- */

/* readBmp() must read in binary mode. in text mode the Windows CRT turned
   CR LF into LF and treated 0x1A as the end of the file */
static void testBmpBinaryMode(void) {
  static const unsigned char expected[3][5][3] = {   /* rows top-down, r,g,b */
    { {0x0A,0x0D,0x1A}, {0x0D,0x0A,0x1A}, {255,0,0}, {0,255,0}, {0,0,255} },
    { {0x1A,0x1A,0x1A}, {1,2,3}, {0x0A,0x0D,0x0A}, {200,100,50}, {0x0D,0x0A,0x0D} },
    { {0,0,0}, {255,255,255}, {0x1A,0,0x1A}, {7,8,9}, {0x0A,0x1A,0x0D} }
  };
  const char* fileName = "s3de_test_texture.bmp";
  unsigned char header[54];
  unsigned char* read;
  unsigned int w = 0, h = 0;
  FILE* fp;
  int x, y, ok = 1;
  char detail[160] = "";

  /* a 5x3 24-bit bitmap: 15 byte rows padded to 16, stored bottom row first as b,g,r */
  memset(header, 0, sizeof header);
  header[0] = 'B'; header[1] = 'M';
  header[2] = 54 + 3 * 16;   /* file size */
  header[10] = 54;           /* pixel data offset */
  header[14] = 40;           /* info header size */
  header[18] = 5;            /* width */
  header[22] = 3;            /* height */
  header[26] = 1;            /* planes */
  header[28] = 24;           /* bits per pixel */
  header[34] = 3 * 16;       /* image size */
  fp = fopen(fileName, "wb");
  if(!fp) { report("bmp_binary_mode", 0, "cannot write a temporary file here"); return; }
  fwrite(header, 1, sizeof header, fp);
  for(y = 2; y >= 0; y--) {
    for(x = 0; x < 5; x++) { fputc(expected[y][x][2], fp); fputc(expected[y][x][1], fp); fputc(expected[y][x][0], fp); }
    fputc(0, fp);
  }
  fclose(fp);

  read = readBmp((char*)fileName, &w, &h);
  remove(fileName);
  if(!read) { report("bmp_binary_mode", 0, GetLastEngineError()); return; }
  if(w != 5 || h != 3) { sprintf(detail, "read as %ux%u, expected 5x3", w, h); ok = 0; }
  for(y = 0; ok && y < 3; y++) for(x = 0; ok && x < 5; x++) {
    const unsigned char* p = &read[(y * 5 + x) * 3];
    if(memcmp(p, expected[y][x], 3)) {
      sprintf(detail, "pixel (%d,%d) read as %s, expected (%d,%d,%d)", x, y, colourName(p),
        expected[y][x][0], expected[y][x][1], expected[y][x][2]);
      ok = 0;
    }
  }
  free(read);
  report("bmp_binary_mode", ok, detail);
}

/* bad input must come back as an error message, not a dead process */
static void testBadInputs(void) {
  const char* fileName = "s3de_test_not_a_bitmap.bmp";
  world* w;
  FILE* fp;
  unsigned char* t;
  unsigned int tw, th;
  int ok = 1;
  char detail[160] = "";

  w = InitializeWorld("s3de_test_missing_model.3ds");
  if(w) { ok = 0; strcpy(detail, "a missing model file loaded"); FreeWorld(w); }
  else if(!GetLastEngineError()[0]) { ok = 0; strcpy(detail, "a missing model file gave no error message"); }

  fp = fopen(fileName, "wb");
  if(fp) { fputs("this is not a bitmap", fp); fclose(fp); }
  t = readBmp((char*)fileName, &tw, &th);
  remove(fileName);
  if(ok && t) { ok = 0; strcpy(detail, "a text file was accepted as a bitmap"); free(t); }
  else if(ok && !GetLastEngineError()[0]) { ok = 0; strcpy(detail, "a bad bitmap gave no error message"); }
  report("bad_inputs", ok, detail);
}

/* a triangle clipped against the six view planes can come back with up to
   MAX_CLIPPED_VERTICES vertices. Clip() in world.c used to reserve seven, so
   an eight or nine vertex result overflowed the stack */
static void testClipVertexCount(void) {
  camera cam;
  triangle t;
  vertex v[3], out[MAX_CLIPPED_VERTICES + 4], sentinel;
  int iter, k, n, maxn = 0, ok = 1;
  char detail[160] = "";

  memset(&cam, 0, sizeof cam);
  cam.distanceToViewPlane = 30.0f; cam.viewPlaneWidth = 40.0f; cam.viewPlaneHeight = 30.0f;
  cam.clipNear = -1.0f; cam.clipFar = -20000.0f; cam.clipUnit = 1.0f;
  SetViewClipPlanes(&cam);
  memset(&t, 0, sizeof t);
  t.pvlist = v; t.v0 = 0; t.v1 = 1; t.v2 = 2;
  memset(&sentinel, 0xAB, sizeof sentinel);

  srand(12345);
  for(iter = 0; iter < 200000 && ok; iter++) {
    /* a point just inside the frustum, a random plane through it, and a
    huge triangle in that plane around it */
    float pz = -frand(1.0f, 4.0f), px = frand(-0.6f, 0.6f) * -pz, py = frand(-0.45f, 0.45f) * -pz;
    float nx = frand(-1, 1), ny = frand(-1, 1), nz = frand(-1, 1), len = sqrtf(nx*nx + ny*ny + nz*nz);
    float ax, ay, az, bx, by, bz, radius = frand(1e3f, 1e5f), a0 = frand(0, 6.283f);
    if(len < 1e-3f) continue;
    nx /= len; ny /= len; nz /= len;
    if(fabsf(nx) < 0.9f) { ax = 0; ay = -nz; az = ny; } else { ax = -nz; ay = 0; az = nx; }
    len = sqrtf(ax*ax + ay*ay + az*az); ax /= len; ay /= len; az /= len;
    bx = ny*az - nz*ay; by = nz*ax - nx*az; bz = nx*ay - ny*ax;
    memset(v, 0, sizeof v);
    for(k = 0; k < 3; k++) {
      float a = a0 + k * 2.0944f + frand(-0.5f, 0.5f), c = cosf(a), s = sinf(a);
      v[k].x = px + radius * (c*ax + s*bx);
      v[k].y = py + radius * (c*ay + s*by);
      v[k].z = pz + radius * (c*az + s*bz);
      v[k].h = 1.0f;
    }
    for(k = MAX_CLIPPED_VERTICES; k < MAX_CLIPPED_VERTICES + 4; k++) memcpy(&out[k], &sentinel, sizeof sentinel);
    n = ClipTriangleToView(&t, &cam, out);
    if(n > maxn) maxn = n;
    if(n < 0 || n > MAX_CLIPPED_VERTICES) { sprintf(detail, "returned %d vertices, the limit is %d", n, MAX_CLIPPED_VERTICES); ok = 0; }
    for(k = MAX_CLIPPED_VERTICES; ok && k < MAX_CLIPPED_VERTICES + 4; k++)
      if(memcmp(&out[k], &sentinel, sizeof sentinel)) { sprintf(detail, "wrote past vertex %d of the output", MAX_CLIPPED_VERTICES); ok = 0; }
  }
  if(ok) sprintf(detail, "largest result %d vertices%s", maxn, maxn > 7 ? "" : " (never reached the old overflow case)");
  report("clip_vertex_count", ok, detail);
}

/* a world must render right after InitializeWorld(), before the host has
   called SetScreenW()/SetScreenH(). the clip planes used to be left unset */
static void testRenderWithoutScreenSize(void) {
  world* w = makeQuad(UV_NORMAL, 0, 0, NULL);
  long n;
  char detail[80] = "";
  if(!w) { report("render_without_screen_size", 0, "cannot build the scene"); return; }
  if(!render(w, SHADE_FLAT | SHADE_AMBIENT | SHADE_ZBUFFER)) { report("render_without_screen_size", 0, GetLastEngineError()); FreeWorld(w); return; }
  n = countDrawn();
  sprintf(detail, "%ld pixels drawn", n);
  FreeWorld(w);
  report("render_without_screen_size", n > 1000, detail);
}

/* the four texture edges must land on the four quad edges, the right way
   round, with nearest and bilinear sampling */
static void testTextureEdges(int bilinear) {
  const char* name = bilinear ? "texture_edges_bilinear" : "texture_edges";
  world* w = makeQuad(UV_NORMAL, 0, 0, makeEdgeTexture());
  int x0, x1, y0, y1, ok = 1;
  char detail[160] = "";
  if(!w) { report(name, 0, "cannot build the scene"); return; }
  if(!render(w, SHADE_FLAT | SHADE_AMBIENT | SHADE_ZBUFFER | SHADE_TEXTURE | (bilinear ? SHADE_BILINEAR : 0))) {
    report(name, 0, GetLastEngineError()); FreeWorld(w); return;
  }
  if(!rowExtent(HEIGHT / 2, &x0, &x1) || !columnExtent(WIDTH / 2, &y0, &y1)) { ok = 0; strcpy(detail, "nothing drawn"); }
  else if(!isNear(pixelAt(x0, HEIGHT / 2), 0, 0, 255))   { ok = 0; sprintf(detail, "left edge is %s, expected blue", colourName(pixelAt(x0, HEIGHT / 2))); }
  else if(!isNear(pixelAt(x1, HEIGHT / 2), 255, 255, 0)) { ok = 0; sprintf(detail, "right edge is %s, expected yellow", colourName(pixelAt(x1, HEIGHT / 2))); }
  else if(!isNear(pixelAt(WIDTH / 2, y1), 255, 0, 0))    { ok = 0; sprintf(detail, "top edge is %s, expected red", colourName(pixelAt(WIDTH / 2, y1))); }
  else if(!isNear(pixelAt(WIDTH / 2, y0), 0, 255, 0))    { ok = 0; sprintf(detail, "bottom edge is %s, expected green", colourName(pixelAt(WIDTH / 2, y0))); }
  FreeWorld(w);
  report(name, ok, detail);
}

/* v=0 along a quad edge must sample the bottom texture row. the mapper used
   to flip v with -v and then wrap, which sent exactly 0 to the top row: a one
   pixel seam along every edge where v is 0 */
static void testTextureSeamV0(int bilinear) {
  const char* name = bilinear ? "texture_seam_v0_bilinear" : "texture_seam_v0";
  world* w = makeQuad(UV_ROTATED, 0, 0, makeEdgeTexture());
  int x0, x1, y0, y1, ok = 1;
  char detail[160] = "";
  if(!w) { report(name, 0, "cannot build the scene"); return; }
  if(!render(w, SHADE_FLAT | SHADE_AMBIENT | SHADE_ZBUFFER | SHADE_TEXTURE | (bilinear ? SHADE_BILINEAR : 0))) {
    report(name, 0, GetLastEngineError()); FreeWorld(w); return;
  }
  if(!rowExtent(HEIGHT / 2, &x0, &x1) || !columnExtent(WIDTH / 2, &y0, &y1)) { ok = 0; strcpy(detail, "nothing drawn"); }
  else if(!isNear(pixelAt(x0, HEIGHT / 2), 0, 255, 0))   { ok = 0; sprintf(detail, "v=0 edge is %s, expected green", colourName(pixelAt(x0, HEIGHT / 2))); }
  else if(!isNear(pixelAt(x1, HEIGHT / 2), 255, 0, 0))   { ok = 0; sprintf(detail, "v=1 edge is %s, expected red", colourName(pixelAt(x1, HEIGHT / 2))); }
  else if(!isNear(pixelAt(WIDTH / 2, y0), 255, 255, 0))  { ok = 0; sprintf(detail, "u=1 edge is %s, expected yellow", colourName(pixelAt(WIDTH / 2, y0))); }
  else if(!isNear(pixelAt(WIDTH / 2, y1), 0, 0, 255))    { ok = 0; sprintf(detail, "u=0 edge is %s, expected blue", colourName(pixelAt(WIDTH / 2, y1))); }
  FreeWorld(w);
  report(name, ok, detail);
}

/* a frame rendered with specular on must not change later frames rendered
   without it. the specular term used to survive in a static */
static void testSpecularNoLeak(void) {
  static unsigned char first[sizeof fb];
  world* w = makeQuad(UV_NORMAL, 35.26f, -45.0f, makeEdgeTexture());   /* faces the default light */
  material* m;
  long i, differing = 0;
  char detail[80] = "";
  if(!w) { report("specular_no_leak", 0, "cannot build the scene"); return; }
  m = &w->materials[0];
  m->ka = 1.0f; m->kd = 1.0f; m->ks = 0.5f; m->ns = 150.0f;
  SetAmbient(0.3f, 0.3f, 0.3f);
  if(!render(w, SHADE_FLAT | SHADE_AMBIENT | SHADE_DIFFUSE | SHADE_ZBUFFER | SHADE_TEXTURE)) { report("specular_no_leak", 0, GetLastEngineError()); FreeWorld(w); return; }
  memcpy(first, fb, sizeof fb);
  render(w, SHADE_PHONG | SHADE_SPECULAR | SHADE_AMBIENT | SHADE_DIFFUSE | SHADE_ZBUFFER | SHADE_TEXTURE);
  render(w, SHADE_FLAT | SHADE_AMBIENT | SHADE_DIFFUSE | SHADE_ZBUFFER | SHADE_TEXTURE);
  for(i = 0; i < (long)WIDTH * HEIGHT; i++) if(memcmp(&fb[i * 3], &first[i * 3], 3)) differing++;
  sprintf(detail, "%ld pixels changed after a phong frame", differing);
  FreeWorld(w);
  report("specular_no_leak", differing == 0, detail);
}

/* the light lives in world space. a quad lit head-on by a light in front of
   it must stay fully lit when the camera moves 60 degrees to the side. it
   used to darken, because the light followed the camera */
static void testLightInWorldSpace(void) {
  world* w = makeQuad(UV_NORMAL, 0, 0, NULL);
  camera* cam;
  matrix4x4 m;
  const unsigned char* p;
  int ok;
  char detail[120] = "";
  if(!w) { report("light_in_world_space", 0, "cannot build the scene"); return; }
  w->materials[0].ka = 0.0f;
  w->materials[0].kd = 1.0f;
  w->lightList[0].locationVertex.x = 0.0f;
  w->lightList[0].locationVertex.y = 0.0f;
  w->lightList[0].locationVertex.z = 100.0f;   /* shining down -z at the quad's +z face */

  /* orbit the camera 60 degrees around the quad's centre, as the hosts do */
  cam = &w->cameraList[w->currentCamIndex];
  Reset(cam);
  GetTranslateMatrix(m, 0.0f, 0.0f, 60.0f);  ApplyMatrix(cam, m);
  GetRotateMatrix(m, 0.0f, 60.0f, 0.0f);     ApplyMatrix(cam, m);
  GetTranslateMatrix(m, 0.0f, 0.0f, -45.0f); ApplyMatrix(cam, m);

  if(!render(w, SHADE_FLAT | SHADE_DIFFUSE | SHADE_ZBUFFER)) { report("light_in_world_space", 0, GetLastEngineError()); FreeWorld(w); return; }
  p = pixelAt(WIDTH / 2, HEIGHT / 2);
  ok = p[0] >= 196 && p[0] <= 200 && p[1] == p[0] && p[2] == p[0];
  if(!ok) sprintf(detail, "centre pixel is %s, expected about (200,200,200)", colourName(p));
  FreeWorld(w);
  report("light_in_world_space", ok, detail);
}

/* loading and freeing worlds repeatedly must not grow the clip buffers
   without bound. their capacity used to double on every reload */
static void testReloadCycles(void) {
  int cycle, ok = 1;
  char detail[160] = "";
  for(cycle = 0; cycle < 40 && ok; cycle++) {
    world* w = makeQuad(UV_NORMAL, 0, 0, NULL);
    if(!w || !render(w, SHADE_FLAT | SHADE_AMBIENT | SHADE_ZBUFFER)) { sprintf(detail, "cycle %d: %s", cycle, GetLastEngineError()); ok = 0; }
    FreeWorld(w);
  }
  if(ok) sprintf(detail, "%d cycles", cycle);
  report("reload_cycles", ok, detail);
}

int main(void) {
  Setppixel(putPixel);
  testBmpBinaryMode();
  testBadInputs();
  testClipVertexCount();
  testRenderWithoutScreenSize();
  testTextureEdges(0);
  testTextureEdges(1);
  testTextureSeamV0(0);
  testTextureSeamV0(1);
  testSpecularNoLeak();
  testLightInWorldSpace();
  testReloadCycles();
  printf("%d failure%s\n", failures, failures == 1 ? "" : "s");
  return failures;
}
