static int32_t roundF32ToI32(float f) {
    int32_t result = (int32_t)f;

    if (f - (float)result > 0.5f) {
        result++;
    }

    return result;
}

static void ttsDrawGlyph(
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

static void ttsDrawString(
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

static Wav ttsWavParseFile(TtsReadResult file) {
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

    result.backgroundColor[0] = 1.0f;
    result.backgroundColor[1] = 1.0f;
    result.backgroundColor[2] = 1.0f;
    result.backgroundColor[3] = 1.0f;

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

static void drawCell(
    TtsTetris *tetris,
    int32_t row,
    int32_t column,
    float r,
    float g,
    float b,
    float a,
    float cellSide,
    float gridX,
    float gridY
)  {
    float x = gridX + (column * cellSide);
    float y = gridY + (row * cellSide);
    float quadSide = cellSide * 0.9f;
    float quadMargin = (cellSide - quadSide) / 2.0f;
    float quadX = x + quadMargin;
    float quadY = y + quadMargin;
    float quadHeight = quadSide;
    float quadWidth = quadSide;

    platformDrawColorQuad(
        quadX, quadY,
        quadWidth, quadHeight,
        r, g, b, a,
        tetris->platform
    );
}

static inline float color255To1(float color) {
    float result = (color / 255.0f);

    return result;
}

uint32_t ttsPressCount(TtsControl control) {
    uint32_t result = 0;

    if (control.transitions > 0) {
        result = control.transitions / 2;
        if (control.isDown) {
            result++;
        }
    }

    return result;
}

static void ttsUpdate(TtsTetris *tetris, float secondsElapsed) {
    if (tetris->frame == 0) {
        platformPlayMusic(tetris, tetris->music);
        tetris->playerColumn = 5;
        tetris->playerYInCells = -4.0f;
    }
    TTS_UNREFERENCED(secondsElapsed);

    if (ttsPressCount(tetris->controls[TtsControlType_P]) % 2 != 0) {
        tetris->paused = !tetris->paused;
    }

    float velocity = 5.0f;

    if (!tetris->wasResizing && !tetris->paused) {
        tetris->playerYInCells += velocity * secondsElapsed;
    }

    if (tetris->playerYInCells > (float)TTS_ROW_COUNT - 4.0f + 1.0f) {
        tetris->playerYInCells = -4.0f;
    }

    // static float x = 0.0f;

    int32_t boardWidthInColumns = TTS_COLUMN_COUNT + 2;
    int32_t boardWidthInRows = TTS_ROW_COUNT + 2;
    float aspectRatio = ((float)tetris->windowWidth / boardWidthInColumns) / ((float)tetris->windowHeight / boardWidthInRows);
    float cellSideInPixels = 0.0f;
    if (aspectRatio > 1.0f) {
        cellSideInPixels = ((float)tetris->windowHeight * TTS_MAX_HEIGTH_RATIO) / boardWidthInRows;
    } else {
        cellSideInPixels = ((float)tetris->windowWidth * TTS_MAX_WIDTH_RATIO) / boardWidthInColumns;
    }

    float gridWidth = cellSideInPixels * TTS_COLUMN_COUNT;
    float gridHeight = cellSideInPixels * TTS_ROW_COUNT;
    float gridX = cellSideInPixels * 2.0f;
    float gridY = ((float)tetris->windowHeight - gridHeight) / 2.0f;

    // Background
    {
        platformDrawColorQuad(
            gridX, gridY,
            gridWidth, gridHeight,
            0.0f, 0.0f, 0.0f, 1.0f,
            tetris->platform
        );
    }

    //Frame
    {
        float r = color255To1(102.0f);
        float g = color255To1(102.0f);
        float b = color255To1(102.0f);
        float a = 1.0f;

        for (int32_t column = -1; column < TTS_COLUMN_COUNT + 1; column++) {
            int32_t row = -1;
            drawCell(
                tetris,
                row,
                column,
                r,
                g,
                b,
                a,
                cellSideInPixels,
                gridX,
                gridY
            );

            row = TTS_ROW_COUNT;
            drawCell(
                tetris,
                row,
                column,
                r,
                g,
                b,
                a,
                cellSideInPixels,
                gridX,
                gridY
            );
        }

        for (int32_t row = 0; row < TTS_ROW_COUNT; row++) {
            int32_t column = -1;
            drawCell(
                tetris,
                row,
                column,
                r,
                g,
                b,
                a,
                cellSideInPixels,
                gridX,
                gridY
            );

            column = TTS_COLUMN_COUNT;
            drawCell(
                tetris,
                row,
                column,
                r,
                g,
                b,
                a,
                cellSideInPixels,
                gridX,
                gridY
            );
        }
    }

    // Player
    {
        float r = color255To1(205.0f);
        float g = color255To1(205.0f);
        float b = color255To1(0.0f);
        float a = 1.0f;

        for (int32_t cell = 0; cell < 4; cell++) {
            int32_t row = (int32_t)tetris->playerYInCells + cell;

            if (row >= 0) {
                drawCell(
                    tetris,
                    row,
                    tetris->playerColumn,
                    r, g, b, a,
                    cellSideInPixels,
                    gridX,
                    gridY
                );
            }
        }
    }

    TtsString score = TTS_MAKE_STRING("Score: 0 009 000");
    ttsDrawString(
        tetris,
        score,
        gridX + gridWidth + (cellSideInPixels * 2),
        gridY + (cellSideInPixels * 2),
        1.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    TtsString level = TTS_MAKE_STRING("Level: 001");
    ttsDrawString(
        tetris,
        level,
        gridX + gridWidth + (cellSideInPixels * 2),
        gridY + gridHeight - (cellSideInPixels * 2) - tetris->atlas.lineHeightInPixels,
        1.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    // TtsString string = TTS_MAKE_STRING("Tetris!");
    // ttsDrawString(
    // tetris,
    // string,
    // 10.0f,  10.0f,
    // 1.0f,
    // 0.0f, 0.0f, 1.0f, 1.0f
    // );

    // platformDrawColorQuad(
    // x, 100.0f,
    // 100.0f, 50.0f,
    // 1.0f, 0.0f, 0.0f, 1.0f,
    // tetris->platform
    // );

    // if (x > 600.0f) {
    // x = 0;
    // }

    for (uint32_t controlIndex = 1; controlIndex < TTS_ARRAYCOUNT(tetris->controls); controlIndex++) {
        tetris->controls[controlIndex].wasDown = tetris->controls[controlIndex].isDown;
        tetris->controls[controlIndex].transitions = 0;
    }
    tetris->frame++;
}

static bool ttsWavIsValid(Wav wav) {
    bool result = wav.riffChunk && wav.fmtChunk && wav.data;

    return result;
}
