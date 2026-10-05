#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "3ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <lib3ds/file.h>
#include <lib3ds/material.h>
#include <lib3ds/mesh.h>
#include "object.h"
#include "vertex.h"
#include "triangle.h"
#include "material.h"
#include "engineerror.h"

static char* copyString(const char* s) {
  size_t n = strlen(s) + 1;
  char* copy = (char*)malloc(n);
  if(copy) memcpy(copy, s, n);
  return copy;
}

static void setMaterialDefaults(material* m) {
  m->name = NULL;
  m->r = 0.0f; m->g = 0.0f; m->b = 255.0f;
  m->ka = 1.0f; m->kd = 1.0f; m->ks = 0.5f; m->ns = 150.0f;
  m->ptexture = NULL;
  m->textureWidth = m->textureHeight = 0;
  m->uScale = m->vScale = 1.0f;
}

/* fills w's objects and materials from a .3ds file. returns 0 on failure with
   the reason in GetLastEngineError(); the caller frees whatever was added.
   a texture that cannot be loaded is not a failure: the material stays
   untextured and the message is left for the host to show as a warning */
int Read3dsFile(char* fn, world* w) {
  const unsigned int maxObjects = sizeof(w->objectList) / sizeof(w->objectList[0]);
  const unsigned int maxMaterials = sizeof(w->materials) / sizeof(w->materials[0]);
  unsigned int j, k, n;
  size_t len;
  Lib3dsFile* fp;
  Lib3dsMesh* pmesh;
  Lib3dsMaterial* pmat;
  char path[1000];
  char fullpath[1000];
  material* m;
  object* o;

  w->objectCount = 0;
  w->matCount = 0;
  w->vcount = 0;
  w->tcount = 0;

  if(strlen(fn) >= sizeof(path)) { SetEngineError("file name is too long: %s", fn); return 0; }

  fp = lib3ds_file_load(fn);
  if(fp == NULL) { SetEngineError("\"%s\" could not be opened as a .3ds file", fn); return 0; }

  /* textures are looked up next to the model, so keep the directory part of
  the file name: everything up to and including the last separator, if any */
  strcpy(path, fn);
  for(len = strlen(path); len > 0 && path[len-1] != '\\' && path[len-1] != '/'; len--) ;
  path[len] = '\0';

  /* material 0 is used by faces that name no material */
  m = &w->materials[0];
  setMaterialDefaults(m);
  m->name = copyString("default");
  if(!m->name) { lib3ds_file_free(fp); SetEngineError("out of memory loading \"%s\"", fn); return 0; }
  w->matCount = 1;

  for(pmat = fp->materials; pmat != NULL; pmat = pmat->next) {
    if(w->matCount >= maxMaterials) {
      lib3ds_file_free(fp);
      SetEngineError("\"%s\" has more than %u materials", fn, maxMaterials - 1);
      return 0;
    }
    m = &w->materials[w->matCount];
    setMaterialDefaults(m);
    m->name = copyString(pmat->name); /* lib3ds frees its own copy below */
    if(!m->name) { lib3ds_file_free(fp); SetEngineError("out of memory loading \"%s\"", fn); return 0; }
    m->r = pmat->diffuse[0] * 255.0f;
    m->g = pmat->diffuse[1] * 255.0f;
    m->b = pmat->diffuse[2] * 255.0f;
    m->uScale = pmat->texture1_map.scale[0];
    m->vScale = pmat->texture1_map.scale[1];
    w->matCount++;
    if(pmat->texture1_map.name[0]) {
      if(snprintf(fullpath, sizeof(fullpath), "%s%s", path, pmat->texture1_map.name) >= (int)sizeof(fullpath))
        SetEngineError("texture path is too long: %s%s", path, pmat->texture1_map.name);
      else
        loadTextureFromBmpFile(m, fullpath);
    }
  }

  for(pmesh = fp->meshes; pmesh != NULL; pmesh = pmesh->next) {
    if(pmesh->points == 0 || pmesh->faces == 0) continue; /* nothing to draw */
    if(w->objectCount >= maxObjects) {
      lib3ds_file_free(fp);
      SetEngineError("\"%s\" has more than %u meshes", fn, maxObjects);
      return 0;
    }
    o = &w->objectList[w->objectCount];
    o->vcount = pmesh->points;
    o->tcount = pmesh->faces;
    o->vlist = (vertex*)calloc(o->vcount, sizeof(vertex));
    o->tlist = (triangle*)calloc(o->tcount, sizeof(triangle));
    if(!o->vlist || !o->tlist) {
      free(o->vlist); free(o->tlist);
      o->vlist = NULL; o->tlist = NULL;
      lib3ds_file_free(fp);
      SetEngineError("out of memory for mesh \"%s\" in \"%s\"", pmesh->name, fn);
      return 0;
    }
    w->objectCount++;
    w->vcount += o->vcount;
    w->tcount += o->tcount;

    for(j=0;j<pmesh->points;j++) {
      /* 3ds files are z-up, the engine is y-up */
      o->vlist[j].x = pmesh->pointL[j].pos[0];
      o->vlist[j].y = pmesh->pointL[j].pos[2];
      o->vlist[j].z = -pmesh->pointL[j].pos[1];
      o->vlist[j].h = 1.0f;
    }

    for(j=0;j<pmesh->faces;j++) {
      triangle* t = &o->tlist[j];
      t->v0 = pmesh->faceL[j].points[0];
      t->v1 = pmesh->faceL[j].points[1];
      t->v2 = pmesh->faceL[j].points[2];
      if(t->v0 >= o->vcount || t->v1 >= o->vcount || t->v2 >= o->vcount) {
        lib3ds_file_free(fp);
        SetEngineError("mesh \"%s\" in \"%s\" has a face that refers to a missing vertex", pmesh->name, fn);
        return 0;
      }
      t->pmat = &w->materials[0];
      for(k=0;k<w->matCount;k++) {
        if(!strcmp(pmesh->faceL[j].material, w->materials[k].name)) t->pmat = &w->materials[k];
      }
    }

    /* texture coordinates, one per point when the mesh has them */
    n = pmesh->texels < pmesh->points ? pmesh->texels : pmesh->points;
    for(j=0;j<n;j++) {
      o->vlist[j].u = pmesh->texelL[j][0];
      o->vlist[j].v = pmesh->texelL[j][1];
    }
  }

  lib3ds_file_free(fp);
  return 1;
}
