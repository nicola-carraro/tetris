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

static TtsReadResult platformReadEntireFile(char *path, TtsArena *arena);

static void platformDebugPrint(_Printf_format_string_ const char *format, ...);

static void platformPlaySound(TtsTetris *tetris, Wav wav, TtsSoundType soundType);

static void platformPauseSound(TtsTetris *tetris, TtsSoundType soundType);

static void platformResumeSound(TtsTetris *tetris, TtsSoundType soundType);

static void platformMemset(void * pointer, int value, size_t count);

static void *platformAllocate(uint64_t size);