static int32_t ttsRoundF32ToI32(float f) {
    int32_t result = (int32_t)f;

    if (f - (float)result > 0.5f) {
        result++;
    }

    return result;
}

static inline int32_t ttsFloatToI32Floor(float f) {
    int32_t result = (int32_t) f;

    if (f < 0.0f && (f != (float)result)) {
        result--;
    }

    return result;
}

static TtsColor ttsMakeColor(float r, float g, float b, float a) {
    TtsColor result = {r, g, b, a};

    return result;
}

static void ttsDrawGlyph(
    TtsTetris *tetris,
    TtsGlyph glyph,
    float x, float y,
    float scale,
    TtsColor color
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
        color,
        tetris->platform
    );
}

static void ttsDrawString(
    TtsTetris *tetris,
    TtsString string,
    float x, float y,
    float scale,
    TtsColor color
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
            color
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

    result.backgroundColor = ttsMakeColor(34.0f, 67.0f, 74.0f, 255.0f);

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

TtsColor ttsGetTetraminoColor(TtsTetraminoType tetraminoType) {
    TTS_ASSERT(tetraminoType > TtsTetraminoType_None);
    TTS_ASSERT(tetraminoType < TtsTetraminoType_Count);

    TtsColor colors[TtsTetraminoType_Count] = {
        [TtsTetraminoType_I] = {0.0f, 205.0f, 205.0f, 255.0f},
        [TtsTetraminoType_O] = {205.0f, 205.0f, 0.0f, 255.0f},
        [TtsTetraminoType_T] = {154.0f, 0.0f, 205.0f, 255.0f},
        [TtsTetraminoType_L] = {205.0f, 102.0f, 0.0f, 255.0f},
        [TtsTetraminoType_J] = {0.0f, 0.0f, 205.0f, 255.0f},
        [TtsTetraminoType_Z] = {0.0f, 205.0f, 0.0f, 255.0f},
        [TtsTetraminoType_S] = {205.0f, 0.0f, 0.0f, 255.0f},
    };

    TtsColor result = colors[tetraminoType];

    return result;
}

static void ttsDrawColorTrapezoid(
    float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    TtsColor color,
    TtsPlatform *platform
) {
    platformDrawColorTriangle(
        x0, y0,
        x1, y1,
        x2, y2,
        0.0f, 0.0f,
        color,
        platform
    );

    platformDrawColorTriangle(
        x2, y2,
        x3, y3,
        x0, y0,
        0.0f, 0.0f,
        color,
        platform
    );
}

static void ttsDrawColorQuad(
    float x, float y,
    float width, float height,
    TtsColor color,
    TtsPlatform *platform
) {
    ttsDrawColorTrapezoid(
        x, y,
        x + width, y,
        x + width, y + height,
        x, y + height,
        color,
        platform
    );
}

TtsColor ttsMultiplyColor(TtsColor color, float multiplier) {
    TtsColor result  = color;

    result.r = ((result.r / 255.0f) * multiplier) * 255.0f;
    result.g = ((result.g / 255.0f) * multiplier) * 255.0f;
    result.b = ((result.b / 255.0f) * multiplier) * 255.0f;

    return result;
}

static void ttsDrawCellLikeQuad(
    TtsTetris *tetris,
    float x, float y,
    float width, float height,
    float margin,
    TtsColor color
)  {
    float internalWidth = width - (margin * 2.0f);
    float internalHeight = height - (margin * 2.0f);
    float internalX = x + margin;
    float internalY = y + margin;
    ttsDrawColorQuad(
        internalX, internalY,
        internalWidth, internalHeight,
        color,
        tetris->platform
    );

    float lightMultiplier = 1.5f;

    ttsDrawColorTrapezoid(
        x, y,
        internalX, internalY,
        internalX , internalY + internalHeight,
        x, y + height,
        ttsMultiplyColor(color, lightMultiplier),
        tetris->platform
    );

    ttsDrawColorTrapezoid(
        x, y,
        x + width, y,
        internalX + internalWidth, internalY,
        internalX, internalY,
        ttsMultiplyColor(color, lightMultiplier),
        tetris->platform
    );

    float darkMultiplier = 0.50f;

    ttsDrawColorTrapezoid(
        x + width, y,
        x + width, y + height,
        internalX + internalWidth, internalY + internalHeight,
        internalX + internalWidth, internalY,
        ttsMultiplyColor(color, darkMultiplier),
        tetris->platform
    );

    ttsDrawColorTrapezoid(
        internalX, internalY + internalHeight,
        internalX + internalWidth, internalY + internalHeight,
        x + width, y + height,
        x, y + height,
        ttsMultiplyColor(color, darkMultiplier),
        tetris->platform
    );
}

static void ttsDrawCell(
    TtsTetris *tetris,
    int32_t row, int32_t column,
    TtsColor color,
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
        color
    );
}

