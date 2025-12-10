static int32_t ttsRoundF32ToI32(float f) {
    int32_t result = (int32_t)f;

    if (f - (float)result > 0.5f) {
        result++;
    }

    return result;
}

static inline float color255To1(float color) {
    float result = (color / 255.0f);

    return result;
}

static inline int32_t ttsFloatToI32Floor(float f) {
    int32_t result = (int32_t) f;

    if (f < 0.0f && (f != (float)result)) {
        result--;
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

    result.backgroundColor[0] = color255To1(34.0f);
    result.backgroundColor[1] = color255To1(67.0f);
    result.backgroundColor[2] = color255To1(74.0f);
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

TtsTetraminoPattern ttsGetTetraminoPattern(TtsTetraminoType tetraminoType) {
    TTS_ASSERT(tetraminoType > TtsTetraminoType_None);
    TTS_ASSERT(tetraminoType < TtsTetraminoType_Count);

    TtsTetraminoPattern patterns[TtsTetraminoType_Count] = {
        [TtsTetraminoType_I] = {{{-1.5f, -0.5f}, {-0.5f, -0.5f}, {0.5f, -0.5f}, {1.5f,  -0.5f}}},
        [TtsTetraminoType_O] = {{{-0.5f, -0.5f}, {0.5f, -0.5f}, {-0.5f, 0.5f}, {0.5f, 0.5f}}},
        [TtsTetraminoType_T] = {{{0.0f, -1.0f}, {-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}}},
        [TtsTetraminoType_L] = {{{-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, -1.0f}}},
        [TtsTetraminoType_J] = {{{-1.0f, -1.0f}, {-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}}},
        [TtsTetraminoType_Z] = {{{-1.0f, -1.0f}, {0.0f, -1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}}},
        [TtsTetraminoType_S] = {{{-1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, -1.0f}, {1.0f, -1.0f}}},
    };

    TtsTetraminoPattern result = patterns[tetraminoType];

    return result;
}

static void ttsDrawColorTrapezoid(
    float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    float r, float g, float b, float a,
    TtsPlatform *platform
) {
    platformDrawColorTriangle(
        x0, y0,
        x1, y1,
        x2, y2,
        0.0f, 0.0f,
        r, g, b, a,
        platform
    );

    platformDrawColorTriangle(
        x2, y2,
        x3, y3,
        x0, y0,
        0.0f, 0.0f,
        r, g, b, a,
        platform
    );
}

static void ttsDrawColorQuad(
    float x, float y,
    float width, float height,
    float r, float g, float b, float a,
    TtsPlatform *platform
) {
    ttsDrawColorTrapezoid(
        x, y,
        x + width, y,
        x + width, y + height,
        x, y + height,
        r,  g,  b,  a,
        platform
    );
}

static void ttsDrawCellLikeQuad(
    TtsTetris *tetris,
    float x, float y,
    float width, float height,
    float margin,
    float r, float g, float b, float a
)  {
    float internalWidth = width - (margin * 2.0f);
    float internalHeight = height - (margin * 2.0f);
    float internalX = x + margin;
    float internalY = y + margin;
    ttsDrawColorQuad(
        internalX, internalY,
        internalWidth, internalHeight,
        r, g, b, a,
        tetris->platform
    );

    float lightMultiplier = 1.5f;

    ttsDrawColorTrapezoid(
        x, y,
        internalX, internalY,
        internalX , internalY + internalHeight,
        x, y + height,
        r * lightMultiplier, g * lightMultiplier, b * lightMultiplier, a,
        tetris->platform
    );

    ttsDrawColorTrapezoid(
        x, y,
        x + width, y,
        internalX + internalWidth, internalY,
        internalX, internalY,
        r * lightMultiplier, g * lightMultiplier, b * lightMultiplier, a,
        tetris->platform
    );

    float darkMultiplier = 0.50f;

    ttsDrawColorTrapezoid(
        x + width, y,
        x + width, y + height,
        internalX + internalWidth, internalY + internalHeight,
        internalX + internalWidth, internalY,
        r * darkMultiplier, g * darkMultiplier, b * darkMultiplier, a,
        tetris->platform
    );

    ttsDrawColorTrapezoid(
        internalX, internalY + internalHeight,
        internalX + internalWidth, internalY + internalHeight,
        x + width, y + height,
        x, y + height,
        r * darkMultiplier, g * darkMultiplier, b * darkMultiplier, a,
        tetris->platform
    );
}

static void ttsDrawCell(
    TtsTetris *tetris,
    int32_t row, int32_t column,
    float r, float g, float b, float a,
    float cellSide,
    float gridX, float gridY
)  {
    float x = gridX + (column * cellSide);
    float y = gridY + (row * cellSide);
    float internalSide = cellSide * 0.7f;
    float margin = (cellSide - internalSide) / 2.0f;

    ttsDrawCellLikeQuad(
        tetris,
        x,  y,
        cellSide, cellSide,
        margin,
        r,  g,  b,  a
    );
}

static uint32_t ttsPressCount(TtsControl control) {
    uint32_t transitions = control.transitions;

    if (control.isDown) {
        transitions++;
    }

    uint32_t result = transitions / 2;

    return result;
}

static TtsTetramino getRotatedCells(TtsTetraminoType tetraminoType, TtsRotationType rotationType) {
    TTS_ASSERT(rotationType > TtsRotationType_None);
    TTS_ASSERT(rotationType < TtsRotationType_Count);

    TtsRotation rotations[TtsRotationType_Count] = {
        [TtsRotationType_S] = {false, false},
        [TtsRotationType_R] = {true, false},
        [TtsRotationType_2] = {false, true},
        [TtsRotationType_L] = {true, true},
    };

    TtsRotation rotation = rotations[rotationType];

    TtsTetraminoPattern pattern = ttsGetTetraminoPattern(tetraminoType);

    TtsTetramino result = {0};

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(pattern.cells); cellIndex++) {
        TtsFloatCoords srcCoords = pattern.cells[cellIndex];
        TtsFloatCoords rotatedCoords = srcCoords;
        if (rotation.flipCoordinates) {
            rotatedCoords.x = -srcCoords.y;
            rotatedCoords.y = srcCoords.x;
        }

        if (rotation.flipSign) {
            rotatedCoords.x = -rotatedCoords.x;
            rotatedCoords.y = -rotatedCoords.y;
        }
        result.cells[cellIndex].x = ttsFloatToI32Floor(rotatedCoords.x);
        result.cells[cellIndex].y = ttsFloatToI32Floor(rotatedCoords.y);
    }

    return result;
}

static TtsTetramino ttsGetTetraminoCells(TtsTetraminoType tetraminoType, TtsRotationType rotationType, float x, float y) {
    TtsTetramino rotatedCells = getRotatedCells(tetraminoType, rotationType);
    TtsTetramino result = {0};

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(rotatedCells.cells); cellIndex++) {
        TtsI32Coords cellCoords = rotatedCells.cells[cellIndex];
        result.cells[cellIndex].x = (int32_t)(x) + cellCoords.x;
        result.cells[cellIndex].y = (int32_t)(y) + cellCoords.y;
    }

    return result;
}

