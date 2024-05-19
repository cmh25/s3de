#include "material.h"
#include <stdio.h>
#include <string.h>
#include "bitmap.h"

void loadTextureFromBmpFile(material* pmat, char* fileName) {
  if(*fileName != 0) 
    pmat->ptexture = readBmp(fileName, &pmat->textureWidth, &pmat->textureHeight);
  else pmat->ptexture = NULL;
}