static uint32_t ttsPressCount(TtsControl control) {
    uint32_t transitions = control.transitions;

    if (control.endedDown) {
        transitions++;
    }

    uint32_t result = transitions / 2;

    return result;
}

static TtsRotation ttsGetRotation(TtsRotationType rotationType) {
    TTS_ASSERT(rotationType > TtsRotationType_None);
    TTS_ASSERT(rotationType < TtsRotationType_Count);

    TtsRotation rotations[TtsRotationType_Count] = {
        [TtsRotationType_S] = {false, false},
        [TtsRotationType_R] = {true, false},
        [TtsRotationType_2] = {false, true},
        [TtsRotationType_L] = {true, true},
    };

    TtsRotation rotation = rotations[rotationType];

    return rotation;
}

static TtsTetraminoPattern ttsGetRotatedCells(TtsTetraminoType tetraminoType, TtsRotationType rotationType) {
    TtsTetraminoPattern pattern = ttsGetTetraminoPattern(tetraminoType);

    TtsRotation rotation = ttsGetRotation(rotationType);

    TtsTetraminoPattern result = {0};

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
        result.cells[cellIndex].x = rotatedCoords.x;
        result.cells[cellIndex].y = rotatedCoords.y;
    }

    return result;
}

static TtsTetramino ttsGetTetraminoCells(TtsTetraminoType tetraminoType, TtsRotationType rotationType, float x, float y) {
    TtsTetraminoPattern rotatedCells = ttsGetRotatedCells(tetraminoType, rotationType);
    TtsTetramino result = {0};

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(rotatedCells.cells); cellIndex++) {
        TtsFloatCoords cellCoords = rotatedCells.cells[cellIndex];
        result.cells[cellIndex].x = ttsFloatToI32Floor(x + cellCoords.x);
        result.cells[cellIndex].y = ttsFloatToI32Floor(y + cellCoords.y);
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

static float ttsGetSpawnX(TtsTetraminoType type) {
    TTS_ASSERT(type > TtsTetraminoType_None);
    TTS_ASSERT(type < TtsTetraminoType_Count);

    TtsTetraminoPattern pattern = ttsGetTetraminoPattern(type);

    float minX = 5.0f;
    float maxX = -5.0f;

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(pattern.cells); cellIndex++) {
        float x = pattern.cells[cellIndex].x;

        if (x < minX) {
            minX = x;
        }
        if (x > maxX) {
            maxX = x;
        }
    }

    float horizontalSpan = maxX - minX;

    float result = 0.0f;
    if ((int32_t)horizontalSpan % 2 == 0) {
        result = 4.5f;
    } else {
        result = 5.0f;
    }

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

    float result = -maxY - 1.0f;

    return result;
}

static bool ttsIsCellAvailable(TtsTetris *tetris, int32_t x, int32_t y) {
    bool result = false;

    if (x >= 0 && x < TTS_ARRAYCOUNT(tetris->grid[0])) {
        if (y < 0) {
            result = true;
        } else if (y < TTS_ARRAYCOUNT(tetris->grid)) {
            result = tetris->grid[y][x] == TtsTetraminoType_None;
        }
    }

    return result;
}

static bool ttsIsPositionAvailable(TtsTetris *tetris, TtsTetramino position) {
    bool result = true;

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(position.cells); cellIndex++) {
        TtsI32Coords cell = position.cells[cellIndex];
        if (!ttsIsCellAvailable(tetris, cell.x, cell.y)) {
            result = false;
            break;
        }
    }

    return result;
}

