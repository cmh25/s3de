#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "bitmap.h"
#include "engineerror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXTURE_SIDE 16384

static unsigned int readU32(const unsigned char* p) {
  return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24);
}

static unsigned int readU16(const unsigned char* p) {
  return p[0] | (p[1] << 8);
}

/* reads an uncompressed 24-bit .bmp into an r,g,b byte array, top row first.
   returns NULL with the reason in GetLastEngineError() on failure */
unsigned char* readBmp(char* fileName, unsigned int* biWidth, unsigned int* biHeight) {
  unsigned char header[54];
  unsigned char* pixels;
  unsigned char* row;
  FILE* fp;
  unsigned int offset, width, rows, rowBytes, x, y;
  int height;

  /* "rb": in text mode the Windows CRT turns CR LF into LF and treats 0x1A as
  end of file, which corrupts or truncates any texture containing those bytes */
  if((fp = fopen(fileName, "rb")) == NULL) {
    SetEngineError("cannot open texture \"%s\"", fileName);
    return NULL;
  }
  if(fread(header, 1, sizeof(header), fp) != sizeof(header) || header[0] != 'B' || header[1] != 'M') {
    SetEngineError("\"%s\" is not a .bmp file", fileName);
    fclose(fp);
    return NULL;
  }
  offset = readU32(header + 10);
  width  = readU32(header + 18);
  height = (int)readU32(header + 22); /* negative means the rows are stored top-down */
  if(readU16(header + 28) != 24 || readU32(header + 30) != 0) {
    SetEngineError("\"%s\" is not an uncompressed 24-bit .bmp file", fileName);
    fclose(fp);
    return NULL;
  }
  if(width == 0 || width > MAX_TEXTURE_SIDE || height == 0 || height > MAX_TEXTURE_SIDE || height < -MAX_TEXTURE_SIDE) {
    SetEngineError("\"%s\" has an unsupported size (%u x %d)", fileName, width, height);
    fclose(fp);
    return NULL;
  }
  rows = (unsigned int)abs(height);
  rowBytes = (width * 3 + 3) & ~3u; /* rows are padded to four bytes in the file */

  pixels = (unsigned char*)malloc((size_t)width * rows * 3);
  row = (unsigned char*)malloc(rowBytes);
  if(!pixels || !row || fseek(fp, (long)offset, SEEK_SET) != 0) {
    SetEngineError(pixels && row ? "\"%s\" is truncated" : "out of memory for texture \"%s\"", fileName);
    free(pixels); free(row); fclose(fp);
    return NULL;
  }

  /* the file stores b,g,r and, when the height is positive, the bottom row first */
  for(y = 0; y < rows; y++) {
    unsigned int target = height > 0 ? rows - 1 - y : y;
    unsigned char* dst = pixels + (size_t)target * width * 3;
    if(fread(row, 1, rowBytes, fp) != rowBytes) {
      SetEngineError("\"%s\" is truncated", fileName);
      free(pixels); free(row); fclose(fp);
      return NULL;
    }
    for(x = 0; x < width; x++) {
      dst[x*3]     = row[x*3 + 2];
      dst[x*3 + 1] = row[x*3 + 1];
      dst[x*3 + 2] = row[x*3];
    }
  }

  free(row);
  fclose(fp);
  *biWidth = width;
  *biHeight = rows;
  return pixels;
}
