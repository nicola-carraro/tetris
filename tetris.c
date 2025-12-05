#define TTS_ASSERT(a) do {if (!(a)) { __debugbreak();}} while (0);
#define TTS_QUOTE(s) #s
#define TTS_ARRAYCOUNT(a) (sizeof(a) / sizeof(*a))
#define TTS_UNREFERENCED(a) a
#define TTS_FIRST_CODEPOINT L' '
#define TTS_LAST_CODEPOINT  L'~'
#define TTS_CODEPOINT_COUNT (TTS_LAST_CODEPOINT - TTS_FIRST_CODEPOINT + 1)
#define TTS_PIXELS_PER_POINT 1.33333333333333333f
#define TTS_POINTS_PER_PIXEL 0.75f
#define TTS_FONT_PATH L"../data/Quantico-Regular.ttf"
#define TTS_ATLAS_PATH "../data/atlas.dat"

typedef struct {
    uint32_t width;
    uint32_t height;
} Atlas;

typedef struct {
    void *data;
    uint64_t size;
} ReadResult;

void ttsUpdate(Platform *platform, Atlas *atlas) {
    static float x = 0.0f;
    platformDrawTextureQuad(
        10, 10.0f,   (float)atlas->width, (float)atlas->height,
        0.0f, 0.0f,
        (float)atlas->width, (float)atlas-> height,
        (float)atlas->width, (float)atlas-> height,
        0.0f, 0.0f, 1.0f, 1.0f,
        platform
    );

    platformDrawColorQuad(
        x, 100.0f,
        100.0f, 50.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        platform
    );

    x += 5.0f;
    if (x > 600.0f) {
        x = 0;
    }
}
