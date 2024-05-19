#ifndef MATRIX_H
#define MATRIX_H

#include "vertex.h"

#define COLUMNS 4
#define ROWS    4
#define PI      3.141592654f

typedef float matrix4x4[ ROWS ][ COLUMNS ];

#ifdef __cplusplus
extern "C" {
#endif

void Translate(vertex* v, float tx, float ty, float tz);
void Scale(vertex* v, vertex* vs, float sx, float sy, float sz);
void Rotatex(vertex* v, vertex* vr, float deg);
void Rotatey(vertex* v, vertex* vr, float deg);
void Rotatez(vertex* v, vertex* vr, float deg);
void MatrixMultiply1x4_4x4(vertex* v, matrix4x4 m1);
void MatrixMultiply4x4_4x4(matrix4x4 m1, matrix4x4 m2);
void SetIdentity(matrix4x4 m1);
void GetRotateMatrix(matrix4x4 m1, float xRot, float yRot, float zRot);
void GetTranslateMatrix(matrix4x4 m1, float xTran, float yTran, float zTran);

#ifdef __cplusplus
}
#endif

#endif /* MATRIX_H */