static TtsTetramino ttsOffsetCells(TtsTetramino cells, int32_t x, int32_t y) {
    TtsTetramino result = cells;

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(result.cells); cellIndex++) {
        result.cells[cellIndex].x += x;
        result.cells[cellIndex].y += y;
    }

    return result;
}

bool tryRotation(TtsTetris *tetris, TtsRotationType newRotation, float xOffsetInCells, float yOffsetInCells) {
    bool ok = false;
    TtsTetramino targetCells = ttsGetTetraminoCells(
        tetris->playerType,
        newRotation,
        tetris->playerXInCells + xOffsetInCells,
        tetris->playerYInCells + yOffsetInCells
    );
    if (ttsIsPositionAvailable(tetris, targetCells)) {
        tetris->playerRotationType = newRotation;
        tetris->playerXInCells += xOffsetInCells;
        tetris->playerYInCells += yOffsetInCells;
        ok = true;
    }

    return ok;
}

void ttsRotatePlayer(TtsTetris *tetris, int32_t rotation) {
    TTS_ASSERT(rotation >= -2);
    TTS_ASSERT(rotation <= 2);

    int32_t newRotation = (int32_t)tetris->playerRotationType + rotation;

    if ((newRotation) >= (int32_t)TtsRotationType_Count) {
        newRotation = newRotation - (int32_t)TtsRotationType_Count + 1;
    }

    if ((newRotation) <= 0) {
        newRotation =  (int32_t)TtsRotationType_Count - 1 - newRotation;
    }

    bool ok = tryRotation(tetris, (TtsRotationType)newRotation, 0.0f, 0.0f);

    if (!ok) {
        ok = tryRotation(tetris, (TtsRotationType)newRotation, 1.0f, 0.0f);
    }

    if (!ok) {
        tryRotation(tetris, (TtsRotationType)newRotation, -1.0f, 0.0f);
    }
}

static void spawnTetramino(TtsTetris *tetris, TtsTetraminoType type) {
    tetris->playerType = type;
    tetris->playerXInCells = ttsGetSpawnX(type);
    tetris->playerYInCells = ttsGetSpawnY(type);
    tetris->playerRotationType = TtsRotationType_None + 1;
    tetris->horizontalDirection = TtsHorizontalDirection_None;
    tetris->playerXProgression = 0.0f;
    tetris->playerYProgression = 0.0f;
}

static bool ttsIsRowFull(TtsTetris *tetris, int32_t row) {
    bool result = true;

    if (row >= 0 && row < TTS_ROW_COUNT) {
        for (int32_t column = 0; column < TTS_COLUMN_COUNT && result; column++) {
            if (ttsIsCellAvailable(tetris, column, row)) {
                result = false;
            }
        }
    }

    return result;
}

static bool ttsIsRowEmpty(TtsTetris *tetris, int32_t row) {
    bool result = true;

    if (row >= 0 && row < TTS_ROW_COUNT) {
        for (int32_t column = 0; column < TTS_COLUMN_COUNT && result; column++) {
            if (!ttsIsCellAvailable(tetris, column, row)) {
                result = false;
            }
        }
    }

    return result;
}
static void ttsClearRow(TtsTetris *tetris, int32_t y) {
    for (int32_t x = 0; x < TTS_COLUMN_COUNT; x++) {
        tetris->grid[y][x] = TtsTetraminoType_None;
    }
}

