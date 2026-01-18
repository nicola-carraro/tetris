static void platformDrawTextureQuad(
    float x, float y,
    float width, float height,
    float xInTexture, float yInTexture,
    float widthInTexture, float heightInTexture,
    float textureWidth, float textureHeight,
    TtsColor color,
    TtsPlatform *platform
);

static void platformDrawColorTriangle(
    float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    TtsColor color,
    TtsPlatform *win32
);

static TtsReadResult platformReadEntireFile(char *path);

static void platformDebugPrint(_Printf_format_string_ const char *format, ...);

static void platformPlayMusic(TtsTetris *tetris, Wav wav);

static void platformPlaySound(TtsTetris *tetris, Wav wav);
