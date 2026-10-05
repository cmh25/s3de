#ifndef MATERIAL_H
#define MATERIAL_H

typedef struct {
  char* name;  /* heap allocated or NULL: FreeWorld() frees it, so never a literal */
  float r,g,b;
  float ka;  /* ambient coefficient */
  float kd;  /* diffuse coefficient */
  float ks;  /* specular coefficient */
  float ns;  /* specular exponent */
  unsigned char* ptexture;
  unsigned int textureWidth,textureHeight;
  float uScale,vScale;
  float* specTable;    /* x^ns lookup, built by the rasterizer on first use and freed by FreeWorld(): leave NULL */
  float specTableNs;   /* the exponent the table was built for */
} material;

#ifdef __cplusplus
extern "C" {
#endif

void loadTextureFromBmpFile(material* pmat, char* fileName);

#ifdef __cplusplus
}
#endif

#endif /* MATERIAL_H */
