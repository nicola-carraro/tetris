typedef struct TtsPlatform TtsPlatform;

void platformDrawColorQuad(
    float x, float y,
    float width, float height,
    float r, float g, float b, float a,
    TtsPlatform *platform
);

void platformDrawTextureQuad(
   float x, float y,
   float width, float height,
   float xInTexture, float yInTexture,
   float widthInTexture, float heightInTexture,
   float textureWidth, float textureHeight,
   float r, float g, float b, float a,
   TtsPlatform *win32
  
);