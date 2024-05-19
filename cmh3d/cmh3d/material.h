#ifndef MATERIAL_H
#define MATERIAL_H

typedef struct {
  char* name;
  float r,g,b;
  float ka;  /* ambient coefficient */
  float kd;  /* diffuse coefficient */
  float ks;  /* specular coefficient */
  float ns;  /* specular exponent */
  unsigned char* ptexture;
  unsigned int textureWidth,textureHeight;
  float uScale,vScale;
} material;

#ifdef __cplusplus
extern "C" {
#endif

void loadTextureFromBmpFile(material* pmat, char* fileName);

#ifdef __cplusplus
}
#endif

#endif /* MATERIAL_H */
