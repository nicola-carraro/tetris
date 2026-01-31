static uint32_t ttsGetRandomNumber(TtsTetris *tetris) {
    uint32_t result = (tetris->seed * 69069) + 1;
    tetris->seed = result;
    return result;
}

static TtsString ttsMakeString (char *text, uint64_t size) {
    TtsString result = {0};

    result.text = text;
    result.size = size;

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

    if (ok) {
        result.music = ttsWavParseFile(musicFile);
    }

    result.backgroundColor = ttsMakeColor(34.0f, 67.0f, 74.0f, 255.0f);

    char paths[TtsSoundEffect_Count][256] = {
        [TtsSoundEffect_Whoosh] = "../data/whoosh.wav",
        [TtsSoundEffect_Click] = "../data/click.wav",
    };

    for (TtsSoundEffect effect = TtsSoundEffect_None + 1; effect < TtsSoundEffect_Count; effect++) {
        TtsReadResult soundFile = {0};
        char *path = paths[effect];
        soundFile = platformReadEntireFile(path);
        ok = soundFile.size > 0;

        if (ok) {
            result.soundEffects[effect] = ttsWavParseFile(soundFile);
        }
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

static float ttsGetCellMargin(float cellSide) {
    float internalSide = cellSide * 0.7f;
    float result = (cellSide - internalSide) / 2.0f;

    return result;
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
    float margin = ttsGetCellMargin(cellSide);

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

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(pattern.cellCenters); cellIndex++) {
        TtsFloatCoords srcCoords = pattern.cellCenters[cellIndex];
        TtsFloatCoords rotatedCoords = srcCoords;
        if (rotation.flipCoordinates) {
            rotatedCoords.x = -srcCoords.y;
            rotatedCoords.y = srcCoords.x;
        }

        if (rotation.flipSign) {
            rotatedCoords.x = -rotatedCoords.x;
            rotatedCoords.y = -rotatedCoords.y;
        }
        result.cellCenters[cellIndex].x = rotatedCoords.x;
        result.cellCenters[cellIndex].y = rotatedCoords.y;
    }

    return result;
}

static TtsTetramino ttsGetTetraminoCells(TtsTetraminoType tetraminoType, TtsRotationType rotationType, float x, float y) {
    TtsTetraminoPattern rotatedCells = ttsGetRotatedCells(tetraminoType, rotationType);
    TtsTetramino result = {0};

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(rotatedCells.cellCenters); cellIndex++) {
        TtsFloatCoords cellCenter = rotatedCells.cellCenters[cellIndex];
        result.cells[cellIndex].x = (int32_t)(x + cellCenter.x - 0.5f);
        result.cells[cellIndex].y = (int32_t)(y + cellCenter.y - 0.5f);
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

static TtsTetraminoType getNextType(TtsTetris *tetris) {
    uint32_t random = ttsGetRandomNumber(tetris);
    TtsTetraminoType result = (TtsTetraminoType)((random % (TtsTetraminoType_Count - 1)) + 1);

    return result;
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
        TtsTetraminoType tetraminoType = tetris->grid[y][x];
        if (tetraminoType) {
            tetris->grid[y][x] = TtsTetraminoType_None;
        }
    }
}

static TtsPatternFeatures ttsGetPatternFeatures(TtsTetraminoType type) {
    TtsTetraminoPattern pattern = ttsGetTetraminoPattern(type);

    float minCenterX = 5.0f;
    float minCenterY = 5.0f;
    float maxCenterX = -5.0f;
    float maxCenterY = -5.0f;

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(pattern.cellCenters); cellIndex++) {
        TtsFloatCoords center = pattern.cellCenters[cellIndex];
        minCenterX = TTS_MIN(center.x, minCenterX);
        minCenterY = TTS_MIN(center.y, minCenterY);
        maxCenterX = TTS_MAX(center.x, maxCenterX);
        maxCenterY = TTS_MAX(center.y, maxCenterY);
    }

    TtsPatternFeatures result = {0};

    result.minX = minCenterX - 0.5f;
    result.minY = minCenterY - 0.5f;
    result.maxX = maxCenterX + 0.5f;
    result.maxY = maxCenterY + 0.5f;
    result.width = result.maxX - result.minX;
    result.height = result.maxY - result.minY;
    result.esteticCenter.x = result.minX + (result.width / 2.0f);
    result.esteticCenter.y = result.minY + (result.height / 2.0f);

    return result;
}

static float ttsGetSpawnX(TtsTetraminoType type) {
    TTS_ASSERT(type > TtsTetraminoType_None);
    TTS_ASSERT(type < TtsTetraminoType_Count);

    TtsPatternFeatures features = ttsGetPatternFeatures(type);

    float result = ((float)TTS_COLUMN_COUNT / 2.0f);

    if ((int32_t)features.width % 2 != 0) {
        result -= 0.5f;
    }

    return result;
}