static void ttsMoveVertically(TtsTetris *tetris) {
    TtsTetramino playerCells = ttsGetPlayerCells(tetris);
    TtsTetramino cellsBelow = ttsOffsetCells(playerCells, 0, 1);
    if (ttsIsPositionAvailable(tetris, cellsBelow)) {
        tetris->playerYProgression -= 1.0f;
        tetris->playerYInCells += 1.0f;
    } else {
        int32_t minY = TTS_ROW_COUNT;
        int32_t maxY = -2;
        for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(playerCells.cells); cellIndex++) {
            TtsI32Coords cell = playerCells.cells[cellIndex];
            if (cell.y < minY) {
                minY = cell.y;
            }
            if (cell.y > maxY) {
                maxY = cell.y;
            }
            if (
                cell.y >= 0 && cell.y < TTS_ARRAYCOUNT(tetris->grid)
                && cell.x >= 0 && cell.x < TTS_ARRAYCOUNT(tetris->grid[0])
            ){
                tetris->grid[cell.y][cell.x] = tetris->playerType;
            }
        }

        int32_t clearedRows[4] = {0};
        int32_t clearedRowsCount = 0;

        for (int32_t y = minY; y <= maxY; y++) {
            if (y >= 0) {
                if (ttsIsRowFull(tetris, y)) {
                    ttsClearRow(tetris, y);
                    clearedRows[clearedRowsCount++] = y;
                }
            }
        }
        TTS_ASSERT(clearedRowsCount <= TTS_ARRAYCOUNT(clearedRows));

        for (int32_t rowIndex = 0; rowIndex < clearedRowsCount; rowIndex++) {
            int32_t clearedRow = clearedRows[rowIndex];
            for (int32_t y = clearedRow - 1; y >= 0; y--) {
                for (int32_t x = 0; x < TTS_COLUMN_COUNT; x++) {
                    tetris->grid[y + 1][x] = tetris->grid[y][x];
                }
            }
        }

        if (clearedRowsCount > 0) {
            ttsClearRow(tetris, 0);
        }

        tetris->playerType++;
        if (tetris->playerType >= TtsTetraminoType_Count) {
            tetris->playerType = TtsTetraminoType_None + 1;
        }

        spawnTetramino(tetris, tetris->playerType);
    }
}

