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
#define TTS_MAKE_STRING(a) {(a), (sizeof(a) - 1)}

int32_t roundF32ToI32(float f) {
    int32_t result = (int32_t)f;

    if (f - (float)result > 0.5f) {
        result++;
    }

    return result;
}

void ttsDrawGlyph(
    TtsTetris *tetris,
    TtsGlyph glyph,
    float x, float y,
    float scale,
    float r, float g, float b, float a
) {
    float quadX = x + glyph.xOffsetInPixels;
    float quadY = y + glyph.yOffsetInPixels;
    float quadWidth = glyph.bitmapWidthInPixels * scale;
    float quadHeight = glyph.bitmapHeightInPixels * scale;
    platformDrawTextureQuad(
        quadX, quadY,
        quadWidth, quadHeight,
        glyph.bitmapXInPixels, glyph.bitmapYInPixels,
        glyph.bitmapWidthInPixels, glyph.bitmapHeightInPixels,
        tetris->atlas.width, tetris->atlas.height,
        r, g, b,  a,
        tetris->platform
    );
}

void ttsDrawString(
    TtsTetris *tetris,
    TtsString string,
    float x, float y,
    float scale,
    float r, float g, float b, float a
) {
    for (char codepointIndex = 0; codepointIndex < string.size; codepointIndex++) {
        char codepoint = string.text[codepointIndex];
        TTS_ASSERT(codepoint >= TTS_FIRST_CODEPOINT);
        TTS_ASSERT(codepoint <= TTS_LAST_CODEPOINT);

        uint32_t index = codepoint - TTS_FIRST_CODEPOINT;
        TTS_ASSERT(index < TTS_ARRAYCOUNT(tetris->atlas.glyphs));

        TtsGlyph glyph = tetris->atlas.glyphs[index];
        ttsDrawGlyph(
            tetris,
            glyph,
            x,  y,
            scale,
            r,  g,  b,  a
        );

        x += glyph.advanceWidthInPixels * scale;
    }
}

static void ttsUpdate(TtsTetris *tetris, float secondsElapsed) {
    static float x = 0.0f;

    float velocity = 50.0f;

    TtsString string = TTS_MAKE_STRING("Tetris!");
    ttsDrawString(
        tetris,
        string,
        10.0f,  10.0f,
        1.0f,
        0.0f, 0.0f, 1.0f, 1.0f
    );

    platformDrawColorQuad(
        x, 100.0f,
        100.0f, 50.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        tetris->platform
    );

    if (!tetris->wasResizing) {
        x += velocity * secondsElapsed;
    }

    if (x > 600.0f) {
        x = 0;
    }
}