static float ttsGetSpawnY(TtsTetraminoType type) {
    TtsPatternFeatures features = ttsGetPatternFeatures(type);

    float result = -features.maxY;

    return result;
}

static void spawnTetramino(TtsTetris *tetris) {
    if (tetris->nextPlayerType == TtsTetraminoType_None) {
        tetris->nextPlayerType = getNextType(tetris);
    }

    tetris->playerType = tetris->nextPlayerType;
    tetris->nextPlayerType = getNextType(tetris);
    tetris->playerXInCells = ttsGetSpawnX(tetris->playerType);
    tetris->playerYInCells = ttsGetSpawnY(tetris->playerType);
    tetris->playerRotationType = TtsRotationType_None + 1;
    tetris->horizontalDirection = TtsHorizontalDirection_None;
    tetris->playerXProgression = 0.0f;
    tetris->playerYProgression = 0.0f;
    tetris->isHardDropping = false;
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

        for (int32_t y = minY; y <= maxY; y++) {
            if (y >= 0) {
                if (ttsIsRowFull(tetris, y)) {
                    tetris->clearedRows[tetris->clearedRowsCount++] = y;
                }
            }
        }
        TTS_ASSERT(tetris->clearedRowsCount <= TTS_ARRAYCOUNT(tetris->clearedRows));

        uint32_t scoreIncrement = 0;
        switch (tetris->clearedRowsCount) {
            case 1: {
                scoreIncrement = 100;
            } break;
            case 2: {
                scoreIncrement = 300;
            } break;
            case 3: {
                scoreIncrement = 500;
            } break;
            case 4: {
                scoreIncrement = 800;
            } break;
        }

        if (tetris->clearedRowsCount > 0) {
            tetris->secondsToFadeEnd = TTS_FADE_SECONDS;
            platformPlaySound(tetris, tetris->soundEffects[TtsSoundEffect_Whoosh]);
        }

        tetris->score += scoreIncrement;
        tetris->clearedLines += tetris->clearedRowsCount;

        spawnTetramino(tetris);
    }
}

static TtsFloatCoords ttsCenterTetraminoInBox(TtsTetraminoType type, float x, float y, float width, float height, float cellSideInPixels) {
    TtsPatternFeatures features = ttsGetPatternFeatures(type);

    TtsFloatCoords result = {0};

    result.x = x + (width / 2.0f) - (features.esteticCenter.x * cellSideInPixels);
    result.y = y + (height / 2.0f) - (features.esteticCenter.y * cellSideInPixels);

    return result;
}

static void ttsDrawCenteredPattern(TtsTetris *tetris, TtsTetraminoType type, float boxX, float  boxY, float boxWidth,float  boxHeight,float  cellSideInPixels) {
    TtsFloatCoords offset = ttsCenterTetraminoInBox(type, boxX, boxY, boxWidth, boxHeight, cellSideInPixels);

    TtsColor color = ttsGetTetraminoColor(type);

    TtsTetraminoPattern pattern = ttsGetTetraminoPattern(type);

    for (uint32_t cellIndex = 0; cellIndex < TTS_ARRAYCOUNT(pattern.cellCenters); cellIndex++) {
        TtsFloatCoords center = pattern.cellCenters[cellIndex];

        float cellX = ((center.x - 0.5f) * cellSideInPixels) + offset.x;
        float cellY = ((center.y - 0.5f) * cellSideInPixels) + offset.y;

        ttsDrawCellLikeQuad(
            tetris,
            cellX, cellY,
            cellSideInPixels, cellSideInPixels,
            ttsGetCellMargin(cellSideInPixels),
            color
        );
    }
}

static void ttsDrawNextTetramino(TtsTetris * tetris, float boxX, float boxY, float boxWidth, float boxHeight, float cellSideInPixels) {
    ttsDrawCenteredPattern(
        tetris,
        tetris->nextPlayerType,
        boxX, boxY,
        boxWidth, boxHeight,
        cellSideInPixels
    );
}

static float ttsGetStringWidthInPixels(TtsAtlas font, TtsString string) {
    float result = 0.0f;

    for (uint32_t glyphIndex = 0; glyphIndex < string.size; glyphIndex++) {
        char c = string.text[glyphIndex];
        TtsGlyph glyph = font.glyphs[c - TTS_FIRST_CODEPOINT];

        if (glyphIndex == string.size - 1) {
            result += glyph.bitmapWidthInPixels;
        } else {
            result += glyph.advanceWidthInPixels;
        }
    }

    return result;
}

