#include "matrix.h"
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "vertex.h"

void Translate(vertex* v, float tx, float ty, float tz) {
  matrix4x4 t = {
    {  1.0,  0.0,  0.0,  tx  },
    {  0.0,  1.0,  0.0,  ty  },
    {  0.0,  0.0,  1.0,  tz  },
    {  0.0,  0.0,  0.0,  1.0 }
  };

  MatrixMultiply1x4_4x4(v, t);
}

void Scale(vertex* v, vertex* vs, float sx, float sy, float sz) {
  matrix4x4 s = {
    {  sx,   0.0,  0.0,  0.0  },
    {  0.0,  sy,   0.0,  0.0  },
    {  0.0,  0.0,  sz,   0.0  },
    {  0.0,  0.0,  0.0,  1.0  }
  };

  matrix4x4 t1 = {
    {  1.0,  0.0,  0.0,  -vs->x  },
    {  0.0,  1.0,  0.0,  -vs->y  },
    {  0.0,  0.0,  1.0,  -vs->z  },
    {  0.0,  0.0,  0.0,  1.0   }
  };

  matrix4x4 t2 = {
    {  1.0,  0.0,  0.0,  vs->x  },
    {  0.0,  1.0,  0.0,  vs->y  },
    {  0.0,  0.0,  1.0,  vs->z  },
    {  0.0,  0.0,  0.0,  1.0    }
  };

  MatrixMultiply4x4_4x4(t1, s);
  MatrixMultiply4x4_4x4(t1, t2);
  MatrixMultiply1x4_4x4(v, t1);
}

void Rotatex(vertex* v, vertex* vr, float deg) {
  float rads = deg * PI / 180.0f;
  float s = sinf(rads);
  float c = cosf(rads);

  matrix4x4 rx = {
    {  1.0,  0.0,  0.0,  0.0  },
    {  0.0,  c,    -s,   0.0  },
    {  0.0,  s,    c,    0.0  },
    {  0.0,  0.0,  0.0,  1.0  }
  };

  matrix4x4 t1 = {
    {  1.0,  0.0,  0.0,  -vr->x  },
    {  0.0,  1.0,  0.0,  -vr->y  },
    {  0.0,  0.0,  1.0,  -vr->z  },
    {  0.0,  0.0,  0.0,  1.0     }
  };

  matrix4x4 t2 =  {
    {  1.0,  0.0,  0.0,  vr->x  },
    {  0.0,  1.0,  0.0,  vr->y  },
    {  0.0,  0.0,  1.0,  vr->z  },
    {  0.0,  0.0,  0.0,  1.0    }
  };

  MatrixMultiply4x4_4x4(t1, rx);
  MatrixMultiply4x4_4x4(t1, t2);
  MatrixMultiply1x4_4x4(v, t1);
}

void Rotatey(vertex* v, vertex* vr, float deg) {
  float rads = deg * PI / 180;
  float s = sinf(rads);
  float c = cosf(rads);

  matrix4x4 ry = {
    {  c,    0.0,  s,    0.0  },
    {  0.0,  1.0,  0.0,  0.0  },
    {  -s,   0.0,  c,    0.0  },
    {  0.0,  0.0,  0.0,  1.0  }
  };

  matrix4x4 t1 = {
    {  1.0,  0.0,  0.0,  -vr->x  },
    {  0.0,  1.0,  0.0,  -vr->y  },
    {  0.0,  0.0,  1.0,  -vr->z  },
    {  0.0,  0.0,  0.0,  1.0     }
  };

  matrix4x4 t2 = {
    {  1.0,  0.0,  0.0,  vr->x  },
    {  0.0,  1.0,  0.0,  vr->y  },
    {  0.0,  0.0,  1.0,  vr->z  },
    {  0.0,  0.0,  0.0,  1.0    }
  };

  MatrixMultiply4x4_4x4(t1, ry);
  MatrixMultiply4x4_4x4(t1, t2);
  MatrixMultiply1x4_4x4(v, t1);
}