static void ttsUpdate(TtsTetris *tetris, float secondsElapsed) {
    if (tetris->frame == 0) {
        platformPlayMusic(tetris, tetris->music);
        spawnTetramino(tetris, TtsTetraminoType_I);
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
            ttsMakeColor(0.0f, 0.0f, 0.0f, 255.0f),
            tetris->platform
        );
    }

    //Frame
    {
        TtsColor color = {102.0f, 102.0f, 102.0f, 255.0f};

        for (int32_t column = -1; column < TTS_COLUMN_COUNT + 1; column++) {
            int32_t row = -1;
            ttsDrawCell(
                tetris,
                row,
                column,
                color,
                cellSideInPixels,
                gridX,
                gridY
            );

            row = TTS_ROW_COUNT;
            ttsDrawCell(
                tetris,
                row,
                column,
                color,
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
                color,
                cellSideInPixels,
                gridX,
                gridY
            );

            column = TTS_COLUMN_COUNT;
            ttsDrawCell(
                tetris,
                row,
                column,
                color,
                cellSideInPixels,
                gridX,
                gridY
            );
        }
    }

    // Player
    {
        float verticalVelocity = 3.0f;
        if (tetris->controls[TtsControlType_Down].wasDown) {
            verticalVelocity *= 3.0f;
        }
        float horizontalVelocity = 3.0f;

        if (!tetris->wasResizing && !tetris->paused) {
            tetris->playerYProgression += verticalVelocity * secondsElapsed;

            bool leftPressed = tetris->controls[TtsControlType_Left].wasDown;
            bool rightPressed = tetris->controls[TtsControlType_Right].wasDown;

            TtsHorizontalDirection previousDirection = TtsHorizontalDirection_None;

            if (leftPressed && !rightPressed) {
                tetris->horizontalDirection = TtsHorizontalDirection_Left;
            } else if (rightPressed && !leftPressed) {
                tetris->horizontalDirection = TtsHorizontalDirection_Right;
            } else {
                tetris->horizontalDirection = TtsHorizontalDirection_None;
                tetris->playerXProgression = 0.0f;
            }

            if (tetris->horizontalDirection != previousDirection) {
                if (tetris->horizontalDirection == TtsHorizontalDirection_Left) {
                    tetris->playerXProgression -= 1.0f;
                }
                if (tetris->horizontalDirection == TtsHorizontalDirection_Right) {
                    tetris->playerXProgression += 1.0f;
                }
            } else {
                float xDelta =  horizontalVelocity * secondsElapsed;

                if (tetris->horizontalDirection == TtsHorizontalDirection_Left) {
                    tetris->playerXProgression -= xDelta;
                }
                if (tetris->horizontalDirection == TtsHorizontalDirection_Right) {
                    tetris->playerXProgression += xDelta;
                }
            }

            for (uint32_t rotationIndex = 0; rotationIndex < ttsPressCount(tetris->controls[TtsControlType_C]); rotationIndex++) {
                ttsRotatePlayer(tetris, -1);
            }

            for (uint32_t rotationIndex = 0; rotationIndex < ttsPressCount(tetris->controls[TtsControlType_Up]); rotationIndex++) {
                ttsRotatePlayer(tetris, +1);
            }

            if (ttsPressCount(tetris->controls[TtsControlType_Space]) > 0) {
                tetris->playerYProgression = (float)(TTS_ROW_COUNT + 4);
            }
        }

        while (tetris->playerYProgression > 1.0f) {
            ttsMoveVertically(tetris);
        }

        while (tetris->playerXProgression > 1.0f || tetris->playerXProgression < -1.0f) {
            tetris->horizontalDirection = TtsHorizontalDirection_None;
            bool moveRight = tetris->playerXProgression > 1.0f ;
            float increment = moveRight ? 1.0f : -1.0f;
            TtsTetramino playerCells = ttsGetPlayerCells(tetris);
            TtsTetramino nextCells = ttsOffsetCells(playerCells, moveRight ? 1 : -1, 0);

            if (ttsIsPositionAvailable(tetris, nextCells)) {
                tetris->playerXProgression -= increment;
                tetris->playerXInCells += increment;
            } else {
                tetris->playerXProgression = 0.0f;
            }
        }

        TtsTetramino playerCells = ttsGetPlayerCells(tetris);
        TtsColor color = ttsGetTetraminoColor(tetris->playerType);
        for (int32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(playerCells.cells); cellIndex++) {
            if (playerCells.cells[cellIndex].y >= 0) {
                ttsDrawCell(
                    tetris,
                    playerCells.cells[cellIndex].y,
                    playerCells.cells[cellIndex].x,
                    color,
                    cellSideInPixels,
                    gridX,
                    gridY
                );
            }
        }
    }

    // Grid
    {
        for (int32_t rowIndex = 0; rowIndex < TTS_ARRAYCOUNT(tetris->grid); rowIndex++) {
            for (int32_t columnIndex = 0; columnIndex < TTS_ARRAYCOUNT(tetris->grid[0]); columnIndex++) {
                if (!ttsIsCellAvailable(tetris, columnIndex, rowIndex)) {
                    TtsColor color = ttsGetTetraminoColor(tetris->grid[rowIndex][columnIndex]);

                    ttsDrawCell(
                        tetris,
                        rowIndex,
                        columnIndex,
                        color,
                        cellSideInPixels,
                        gridX,
                        gridY
                    );
                }
            }
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
                ttsMakeColor(223.0f, 240.0f, 216.0f, 255.0f)
            );

            TtsString next = TTS_MAKE_STRING("Next:");
            ttsDrawString(
                tetris,
                next,
                labelX,
                gridY,
                1.0f,
                ttsMakeColor(0.0f, 0.0f, 0.0f, 255.0f)
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
            ttsMakeColor(223.0f, 240.0f, 216.0f, 255.0f)
        ) ;

        TtsString score = TTS_MAKE_STRING("Score:");
        ttsDrawString(
            tetris,
            score,
            labelX,
            scoreY,
            1.0f,
            ttsMakeColor(0.0f, 0.0f, 0.0f, 255.0f)
        );

        TtsString points = TTS_MAKE_STRING("0.000.000.350");
        ttsDrawString(
            tetris,
            points,
            labelX,
            scoreY + tetris->atlas.lineHeightInPixels,
            1.0f,
            ttsMakeColor(0.0f, 0.0f, 0.0f, 255.0f)
        );
    }

    for (uint32_t controlIndex = 1; controlIndex < TTS_ARRAYCOUNT(tetris->controls); controlIndex++) {
        tetris->controls[controlIndex].wasDown = tetris->controls[controlIndex].endedDown;
        tetris->controls[controlIndex].endedDown = 0;
        tetris->controls[controlIndex].transitions = 0;
    }
    tetris->frame++;
}

static bool ttsWavIsValid(Wav wav) {
    bool result = wav.riffChunk && wav.fmtChunk && wav.data;

    return result;
}
