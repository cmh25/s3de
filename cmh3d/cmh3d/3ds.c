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

int Read3dsFile(char* fn, world* w) {
  unsigned int i, j, k;
  Lib3dsFile* fp;
  char path[1000];
  char fullpath[1000];

  w->objectCount = 0;
  w->vcount = 0;
  w->tcount = 0;

  fp = lib3ds_file_load(fn);
  if(fp == NULL) {
    fprintf(stderr, "\"%s\" is not a valid *.3ds filename\n", fn);
    return 0;
  }

  /* if we make it this far, fileName was valid. Therefore the path was
     valid and we need to know what the path is to append the texture filenames
     since the textures are required to be in the same folder as the *.3ds file */
  strcpy(path, fn);
  i=(int)strlen(path) - 1;
#ifdef _WIN32
  while(path[i] != '\\')path[i--] = '\0';
#else
  while (path[i] != '/')path[i--] = '\0';
#endif

  Lib3dsMesh* pmesh = fp->meshes;
  Lib3dsMaterial* pmat = fp->materials;

  w->matCount = 0;
  w->materials[w->matCount].name = "default";
  w->materials[w->matCount].r = 0.0;
  w->materials[w->matCount].g = 0.0;
  w->materials[w->matCount].b = 255.0;
  w->materials[w->matCount].ka = 1.0;
  w->materials[w->matCount].kd = 1.0;
  w->materials[w->matCount].ks = 0.5;
  w->materials[w->matCount].ns = 150.0;
  w->materials[w->matCount].ptexture = NULL;
  w->materials[w->matCount].uScale = 0.0;
  w->materials[w->matCount].vScale = 0.0;
  w->matCount++;
  for(i=1;pmat!=0;i++) {
    w->matCount++;
    w->materials[i].name = pmat->name;
    w->materials[i].r = pmat->diffuse[0] * 255.0f;
    w->materials[i].g = pmat->diffuse[1] * 255.0f;
    w->materials[i].b = pmat->diffuse[2] * 255.0f;
    w->materials[i].ka = 1.0;
    w->materials[i].kd = 1.0;
    w->materials[i].ks = 0.5;
    w->materials[i].ns = 150.0;
    w->materials[i].ptexture = 0;
    if(pmat->texture1_map.name && pmat->texture1_map.name[0]) {
      strcpy(fullpath, path);
      loadTextureFromBmpFile(&w->materials[i], strcat(fullpath, pmat->texture1_map.name));
    }
    w->materials[i].uScale = pmat->texture1_map.scale[0];
    w->materials[i].vScale = pmat->texture1_map.scale[1];

    pmat = pmat->next;
  }

  for(i=0;pmesh!=0;i++) {
    w->objectCount++;
    w->vcount += pmesh->points;
    w->tcount += pmesh->faces;
    w->objectList[i].vcount = pmesh->points;
    w->objectList[i].tcount = pmesh->faces;
    w->objectList[i].vlist = calloc(w->objectList[i].vcount, sizeof(vertex));
    if(w->objectList[i].vlist == NULL) {
      fprintf(stderr, "malloc failed for vlist in Read3dsFile()!\n");
      exit(1);
    }
    w->objectList[i].tlist = calloc(w->objectList[i].tcount, sizeof(triangle));
    if(w->objectList[i].tlist == NULL) {
      fprintf(stderr, "malloc failed for tlist in Read3dsFile()!\n");
      exit(1);
    }

    for(j=0;j<pmesh->points;j++) {
      w->objectList[i].vlist[j].x = pmesh->pointL[j].pos[0];
      w->objectList[i].vlist[j].y = pmesh->pointL[j].pos[2];
      w->objectList[i].vlist[j].z = -pmesh->pointL[j].pos[1];
      w->objectList[i].vlist[j].h = 1.0;
    }

    for(j=0;j<pmesh->faces;j++) {
      w->objectList[i].tlist[j].v0 = pmesh->faceL[j].points[0];
      w->objectList[i].tlist[j].v1 = pmesh->faceL[j].points[1];
      w->objectList[i].tlist[j].v2 = pmesh->faceL[j].points[2];
      w->objectList[i].tlist[j].pmat = &w->materials[0];
      for(k=0;k<w->matCount;k++) {
        if(!strcmp(pmesh->faceL[j].material, w->materials[k].name)) {
          w->objectList[i].tlist[j].pmat = &w->materials[k];
        }
      }
    }

    for(j=0;j<pmesh->texels;j++) {
      w->objectList[i].vlist[j].u = pmesh->texelL[j][0];
      w->objectList[i].vlist[j].v = pmesh->texelL[j][1];
    }

    pmesh = pmesh->next;
  }

  lib3ds_file_free(fp);

  return 1;
}
