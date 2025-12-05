typedef struct Platform Platform;

void platformDrawColorQuad(
    float x, float y,
    float width, float height,
    float r, float g, float b, float a,
    Platform *platform
);

void platformDrawTextureQuad(
   float x, float y,
   float width, float height,
   float xInTexture, float yInTexture,
   float widthInTexture, float heightInTexture,
   float textureWidth, float textureHeight,
   float r, float g, float b, float a,
   Platform *win32
  
);