static TtsString ttsFormatNumber(uint32_t number, char *dest, uint32_t destSize) {
    TtsString result = {0};

    if (number == 0) {
        dest[destSize - 1] = '0';
        result.size = 1;
        result.text = dest + destSize - 1;
    } else {
        uint32_t remaining = number;
        uint32_t charIndex = destSize - 1;

        while (charIndex >= 0 && remaining > 0) {
            dest[charIndex] = '0' + (remaining % 10);
            remaining /= 10;

            if (remaining > 0) {
                charIndex--;
            }
        }

        result.text = dest + charIndex;
        result.size = destSize - charIndex;
    }

    return result;
}

static uint32_t ttsGetCurrentLevel(TtsTetris *tetris) {
    uint32_t result = (tetris->clearedLines / 10) + 1;

    return result;
}

static bool ttsIsFading(TtsTetris *tetris) {
    return tetris->secondsToFadeEnd > 0.0f;
}

static void ttsUpdate(TtsTetris *tetris, float secondsElapsed) {
    if (tetris->frame == 0) {
        platformPlayMusic(tetris, tetris->music);
        spawnTetramino(tetris);
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

    float gridMargin = 20.0f;

    float boxHeight = tetris->atlas.lineHeightInPixels * 2.0f;
    float boxWidth = boxHeight * 3.0f;

    bool drawLabels = (gridX + gridWidth + cellSideInPixels + gridMargin + boxWidth) <= tetris->windowWidth;

    float gridY = drawLabels ? ((float)tetris->windowHeight - gridHeight) / 2.0f : ((float)tetris->windowHeight - gridHeight) - cellSideInPixels;

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

        if (ttsPressCount(tetris->controls[TtsControlType_Space]) > 0) {
            tetris->isHardDropping = true;
            platformPlaySound(tetris, tetris->soundEffects[TtsSoundEffect_Click]);
        }

        if (tetris->isHardDropping) {
            verticalVelocity = 200.0f;
        } else if (tetris->controls[TtsControlType_Down].wasDown) {
            verticalVelocity *= 10.0f;
        }
        float horizontalVelocity = 3.0f;

        if (!tetris->wasResizing && !tetris->paused && !ttsIsFading(tetris)) {
            tetris->playerYProgression += verticalVelocity * secondsElapsed;

            bool leftPressed = tetris->controls[TtsControlType_Left].wasDown;
            bool rightPressed = tetris->controls[TtsControlType_Right].wasDown;

            TtsHorizontalDirection previousDirection = TtsHorizontalDirection_None;

            if (!tetris->isHardDropping) {
                if (leftPressed && !rightPressed) {
                    tetris->horizontalDirection = TtsHorizontalDirection_Left;
                } else if (rightPressed && !leftPressed) {
                    tetris->horizontalDirection = TtsHorizontalDirection_Right;
                } else {
                    tetris->horizontalDirection = TtsHorizontalDirection_None;
                    tetris->playerXProgression = 0.0f;
                }
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

    if (tetris->clearedRowsCount > 0 && tetris->secondsToFadeEnd <= 0.0f){
        for (int32_t rowIndex = 0; rowIndex < tetris->clearedRowsCount; rowIndex++) {
            int32_t clearedRow = tetris->clearedRows[rowIndex];
            ttsClearRow(tetris, clearedRow);
            for (int32_t y = clearedRow - 1; y >= 0; y--) {
                for (int32_t x = 0; x < TTS_COLUMN_COUNT; x++) {
                    tetris->grid[y + 1][x] = tetris->grid[y][x];
                }
            }
        }

        ttsClearRow(tetris, 0);

        tetris->clearedRowsCount = 0;

        platformPlaySound(tetris, tetris->soundEffects[TtsSoundEffect_Click]);
    }

    // Grid
    {
        for (int32_t rowIndex = 0; rowIndex < TTS_ARRAYCOUNT(tetris->grid); rowIndex++) {
            bool isClearedRow = false;

            for (int32_t clearedRowIndex = 0; clearedRowIndex < tetris->clearedRowsCount; clearedRowIndex++) {
                if (tetris->clearedRows[clearedRowIndex] == rowIndex) {
                    isClearedRow = true;
                    break;
                }
            }

            for (int32_t columnIndex = 0; columnIndex < TTS_ARRAYCOUNT(tetris->grid[0]); columnIndex++) {
                TtsTetraminoType cell = tetris->grid[rowIndex][columnIndex];

                if (!ttsIsCellAvailable(tetris, columnIndex, rowIndex)) {
                    TtsColor color = ttsGetTetraminoColor(cell);

                    if (isClearedRow) {
                        float fadeRatio = 1.0f - (tetris->secondsToFadeEnd / TTS_FADE_SECONDS);
                        float alphaRatio = fadeRatio * fadeRatio * fadeRatio;
                        float alpha = 255 - (alphaRatio * 255.0f);
                        color.a = alpha;
                    }

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

    if (ttsIsFading(tetris)) {
        tetris->secondsToFadeEnd -= secondsElapsed;
    } else {
        tetris->secondsToFadeEnd = 0.0f;
    }

    {
        if (drawLabels) {
            float rightBoxX = gridX + gridWidth + (gridMargin * 2.0f);
            float rightLabelX = gridX + gridWidth + (gridMargin * 3.0f);
            float leftBoxX = gridX - (gridMargin * 2.0f) - boxWidth;
            float leftLabelX = leftBoxX + gridMargin;
            float upperBoxY = gridY;
            float lowerBoxY = gridY + gridHeight - boxHeight;

            TtsColor boxColor = ttsMakeColor(223.0f, 240.0f, 216.0f, 255.0f);
            TtsColor fontColor = ttsMakeColor(0.0f, 0.0f, 0.0f, 255.0f);

            char buffer[256] = {0};

            {
                ttsDrawCellLikeQuad(
                    tetris,
                    leftBoxX, upperBoxY,
                    boxWidth, boxHeight,
                    5.0f,
                    boxColor
                );

                ttsDrawString(
                    tetris,
                    TTS_MAKE_STRING("Lines:"),
                    leftLabelX,
                    upperBoxY,
                    1.0f,
                    fontColor
                );

                ttsDrawString(
                    tetris,
                    ttsFormatNumber(tetris->clearedLines, buffer, TTS_ARRAYCOUNT(buffer)),
                    leftLabelX,
                    upperBoxY + tetris->atlas.lineHeightInPixels,
                    1.0f,
                    fontColor
                );
            }

            {
                ttsDrawCellLikeQuad(
                    tetris,
                    leftBoxX, lowerBoxY,
                    boxWidth, boxHeight,
                    5.0f,
                    boxColor
                );
            }

            {
                float boxMargin = 5.0f;
                ttsDrawCellLikeQuad(
                    tetris,
                    rightBoxX, upperBoxY,
                    boxWidth, boxHeight,
                    boxMargin,
                    boxColor
                );

                TtsString next = TTS_MAKE_STRING("Next:");

                ttsDrawString(
                    tetris,
                    next,
                    rightLabelX,
                    upperBoxY,
                    1.0f,
                    fontColor
                );

                {
                    float strWidth = ttsGetStringWidthInPixels(tetris->atlas, next);
                    float x = rightLabelX + strWidth;
                    float y = upperBoxY + boxMargin;
                    float width = boxWidth - (boxMargin * 2.0f) - gridMargin - strWidth;
                    float height = boxHeight - (boxMargin * 2.0f);
                    ttsDrawNextTetramino(tetris, x , y,  width,  height,  cellSideInPixels);
                }
            }

            {
                ttsDrawCellLikeQuad(
                    tetris,
                    leftBoxX, lowerBoxY,
                    boxWidth, boxHeight,
                    5.0f,
                    boxColor
                );
                ttsDrawString(
                    tetris,
                    TTS_MAKE_STRING("Level:"),
                    leftLabelX,
                    lowerBoxY,
                    1.0f,
                    fontColor
                );

                uint32_t level = ttsGetCurrentLevel(tetris);
                ttsDrawString(
                    tetris,
                    ttsFormatNumber(level, buffer, TTS_ARRAYCOUNT(buffer)),
                    leftLabelX,
                    lowerBoxY + tetris->atlas.lineHeightInPixels,
                    1.0f,
                    fontColor
                );
            }

            {
                ttsDrawCellLikeQuad(
                    tetris,
                    rightBoxX, lowerBoxY,
                    boxWidth, boxHeight,
                    5.0f,
                    boxColor
                ) ;

                ttsDrawString(
                    tetris,
                    TTS_MAKE_STRING("Score:"),
                    rightLabelX,
                    lowerBoxY,
                    1.0f,
                    fontColor
                );

                ttsDrawString(
                    tetris,
                    ttsFormatNumber(tetris->score, buffer, TTS_ARRAYCOUNT(buffer)),
                    rightLabelX,
                    lowerBoxY + tetris->atlas.lineHeightInPixels,
                    1.0f,
                    fontColor
                );
            }
        } else {
            ttsDrawNextTetramino(
                tetris,
                0.0f, 0.0f,
                (float) tetris->windowWidth, gridY - cellSideInPixels,
                cellSideInPixels
            );
        }
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
