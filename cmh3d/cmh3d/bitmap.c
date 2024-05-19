#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>

unsigned char* readBmp(char* fileName, unsigned int* biWidth, unsigned int* biHeight) {
  unsigned int bfOffBits;
  unsigned char biBitCount,*pbuffer;

  FILE* fp;
  unsigned int i,j;
  int k,pad;

  if((fp = fopen(fileName, "r")) == NULL) {
    fprintf(stderr, "Unable to open bitmap file: \"%s\"\n", fileName);
    return NULL;
  }

  fseek(fp, 10, SEEK_SET);
  fread(&bfOffBits, 4, 1, fp);

  fseek(fp, 18, SEEK_SET);
  fread(biWidth, 4, 1, fp);

  fseek(fp, 22, SEEK_SET);
  fread(biHeight, 4, 1, fp);

  fseek(fp, 28, SEEK_SET);
  fread(&biBitCount, 1, 1, fp);

  /* make sure its 24-bit */
  if(biBitCount != 24) {
    fprintf(stderr, "%s is not a 24-bit bitmap file!\n", fileName);
    fclose(fp);
    exit(1);
  }

  /* find size and allocate pbuffer */
  if((pbuffer = malloc(*biWidth * *biHeight * sizeof(int) * 3)) == NULL) {
    fprintf(stderr, "malloc failed for pbuffer in readBmp()!\n");
    fclose(fp);
    exit(1);
  }

  /* finally, time to read
  in the bitmap file, the order is blue green red in uints
  we want pbuffer to have red green blue in floats (0.0 - 1.0)
  the rows are ordered bottom to top in the file and each
  row is zero-padded to a 4-byte boundary */
  pad = 4 - (*biWidth*3) % 4;
  if(pad == 4)pad = 0;

  fseek(fp, bfOffBits, SEEK_SET);

  k = *biWidth * (*biHeight-1) * 3;
  for(i=0;i<*biHeight;i++) {
    for(j=0;j<*biWidth;j++) {
      fread(&pbuffer[k+2], 1, 1, fp);
      fread(&pbuffer[k+1], 1, 1, fp);
      fread(&pbuffer[k], 1, 1, fp);
      k+=3;
    }
    fseek(fp, pad, SEEK_CUR);
    k -= *biWidth * 2 * 3;
  }

  fclose(fp);

  return pbuffer;
}
