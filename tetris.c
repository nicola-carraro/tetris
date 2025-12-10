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

Wav ttsWavParseFile(TtsReadResult file) {
    uint8_t *bytes = (uint8_t*) file.data;

    Wav wav = {0};

    RiffChunk *riffChunk = (RiffChunk *)bytes;
    wav.riffChunk = riffChunk;

    WavChunkHeader *chunkHeader = 0;
    for (uint64_t offset = sizeof(RiffChunk); (offset + sizeof(RiffChunk) - 4) < riffChunk->chunkSize && offset < file.size; offset += (chunkHeader->chunkSize + sizeof(WavChunkHeader))) {
        chunkHeader = (WavChunkHeader *) (bytes + offset);

        if (chunkHeader->chunkId == ' tmf') {
            WavFmtChunk *fmtChunk = (WavFmtChunk *)(bytes + offset + sizeof(WavChunkHeader));
            wav.fmtChunk = fmtChunk;
            #if 0
            platformDebugPrint("formatTag %u\n", fmtChunk->formatTag);
            platformDebugPrint("channels %u\n", fmtChunk->channels);
            platformDebugPrint("samplesPerSec %u\n", fmtChunk->samplesPerSec);
            platformDebugPrint("avgBytesPerSec %u\n", fmtChunk->avgBytesPerSec);
            platformDebugPrint("blockAlign %u\n", fmtChunk->blockAlign);
            platformDebugPrint("bitsPerSample %u\n", fmtChunk->bitsPerSample);
            platformDebugPrint("extensionSize %u\n", fmtChunk->extensionSize);
            platformDebugPrint("validBitsPerSample %u\n", fmtChunk->validBitsPerSample);
            platformDebugPrint("channelMask %u\n", fmtChunk->channelMask);
            #endif
        }

        if (chunkHeader->chunkId == 'atad') {
            void *data = (void *)(bytes + offset + sizeof(WavChunkHeader));
            wav.data = data;
            wav.dataSize = chunkHeader->chunkSize;
        }
    }

    return wav;
}

static TtsTetris ttsInit(TtsPlatform *platform, bool hasSound) {
    TtsTetris result = {0};
    bool ok = 0;
    result.platform = platform;
    result.hasSound = hasSound;

    TtsReadResult musicFile = {0};
    musicFile = platformReadEntireFile("../data/tetris.wav");
    ok = musicFile.size > 0;

    TtsReadResult soundFile = {0};
    if (ok) {
        soundFile = platformReadEntireFile("../data/sound.wav");
        ok = musicFile.size > 0;
    }

    if (ok) {
        result.music = ttsWavParseFile(musicFile);
        result.sound = ttsWavParseFile(soundFile);
    }

    return result;
}

static void ttsUpdate(TtsTetris *tetris, float secondsElapsed) {
    if (tetris->frame == 0) {
        platformPlayMusic(tetris, tetris->music);
    }
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

    tetris->frame++;
}

bool ttsWavIsValid(Wav wav) {
    bool result = wav.riffChunk && wav.fmtChunk && wav.data;

    return result;
}