void Rotatez(vertex* v, vertex* vr, float deg) {
  float rads = deg * PI / 180;
  float s = sinf(rads);
  float c = cosf(rads);

  matrix4x4 rz = {
    {  c,    -s,   0.0,  0.0  },
    {  s,    c,    0.0,  0.0  },
    {  0.0,  0.0,  1.0,  0.0  },
    {  0.0,  0.0,  0.0,  1.0  }
  };

  matrix4x4 t1 =  {
    {  1.0,  0.0,  0.0,  -vr->x  },
    {  0.0,  1.0,  0.0,  -vr->y  },
    {  0.0,  0.0,  1.0,  -vr->z  },
    {  0.0,  0.0,  0.0,  1.0     }
  };

  matrix4x4 t2 =  {
    {  1.0,  0.0,  0.0,  vr->x  },
    {  0.0,  1.0,  0.0,  vr->y  },
    {  0.0,  0.0,  1.0,  vr->z  },
    {  0.0,  0.0,  0.0,  1.0    }
  };

  MatrixMultiply4x4_4x4(t1, rz);
  MatrixMultiply4x4_4x4(t1, t2);
  MatrixMultiply1x4_4x4(v, t1);
}

void MatrixMultiply1x4_4x4(vertex* v, matrix4x4 m1) {
  int i;
  float vret[4];

  memset(vret, 0, 4 * sizeof(float));

  for(i=0;i<4;i++) {
    vret[i] += v->x * m1[i][0];
    vret[i] += v->y * m1[i][1];
    vret[i] += v->z * m1[i][2];
    vret[i] += v->h * m1[i][3];
  }

  v->x = vret[0];
  v->y = vret[1];
  v->z = vret[2];
  v->h = vret[3];
}

void MatrixMultiply4x4_4x4(matrix4x4 m1, matrix4x4 m2) {
  int i, j, k;
  matrix4x4 m3;

  memset(m3, 0, sizeof(matrix4x4));

  for (k=0;k<4;k++)
    for (i=0;i<4;i++)
      for (j=0;j<4;j++)
        m3[i][k] += m1[j][k] * m2[i][j];

  memcpy(m1, m3, sizeof(matrix4x4));
}

void SetIdentity(matrix4x4 m1) {
  memset(m1, 0, sizeof(matrix4x4));
  m1[0][0] = 1.0;
  m1[1][1] = 1.0;
  m1[2][2] = 1.0;
  m1[3][3] = 1.0;
}

void GetRotateMatrix(matrix4x4 m1, float xRot, float yRot, float zRot) {
  float xrad,yrad,zrad,xs,xc,ys,yc,zs,zc;
  matrix4x4 xm,ym,zm;

  SetIdentity(m1);
  SetIdentity(xm);
  SetIdentity(ym);
  SetIdentity(zm);

  xrad = xRot * PI / 180.0f;
  yrad = yRot * PI / 180.0f;
  zrad = zRot * PI / 180.0f;

  xs = sinf(xrad);
  xc = cosf(xrad);
  ys = sinf(yrad);
  yc = cosf(yrad);
  zs = sinf(zrad);
  zc = cosf(zrad);

  xm[1][1] =  xc;
  xm[1][2] = -xs;
  xm[2][1] =  xs;
  xm[2][2] =  xc;

  ym[0][0] =  yc;
  ym[0][2] =  ys;
  ym[2][0] = -ys;
  ym[2][2] =  yc;

  zm[0][0] =  zc;
  zm[0][1] = -zs;
  zm[1][0] =  zs;
  zm[1][1] =  zc;

  MatrixMultiply4x4_4x4(m1, xm);
  MatrixMultiply4x4_4x4(m1, ym);
  MatrixMultiply4x4_4x4(m1, zm);
}

void GetTranslateMatrix(matrix4x4 m1, float xTran, float yTran, float zTran) {
  SetIdentity(m1);
  m1[0][3] = xTran;
  m1[1][3] = yTran;
  m1[2][3] = zTran;
}
