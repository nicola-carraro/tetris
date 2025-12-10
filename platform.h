
static void platformDrawColorQuad(
    float x, float y,
    float width, float height,
    float r, float g, float b, float a,
    TtsPlatform *platform
);

static void platformDrawTextureQuad(
   float x, float y,
   float width, float height,
   float xInTexture, float yInTexture,
   float widthInTexture, float heightInTexture,
   float textureWidth, float textureHeight,
   float r, float g, float b, float a,
   TtsPlatform *platform
  
);

static TtsReadResult platformReadEntireFile(char *path);

static void platformDebugPrint(_Printf_format_string_ const char *format, ...);

static void platformPlayMusic(TtsTetris *tetris, Wav wav);

static void platformPlaySound(TtsTetris *tetris, Wav wav);