static TtsTetramino ttsGetPlayerCells(TtsTetris *tetris) {
    TtsTetramino result = ttsGetTetraminoCells(
        tetris->playerType,
        tetris->playerRotationType,
        tetris->playerXInCells,
        tetris->playerYInCells
    );

    return result;
}

static float ttsGetSpawnY(TtsTetraminoType type) {
    float maxY = -5.0f;

    TtsTetraminoPattern pattern = ttsGetTetraminoPattern(type);

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(pattern.cells); cellIndex++) {
        float y = pattern.cells[cellIndex].y;

        if (y > maxY) {
            maxY = y;
        }
    }

    float result = -maxY;

    return result;
}

static void ttsUpdate(TtsTetris *tetris, float secondsElapsed) {
    TTS_UNREFERENCED(secondsElapsed);

    if (tetris->frame == 0) {
        platformPlayMusic(tetris, tetris->music);
        tetris->playerXInCells = 5.5f;
        tetris->playerType = TtsTetraminoType_I;
        tetris->playerYInCells = ttsGetSpawnY(tetris->playerType);
        tetris->playerRotationType = TtsRotationType_S;
    }

    if (ttsPressCount(tetris->controls[TtsControlType_P]) % 2 != 0) {
        tetris->paused = !tetris->paused;
    }

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
    float gridX = ((float)tetris->windowWidth - gridWidth) / 2.0f;
    float gridY = ((float)tetris->windowHeight - gridHeight) / 2.0f;

    // Background
    {
        ttsDrawColorQuad(
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
            ttsDrawCell(
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
            ttsDrawCell(
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
            ttsDrawCell(
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
            ttsDrawCell(
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
        float velocity = 5.0f;

        if (!tetris->wasResizing && !tetris->paused) {
            tetris->playerYProgression += velocity * secondsElapsed;
        }

        if (tetris->playerYProgression > 1.0f) {
            float cellsToMove = (float)((uint32_t)tetris->playerYProgression);
            tetris->playerYInCells += cellsToMove;
            tetris->playerYProgression  -= cellsToMove;
        }

        if (tetris->playerYInCells > (float)TTS_ROW_COUNT - 4.0f + 1.0f) {
            tetris->playerYInCells =  ttsGetSpawnY(tetris->playerType);
        }

        if (ttsPressCount(tetris->controls[TtsControlType_Enter]) > 0) {
            tetris->playerType++;
            if (tetris->playerType >= TtsTetraminoType_Count) {
                tetris->playerType = TtsTetraminoType_None + 1;
            }

            tetris->playerRotationType = TtsRotationType_S;
        }

        for (uint32_t rotationIndex = 0; rotationIndex < ttsPressCount(tetris->controls[TtsControlType_Down]); rotationIndex++) {
            tetris->playerRotationType++;
            if (tetris->playerRotationType >= TtsRotationType_Count) {
                tetris->playerRotationType = TtsRotationType_S;
            }
        }
        float r = color255To1(205.0f);
        float g = color255To1(205.0f);
        float b = color255To1(0.0f);
        float a = 1.0f;

        TtsTetramino playerCells = ttsGetPlayerCells(tetris);
        for (int32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(playerCells.cells); cellIndex++) {
            // if (row >= 0) {
            ttsDrawCell(
                tetris,
                playerCells.cells[cellIndex].y,
                playerCells.cells[cellIndex].x,
                r, g, b, a,
                cellSideInPixels,
                gridX,
                gridY
            );
            // }
        }
    }

    float boxHeight = cellSideInPixels * 4;
    float labelX = gridX + gridWidth + (cellSideInPixels * 3);
    {
        {
            ttsDrawCellLikeQuad(
                tetris,
                gridX + gridWidth + (cellSideInPixels * 2), gridY,
                gridWidth, boxHeight,
                5.0f,
                color255To1(223.0f), color255To1(240.0f), color255To1(216.0f), 1.0f
            ) ;

            TtsString next = TTS_MAKE_STRING("Next:");
            ttsDrawString(
                tetris,
                next,
                labelX,
                gridY,
                1.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            );
        }
    }

    {
        float scoreY = gridY + gridHeight - boxHeight;
        ttsDrawCellLikeQuad(
            tetris,
            gridX + gridWidth + (cellSideInPixels * 2), scoreY,
            gridWidth, boxHeight,
            5.0f,
            color255To1(223.0f), color255To1(240.0f), color255To1(216.0f), 1.0f
        ) ;

        TtsString score = TTS_MAKE_STRING("Score:");
        ttsDrawString(
            tetris,
            score,
            labelX,
            scoreY,
            1.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );

        TtsString points = TTS_MAKE_STRING("0.000.000.350");
        ttsDrawString(
            tetris,
            points,
            labelX,
            scoreY + tetris->atlas.lineHeightInPixels,
            1.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

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
