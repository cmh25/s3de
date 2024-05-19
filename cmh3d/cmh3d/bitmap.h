#ifndef BITMAP_H
#define BITMAP_H

#ifdef __cplusplus
extern "C" {
#endif
	
unsigned char* readBmp(char* fileName, unsigned int* biWidth, unsigned int* biHeight);

#ifdef __cplusplus
}
#endif

#endif /* BITMAP_H */
