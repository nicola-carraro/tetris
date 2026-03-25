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

static bool ttsArenaInit(
    TtsArena *arena, uint64_t size
) {
    bool result = false;

    void *buffer = platformAllocate(size);

    if (buffer) {
        arena->capacity = size;
        arena->buffer = buffer;
        result = true;
    }

    return result;
}

static void *ttsArenaPushSize(TtsArena *arena, uint64_t size) {
    uint64_t newUsed = arena->used + size;

    uint64_t alignement = 64;

    if (newUsed % alignement != 0) {
        newUsed = ((newUsed / alignement) + 1) * alignement;
    }

    TTS_ASSERT(newUsed <= arena->capacity);

    void *result = ((uint8_t *) arena->buffer) + arena->used;

    arena->used = newUsed;

    return result;
}

static TtsColor ttsMakeColor(float r, float g, float b, float a) {
    TtsColor result = {r, g, b, a};

    return result;
}

static TtsControl ttsGetControl(TtsTetris *tetris, TtsControlType controlType) {
    TTS_ASSERT(tetris);
    TTS_ASSERT(controlType > TtsControlType_None);
    TTS_ASSERT(controlType < TtsControlType_Count);

    TtsControl result = tetris->controls[controlType];

    return result;
}

static bool ttsControlPressed(TtsTetris *tetris, TtsControlType controlType) {
    bool result = ttsGetControl(tetris, controlType).pressCount > 0;

    return result;
}

static bool ttsControlReleased(TtsTetris *tetris, TtsControlType controlType) {
    bool result = ttsGetControl(tetris, controlType).releaseCount > 0;

    return result;
}

static bool ttsControlDown(TtsTetris *tetris, TtsControlType controlType) {
    bool result = ttsGetControl(tetris, controlType).isDown;

    return result;
}

static void ttsDrawGlyph(
    TtsTetris *tetris,
    TtsGlyph glyph,
    float x, float y,
    float scale,
    TtsColor color
) {
    float quadX = x + glyph.xOffsetInPixels * scale;
    float quadY = y + glyph.yOffsetInPixels * scale;
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

static bool ttsInit(TtsTetris *tetris, uint64_t platformSize) {
    bool result = false;

    if (ttsArenaInit(&tetris->arena, TTS_ALLOCATION_SIZE)) {
        result = true;

        tetris->platform = ttsArenaPushSize(&tetris->arena , platformSize);

        char musicPaths[TtsMusic_Count][256] = {
            [TtsMusic_Theme] = TTS_DATA_DIR "theme.wav",
            [TtsMusic_Celebrate] = TTS_DATA_DIR "celebrate.wav",
        };

        for (TtsMusic music = TtsMusic_None + 1; music < TtsMusic_Count; music++) {
            TtsReadResult soundFile = {0};
            char *path = musicPaths[music];
            soundFile = platformReadEntireFile(path, &tetris->arena);
            tetris->musics[music] = ttsWavParseFile(soundFile);
        }

        tetris->backgroundColor = ttsMakeColor(34.0f, 67.0f, 74.0f, 255.0f);

        char effectsPaths[TtsSoundEffect_Count][256] = {
            [TtsSoundEffect_Whoosh] = TTS_DATA_DIR "whoosh.wav",
            [TtsSoundEffect_Click] = TTS_DATA_DIR "click.wav",
            [TtsSoundEffect_GameOver] = TTS_DATA_DIR "game_over.wav",
            [TtsSoundEffect_Ding] = TTS_DATA_DIR "ding.wav",
            [TtsSoundEffect_Success] = TTS_DATA_DIR "success.wav",
            [TtsSoundEffect_Pluck] = TTS_DATA_DIR "pluck.wav",
            [TtsSoundEffect_Yay] = TTS_DATA_DIR "yay.wav",
            [TtsSoundEffect_LevelUp] = TTS_DATA_DIR "level-up.wav",
        };

        for (TtsSoundEffect effect = TtsSoundEffect_None + 1; effect < TtsSoundEffect_Count; effect++) {
            TtsReadResult soundFile = {0};
            char *path = effectsPaths[effect];
            soundFile = platformReadEntireFile(path, &tetris->arena);
            tetris->soundEffects[effect] = ttsWavParseFile(soundFile);
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

static uint32_t ttsGetCurrentLevel(TtsTetris *tetris) {
    uint32_t result = (tetris->clearedLines / TTS_LINES_PER_LEVEL) + 1;

    if (result >= TTS_LEVEL_COUNT_PLUS_ONE) {
        result = TTS_LEVEL_COUNT_PLUS_ONE - 1;
    }

    return result;
}

TtsColorScheme ttsGetColorScheme(TtsTetris *tetris) {
    uint32_t level = ttsGetCurrentLevel(tetris);

    TtsColorScheme schemes[] = {
        [1] = {
            {
                [TtsTetraminoType_I] = {0.0f, 255.0f, 255.0f, 255.0f},
                [TtsTetraminoType_O] = {255.0f, 255.0f, 0.0f, 255.0f},
                [TtsTetraminoType_T] = {180.0f, 0.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 140.0f, 0.0f, 255.0f},
                [TtsTetraminoType_J] = {0.0f, 120.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 0.0f, 80.0f, 255.0f},
                [TtsTetraminoType_S] = {0.0f, 255.0f, 120.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {255.0f, 64.0f, 64.0f, 255.0f},
                [TtsTetraminoType_O] = {64.0f, 255.0f, 64.0f, 255.0f},
                [TtsTetraminoType_T] = {64.0f, 128.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 200.0f, 64.0f, 255.0f},
                [TtsTetraminoType_J] = {200.0f, 64.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 64.0f, 160.0f, 255.0f},
                [TtsTetraminoType_S] = {64.0f, 255.0f, 200.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {0.0f, 200.0f, 255.0f, 255.0f},
                [TtsTetraminoType_O] = {255.0f, 180.0f, 0.0f, 255.0f},
                [TtsTetraminoType_T] = {255.0f, 0.0f, 200.0f, 255.0f},
                [TtsTetraminoType_L] = {120.0f, 255.0f, 0.0f, 255.0f},
                [TtsTetraminoType_J] = {0.0f, 100.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 50.0f, 0.0f, 255.0f},
                [TtsTetraminoType_S] = {0.0f, 255.0f, 150.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {255.0f, 0.0f, 0.0f, 255.0f},
                [TtsTetraminoType_O] = {0.0f, 255.0f, 0.0f, 255.0f},
                [TtsTetraminoType_T] = {0.0f, 0.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 255.0f, 0.0f, 255.0f},
                [TtsTetraminoType_J] = {255.0f, 0.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {0.0f, 255.0f, 255.0f, 255.0f},
                [TtsTetraminoType_S] = {255.0f, 140.0f, 0.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {255.0f, 120.0f, 120.0f, 255.0f},
                [TtsTetraminoType_O] = {120.0f, 255.0f, 120.0f, 255.0f},
                [TtsTetraminoType_T] = {120.0f, 120.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 200.0f, 120.0f, 255.0f},
                [TtsTetraminoType_J] = {200.0f, 120.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 120.0f, 200.0f, 255.0f},
                [TtsTetraminoType_S] = {120.0f, 255.0f, 200.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {0.0f, 255.0f, 200.0f, 255.0f},
                [TtsTetraminoType_O] = {255.0f, 220.0f, 0.0f, 255.0f},
                [TtsTetraminoType_T] = {180.0f, 0.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 100.0f, 0.0f, 255.0f},
                [TtsTetraminoType_J] = {0.0f, 140.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 0.0f, 120.0f, 255.0f},
                [TtsTetraminoType_S] = {0.0f, 255.0f, 80.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {173.0f, 255.0f, 47.0f, 255.0f},
                [TtsTetraminoType_O] = {255.0f, 105.0f, 180.0f, 255.0f},
                [TtsTetraminoType_T] = {65.0f, 105.0f, 225.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 165.0f, 0.0f, 255.0f},
                [TtsTetraminoType_J] = {138.0f, 43.0f, 226.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 69.0f, 0.0f, 255.0f},
                [TtsTetraminoType_S] = {0.0f, 255.0f, 180.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {255.0f, 80.0f, 0.0f, 255.0f},
                [TtsTetraminoType_O] = {0.0f, 200.0f, 255.0f, 255.0f},
                [TtsTetraminoType_T] = {200.0f, 0.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 255.0f, 80.0f, 255.0f},
                [TtsTetraminoType_J] = {0.0f, 255.0f, 120.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 0.0f, 0.0f, 255.0f},
                [TtsTetraminoType_S] = {80.0f, 160.0f, 255.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {0.0f, 255.0f, 255.0f, 255.0f},
                [TtsTetraminoType_O] = {255.0f, 200.0f, 0.0f, 255.0f},
                [TtsTetraminoType_T] = {255.0f, 0.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {0.0f, 255.0f, 120.0f, 255.0f},
                [TtsTetraminoType_J] = {255.0f, 80.0f, 80.0f, 255.0f},
                [TtsTetraminoType_Z] = {120.0f, 80.0f, 255.0f, 255.0f},
                [TtsTetraminoType_S] = {80.0f, 255.0f, 80.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {255.0f, 64.0f, 128.0f, 255.0f},
                [TtsTetraminoType_O] = {64.0f, 255.0f, 192.0f, 255.0f},
                [TtsTetraminoType_T] = {192.0f, 64.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 192.0f, 64.0f, 255.0f},
                [TtsTetraminoType_J] = {64.0f, 128.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 64.0f, 64.0f, 255.0f},
                [TtsTetraminoType_S] = {128.0f, 255.0f, 64.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {0.0f, 180.0f, 255.0f, 255.0f},
                [TtsTetraminoType_O] = {255.0f, 220.0f, 120.0f, 255.0f},
                [TtsTetraminoType_T] = {200.0f, 120.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 120.0f, 60.0f, 255.0f},
                [TtsTetraminoType_J] = {120.0f, 255.0f, 200.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 80.0f, 140.0f, 255.0f},
                [TtsTetraminoType_S] = {120.0f, 255.0f, 120.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {255.0f, 0.0f, 120.0f, 255.0f},
                [TtsTetraminoType_O] = {0.0f, 255.0f, 200.0f, 255.0f},
                [TtsTetraminoType_T] = {120.0f, 0.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 200.0f, 0.0f, 255.0f},
                [TtsTetraminoType_J] = {0.0f, 120.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 60.0f, 0.0f, 255.0f},
                [TtsTetraminoType_S] = {120.0f, 255.0f, 0.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {120.0f, 255.0f, 255.0f, 255.0f},
                [TtsTetraminoType_O] = {255.0f, 255.0f, 120.0f, 255.0f},
                [TtsTetraminoType_T] = {255.0f, 120.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {120.0f, 255.0f, 120.0f, 255.0f},
                [TtsTetraminoType_J] = {255.0f, 120.0f, 120.0f, 255.0f},
                [TtsTetraminoType_Z] = {120.0f, 120.0f, 255.0f, 255.0f},
                [TtsTetraminoType_S] = {255.0f, 180.0f, 120.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {0.0f, 255.0f, 100.0f, 255.0f},
                [TtsTetraminoType_O] = {255.0f, 200.0f, 0.0f, 255.0f},
                [TtsTetraminoType_T] = {200.0f, 0.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 80.0f, 80.0f, 255.0f},
                [TtsTetraminoType_J] = {0.0f, 150.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 0.0f, 0.0f, 255.0f},
                [TtsTetraminoType_S] = {120.0f, 255.0f, 0.0f, 255.0f},
            }
        },
        {
            {
                [TtsTetraminoType_I] = {255.0f, 50.0f, 200.0f, 255.0f},
                [TtsTetraminoType_O] = {50.0f, 255.0f, 200.0f, 255.0f},
                [TtsTetraminoType_T] = {200.0f, 50.0f, 255.0f, 255.0f},
                [TtsTetraminoType_L] = {255.0f, 200.0f, 50.0f, 255.0f},
                [TtsTetraminoType_J] = {50.0f, 150.0f, 255.0f, 255.0f},
                [TtsTetraminoType_Z] = {255.0f, 50.0f, 50.0f, 255.0f},
                [TtsTetraminoType_S] = {50.0f, 255.0f, 50.0f, 255.0f},
            }
        }
    };

    static_assert(TTS_ARRAYCOUNT(schemes) == TTS_LEVEL_COUNT_PLUS_ONE);

    TTS_ASSERT(level < TTS_LEVEL_COUNT_PLUS_ONE);

    TtsColorScheme result = schemes[level];

    return result;
}

TtsColor ttsGetTetraminoColor(TtsTetris *tetris, TtsTetraminoType tetraminoType) {
    TTS_ASSERT(tetraminoType > TtsTetraminoType_None);
    TTS_ASSERT(tetraminoType < TtsTetraminoType_Count);

    TtsColorScheme scheme = ttsGetColorScheme(tetris);

    TtsColor result = scheme.colors[tetraminoType];

    return result;
}

TtsString ttsGetButtonLabel(TtsTetris *tetris, TtsButtonType buttonType) {
    TTS_ASSERT(buttonType > TtsButtonType_None);
    TTS_ASSERT(buttonType < TtsButtonType_Count);

    TtsString result = {0};

    switch (buttonType) {
        case TtsButtonType_New: {
            result = TTS_MAKE_STRING("New");
        } break;
        case TtsButtonType_Resume: {
            result = TTS_MAKE_STRING("Resume");
        } break;
        case TtsButtonType_Sound: {
            result = tetris->effectsOff ? TTS_MAKE_STRING("Sound on") : TTS_MAKE_STRING("Sound off");
        } break;
        case TtsButtonType_Music: {
            result = tetris->musicOff ? TTS_MAKE_STRING("Music on") : TTS_MAKE_STRING("Music off");
        } break;
        case TtsButtonType_Quit: {
            result =  TTS_MAKE_STRING("Quit");
        } break;
    }

    return result;
}

TtsTetraminoType ttsGetButtonTetraminoType (TtsButtonType buttonType) {
    TTS_ASSERT(buttonType > TtsButtonType_None);
    TTS_ASSERT(buttonType < TtsButtonType_Count);

    TtsTetraminoType types[TtsButtonType_Count] = {
        [TtsButtonType_New] = TtsTetraminoType_I,
        [TtsButtonType_Resume] = TtsTetraminoType_O,
        [TtsButtonType_Sound] = TtsTetraminoType_L,
        [TtsButtonType_Music] = TtsTetraminoType_S,
        [TtsButtonType_Quit] = TtsTetraminoType_Z,
    };

    TtsTetraminoType result = types[buttonType];

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

static bool ttsHasWon(TtsTetris *tetris) {
    bool result = (tetris->clearedLines / TTS_LINES_PER_LEVEL) + 1 >= TTS_LEVEL_COUNT_PLUS_ONE;

    return result;
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

static void ttsStartMusic(TtsTetris *tetris, TtsMusic music) {
    TTS_ASSERT(music > TtsMusic_None);
    TTS_ASSERT(music < TtsMusic_Count);

    platformPlaySound(tetris, tetris->musics[music], TtsSoundType_Music);
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
    tetris->isSoftDropping = false;
    tetris->isHardDropping = false;
}

static void ttsPlaySoundEffect(TtsTetris *tetris, TtsSoundEffect soundEffect) {
    TTS_ASSERT(soundEffect > TtsSoundEffect_None);
    TTS_ASSERT(soundEffect < TtsSoundEffect_Count);

    if (!tetris->effectsOff) {
        platformPlaySound(tetris, tetris->soundEffects[soundEffect], TtsSoundType_Effect);
    }
}
static void ttsAddClearedRows(TtsTetris *tetris, uint32_t rowsCount) {
    uint32_t scoreIncrement = 0;
    switch (rowsCount) {
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
        ttsPlaySoundEffect(tetris, TtsSoundEffect_Whoosh);
    }

    tetris->score += scoreIncrement;
    uint32_t previousLevel = ttsGetCurrentLevel(tetris);
    tetris->clearedLines += rowsCount;
    uint32_t currentLevel = ttsGetCurrentLevel(tetris);

    if (currentLevel > previousLevel) {
        ttsPlaySoundEffect(tetris, TtsSoundEffect_LevelUp);
    }

    if (ttsHasWon(tetris)) {
        ttsStartMusic(tetris, TtsMusic_Celebrate);
    }
}

static void ttsMoveVertically(TtsTetris *tetris) {
    TtsTetramino playerCells = ttsGetPlayerCells(tetris);
    TtsTetramino cellsBelow = ttsOffsetCells(playerCells, 0, 1);
    if (ttsIsPositionAvailable(tetris, cellsBelow)) {
        tetris->playerYProgression -= 1.0f;
        tetris->playerYInCells += 1.0f;
        if (tetris->isHardDropping) {
            tetris->score += 2;
        } else if (tetris->isSoftDropping) {
            tetris->score += 1;
        }
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

        if (minY < 0) {
            tetris->gameOver = true;
            platformPauseSound(tetris, TtsSoundType_Music);
        }

        for (int32_t y = minY; y <= maxY; y++) {
            if (y >= 0) {
                if (ttsIsRowFull(tetris, y)) {
                    tetris->clearedRows[tetris->clearedRowsCount++] = y;
                }
            }
        }
        TTS_ASSERT(tetris->clearedRowsCount <= TTS_ARRAYCOUNT(tetris->clearedRows));

        ttsAddClearedRows(tetris, tetris->clearedRowsCount);

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

    TtsColor color = ttsGetTetraminoColor(tetris, type);

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

static bool ttsIsFading(TtsTetris *tetris) {
    return tetris->secondsToFadeEnd > 0.0f;
}

static bool ttsIsFalling(TtsTetris *tetris) {
    return tetris->clearedRowsCount > 0 && !ttsIsFading(tetris);
}

static bool shouldUpdate(TtsTetris *tetris) {
    return !tetris->wasResizing && !tetris->menuOpen && !tetris->paused && tetris->clearedRowsCount == 0 && !tetris->gameOver && !ttsHasWon(tetris);
}

static void ttsCloseMenu(TtsTetris *tetris) {
    tetris->menuOpen = false;
    tetris->paused = false;
    tetris->pressedButton = TtsButtonType_None;
    tetris->hoveredButton = TtsButtonType_None;
}

static void ttsOpenMenu(TtsTetris *tetris) {
    tetris->menuOpen = true;
    tetris->hoveredButton = TtsButtonType_None + 1;
}

static void ttsNewGame(TtsTetris *tetris) {
    platformMemset(tetris->grid, 0, sizeof(tetris->grid));
    tetris->score = 0;
    tetris->clearedLines = 0;
    tetris->isHardDropping = false;
    tetris->isSoftDropping = false;
    tetris->secondsToFadeEnd = 0;
    tetris->clearedRowsCount = 0;
    tetris->gameOver = false;
    tetris->gameOverAnimationSteps = 0;
    tetris->secondsToNextGameOverAnimation = 0.0f;
    tetris->secondsToOpenMenu = 0.0f;

    if (!tetris->musicOff) {
        ttsStartMusic(tetris, TtsMusic_Theme);
    }
    ttsCloseMenu(tetris);
}

void ttsResumeGame(TtsTetris *tetris) {
    if (tetris->gameOver || ttsHasWon(tetris)) {
        ttsNewGame(tetris);
    } else {ttsCloseMenu(tetris);}
}

static float ttsGetVelocityMultiplier(TtsTetris *tetris) {
    float result = 1.0f;

    uint32_t currentLevel = ttsGetCurrentLevel(tetris);

    for (uint32_t level = 1; level < currentLevel; level++) {
        result *= 1.2f;
    }

    return result;
}

void ttsDrawLabel(TtsTetris *tetris, float x, float y, float width, float height, float margin, TtsString label, TtsColor backgroundColor, TtsColor fontColor) {
    ttsDrawCellLikeQuad(
        tetris,
        x, y,
        width, height,
        5.0f,
        backgroundColor
    );

    float labelX = x + margin;

    ttsDrawString(
        tetris,
        label,
        labelX,
        y,
        1.0f,
        fontColor
    );
}

void ttsDrawNumberLabel(TtsTetris *tetris, float x, float y, float width, float height, float margin, TtsString label, TtsColor backgroundColor, TtsColor fontColor, uint32_t number) {
    {
        ttsDrawLabel(tetris, x,  y,  width,  height,  margin,  label,  backgroundColor,  fontColor);
        char buffer[256] = {0};

        float labelX = x + margin;

        ttsDrawString(
            tetris,
            ttsFormatNumber(number, buffer, TTS_ARRAYCOUNT(buffer)),
            labelX,
            y + tetris->atlas.lineHeightInPixels,
            1.0f,
            fontColor
        );
    }
}

void ttsDrawNextTetraminoLabel(TtsTetris *tetris, float x, float y, float width, float height, float margin,  TtsColor backgroundColor, TtsColor fontColor, float cellSideInPixels) {
    {
        float boxMargin = 5.0f;

        TtsString next = TTS_MAKE_STRING("Next:");

        ttsDrawLabel(
            tetris, x, y, width, height, margin,
            next,
            backgroundColor, fontColor
        );

        {
            float strWidth = ttsGetStringWidthInPixels(tetris->atlas, next);
            float tetraminoX = x + margin + strWidth;
            float tetraminoY = y + boxMargin;
            float tetraminoWidth = width - (boxMargin * 2.0f) - margin - strWidth;
            float tetraminoHeight = height - (boxMargin * 2.0f);
            ttsDrawNextTetramino(tetris, tetraminoX , tetraminoY,  tetraminoWidth,  tetraminoHeight,  cellSideInPixels);
        }
    }
}

bool ttsPointInQuad(float x, float y, float top, float left, float width, float height) {
    float right = left + width;
    float bottom = top + height;

    bool result = x >= left
    && x <= right
    && y >= top
    && y <= bottom;

    return result;
}

static void ttsUpdate(TtsTetris *tetris, float secondsElapsed) {
    if (tetris->frame == 0) {
        spawnTetramino(tetris);
        ttsStartMusic(tetris, TtsMusic_Theme);
    }

    if (!tetris->menuOpen && ttsControlPressed(tetris, TtsControlType_P)) {
        tetris->paused = !tetris->paused;
    }

    if (ttsControlPressed(tetris, TtsControlType_Esc)) {
        if (tetris->menuOpen) {
            ttsResumeGame(tetris);
        } else {
            ttsOpenMenu(tetris);
        }
    }

    #ifdef TTS_ENABLE_CHEAT
    uint32_t currentLevel =  ttsGetCurrentLevel(tetris);
    if (ttsControlPressed(tetris, TtsControlType_L) && !ttsHasWon(tetris)) {
        ttsAddClearedRows(tetris,  (currentLevel * TTS_LINES_PER_LEVEL) - tetris->clearedLines);
    }
    #endif

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

    TtsColor frameColor = {102.0f, 102.0f, 102.0f, 255.0f};

    //Frame
    {
        for (int32_t column = -1; column < TTS_COLUMN_COUNT + 1; column++) {
            int32_t row = -1;
            ttsDrawCell(
                tetris,
                row,
                column,
                frameColor,
                cellSideInPixels,
                gridX,
                gridY
            );

            row = TTS_ROW_COUNT;
            ttsDrawCell(
                tetris,
                row,
                column,
                frameColor,
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
                frameColor,
                cellSideInPixels,
                gridX,
                gridY
            );

            column = TTS_COLUMN_COUNT;
            ttsDrawCell(
                tetris,
                row,
                column,
                frameColor,
                cellSideInPixels,
                gridX,
                gridY
            );
        }
    }

    // Player
    {
        if (shouldUpdate(tetris)) {
            float velocityMultiplier = ttsGetVelocityMultiplier(tetris);
            float verticalVelocity = 3.0f * velocityMultiplier;

            if (ttsControlPressed(tetris, TtsControlType_Space)) {
                tetris->isHardDropping = true;
                tetris->isSoftDropping = false;
                ttsPlaySoundEffect(tetris, TtsSoundEffect_Click);
            }

            if (!tetris->isHardDropping && ttsControlPressed(tetris, TtsControlType_Down)) {
                tetris->isSoftDropping = true;
            }

            if (!ttsControlDown(tetris, TtsControlType_Down)) {
                tetris->isSoftDropping = false;
            }

            if (tetris->isHardDropping) {
                verticalVelocity = 200.0f;
            } else if (tetris->isSoftDropping) {
                verticalVelocity *= 10.0f;
            }
            float horizontalVelocity = 8.0f;
            tetris->playerYProgression += verticalVelocity * secondsElapsed;

            // bool leftPressed = ttsControlPressed(tetris, TtsControlType_Left);
            // bool rightPressed = ttsControlPressed(tetris, TtsControlType_Right);
            bool leftDown = ttsControlDown(tetris, TtsControlType_Left);
            bool rightDown = ttsControlDown(tetris, TtsControlType_Right);

            TtsHorizontalDirection previousDirection = tetris->horizontalDirection;

            if (!tetris->isHardDropping) {
                if (leftDown && !rightDown) {
                    tetris->horizontalDirection = TtsHorizontalDirection_Left;
                } else if (rightDown && !leftDown) {
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

            for (uint32_t pressIndex = 0; pressIndex < tetris->controls[TtsControlType_C].pressCount; pressIndex++) {
                ttsRotatePlayer(tetris, -1);
            }

            for (uint32_t pressIndex = 0; pressIndex < tetris->controls[TtsControlType_Up].pressCount; pressIndex++) {
                ttsRotatePlayer(tetris, +1);
            }
        }

        while (tetris->playerYProgression > 1.0f) {
            ttsMoveVertically(tetris);
        }

        while (tetris->playerXProgression >= 1.0f || tetris->playerXProgression <= -1.0f) {
            bool moveRight = tetris->playerXProgression > 0.0f ;
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
        TtsColor color = ttsGetTetraminoColor(tetris, tetris->playerType);
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

    bool wasFading = ttsIsFading(tetris);
    if (ttsIsFading(tetris)) {
        tetris->secondsToFadeEnd -= secondsElapsed;
    } else {
        tetris->secondsToFadeEnd = 0.0f;
    }
    bool isFading = ttsIsFading(tetris);

    if (wasFading && !isFading) {
        for (int32_t rowIndex = 0; rowIndex < tetris->clearedRowsCount; rowIndex++) {
            int32_t clearedRow = tetris->clearedRows[rowIndex];
            ttsClearRow(tetris, clearedRow);
        }
        ttsPlaySoundEffect(tetris, TtsSoundEffect_Click);
    }

    if (ttsIsFalling(tetris)){
        float fallingVelocity = 20.0f;
        tetris->fallingYProgression += (fallingVelocity * secondsElapsed);

        while (tetris->fallingYProgression >= 1.0f && tetris->clearedRowsCount > 0) {
            int32_t clearedRow = tetris->clearedRows[tetris->clearedRowsCount - 1];

            for (int32_t y = clearedRow - 1; y >= 0; y--) {
                for (int32_t x = 0; x < TTS_COLUMN_COUNT; x++) {
                    tetris->grid[y + 1][x] = tetris->grid[y][x];
                }
            }

            ttsClearRow(tetris, 0);

            tetris->clearedRowsCount--;
            for (int32_t rowIndex = 0; rowIndex < tetris->clearedRowsCount; rowIndex++) {
                tetris->clearedRows[rowIndex]++;
            }
            tetris->fallingYProgression -= 1.0f;
        }
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
                    TtsColor color = ttsGetTetraminoColor(tetris, cell);

                    if (isClearedRow && ttsIsFading(tetris)) {
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

    if (tetris->gameOver) {
        if (tetris->gameOverAnimationSteps < TTS_ROW_COUNT) {
            float secondsForGameOverRow = 0.1f;
            if (tetris->secondsToNextGameOverAnimation <= 0.0f) {
                tetris->gameOverAnimationSteps++;
                tetris->secondsToNextGameOverAnimation += secondsForGameOverRow;
                TtsSoundEffect soundEffect  = TtsSoundEffect_Click;

                if (tetris->gameOverAnimationSteps >= TTS_ROW_COUNT - 1){
                    soundEffect = TtsSoundEffect_GameOver;
                    tetris->secondsToOpenMenu = 1.5f;
                }

                ttsPlaySoundEffect(tetris, soundEffect);
            }
            tetris->secondsToNextGameOverAnimation -= secondsElapsed;
        }

        for (uint32_t gameOverRowIndex = 0; gameOverRowIndex <tetris->gameOverAnimationSteps; gameOverRowIndex++) {
            for (uint32_t column = 0; column < TTS_COLUMN_COUNT; column++) {
                uint32_t row = TTS_ROW_COUNT - 1 - gameOverRowIndex;
                ttsDrawCell(
                    tetris,
                    row,
                    column,
                    frameColor,
                    cellSideInPixels,
                    gridX,
                    gridY
                );
            }
        }
    }

    if (ttsHasWon(tetris)) {
        uint32_t cellCount = TTS_ROW_COUNT * TTS_COLUMN_COUNT;

        float secondsForGameOverStep = 0.05f;
        if (tetris->secondsToNextGameOverAnimation <= 0.0f) {
            if (tetris->gameOverAnimationSteps < cellCount) {
                tetris->gameOverAnimationSteps++;
                if (tetris->gameOverAnimationSteps >= cellCount){
                    tetris->secondsToOpenMenu = 1.5f;
                    platformPauseSound(tetris, TtsSoundType_Music);
                    ttsPlaySoundEffect(tetris, TtsSoundEffect_Yay);
                }
            }
            tetris->secondsToNextGameOverAnimation += secondsForGameOverStep;

            for (uint32_t gameOverCellIndex = 0; gameOverCellIndex <tetris->gameOverAnimationSteps; gameOverCellIndex++) {
                TtsTetraminoType type = getNextType(tetris);

                uint32_t gameOverRowIndex = gameOverCellIndex / TTS_COLUMN_COUNT;
                uint32_t gameOverColumnIndex = gameOverCellIndex % TTS_COLUMN_COUNT;

                tetris->grid[TTS_ROW_COUNT - gameOverRowIndex - 1][gameOverColumnIndex] = type;
            }
        }
        tetris->secondsToNextGameOverAnimation -= secondsElapsed;
    }

    if (tetris->secondsToOpenMenu > 0.0f) {
        tetris->secondsToOpenMenu -= secondsElapsed;
    }

    if (tetris->secondsToOpenMenu < 0.0f) {
        ttsOpenMenu(tetris);
        tetris->secondsToOpenMenu = 0.0f;
        if (!tetris->musicOff) {
            platformResumeSound(tetris, TtsSoundType_Music);
        }
    }

    TtsColor boxColor = ttsMakeColor(223.0f, 240.0f, 216.0f, 255.0f);
    TtsColor fontColor = ttsMakeColor(0.0f, 0.0f, 0.0f, 255.0f);

    {
        if (drawLabels) {
            float rightBoxX = gridX + gridWidth + (gridMargin * 2.0f);
            float leftBoxX = gridX - (gridMargin * 2.0f) - boxWidth;
            float upperBoxY = gridY;
            float lowerBoxY = gridY + gridHeight - boxHeight;

            ttsDrawNumberLabel(
                tetris, leftBoxX, upperBoxY, boxWidth, boxHeight, gridMargin,
                TTS_MAKE_STRING("Lines:"),
                boxColor, fontColor,
                tetris->clearedLines
            );

            ttsDrawNextTetraminoLabel(
                tetris, rightBoxX, upperBoxY, boxWidth, boxHeight, gridMargin,
                boxColor,  fontColor,
                cellSideInPixels
            );

            ttsDrawNumberLabel(
                tetris, leftBoxX, lowerBoxY, boxWidth, boxHeight, gridMargin,
                TTS_MAKE_STRING("Level:"),
                boxColor, fontColor,
                ttsGetCurrentLevel(tetris)
            );

            ttsDrawNumberLabel(
                tetris, rightBoxX, lowerBoxY, boxWidth, boxHeight, gridMargin,
                TTS_MAKE_STRING("Score:"),
                boxColor, fontColor,
                tetris->score
            );
        } else {
            ttsDrawNextTetramino(
                tetris,
                0.0f, 0.0f,
                (float) tetris->windowWidth, gridY - cellSideInPixels,
                cellSideInPixels
            );
        }
    }

    if (tetris->menuOpen) {
        float menuWidth = gridWidth * 1.2f;
        float menuLeft = (tetris->windowWidth - menuWidth) / 2.0f;
        float menuHeight = gridHeight * 1.2f;
        float menuTop = (tetris->windowHeight - menuHeight) / 2.0f;

        ttsDrawCellLikeQuad(
            tetris,
            menuLeft, menuTop,
            menuWidth, menuHeight,
            5.0f,
            boxColor
        ) ;

        float buttonWidth = menuWidth * 0.8f;
        float buttonMargin = (menuWidth - buttonWidth) / 2.0f;
        float buttonHeight = buttonWidth * 0.4f;

        uint32_t buttonCount = TtsButtonType_Count - 1;
        float buttonsGap = (menuHeight - (2.0f * buttonMargin) - ((float)buttonCount * buttonHeight)) / ((float) (buttonCount - 1));

        float buttonLeft = menuLeft + buttonMargin;
        float buttonTop = menuTop + buttonMargin;

        bool mouseDown = ttsControlDown(tetris, TtsControlType_MouseLeft);
        bool enterPressed = ttsControlPressed(tetris, TtsControlType_Enter);
        bool mousePressed = ttsControlPressed(tetris, TtsControlType_MouseLeft);
        bool mouseReleased = ttsControlReleased(tetris, TtsControlType_MouseLeft);

        if (tetris->hoveredButton > TtsButtonType_None + 1 && ttsControlPressed(tetris, TtsControlType_Up)) {
            tetris->hoveredButton--;
        }

        if (tetris->hoveredButton < TtsButtonType_Count - 1 && ttsControlPressed(tetris, TtsControlType_Down)) {
            tetris->hoveredButton++;
        }

        for (TtsButtonType buttonType = TtsButtonType_None + 1; buttonType < TtsButtonType_Count; buttonType++) {
            TtsString label = ttsGetButtonLabel(tetris, buttonType);
            TtsTetraminoType tetraminoType = ttsGetButtonTetraminoType(buttonType);
            TtsColor buttonColor = ttsGetTetraminoColor(tetris, tetraminoType);

            float mouseX = (float) tetris->mouseX;
            float mouseY = (float) tetris->mouseY;
            float previousMouseX = (float) tetris->previousMouseX;
            float previousMouseY = (float) tetris->previousMouseY;

            bool isMouseOver = ttsPointInQuad(mouseX, mouseY, buttonTop, buttonLeft, buttonWidth, buttonHeight);
            bool wasMouseOver = ttsPointInQuad(previousMouseX, previousMouseY, buttonTop, buttonLeft, buttonWidth, buttonHeight);

            bool mouseEntered = isMouseOver && !wasMouseOver;

            if (mouseEntered) {
                tetris->hoveredButton = buttonType;
            }

            if (tetris->hoveredButton == buttonType && mousePressed) {
                tetris->pressedButton = buttonType;
            }

            float buttonPadding = 0.0f;

            if (tetris->pressedButton == buttonType) {
                float releaseThreshold = 5.0f;
                if (!ttsPointInQuad(mouseX, mouseY, buttonTop - releaseThreshold, buttonLeft - releaseThreshold, buttonWidth + (2.0f * releaseThreshold), buttonHeight * (2.0f * releaseThreshold))) {
                    tetris->pressedButton = TtsButtonType_None;
                }

                buttonPadding = 3.0f;
            } else {
                buttonPadding = 5.0f;
            }

            bool doAction = (tetris->pressedButton == buttonType && mouseReleased) || (tetris->hoveredButton == buttonType && enterPressed);

            if (doAction) {
                ttsPlaySoundEffect(tetris, TtsSoundEffect_Click);

                switch (buttonType) {
                    case TtsButtonType_Resume: {
                        ttsResumeGame(tetris);
                    } break;
                    case TtsButtonType_New: {
                        ttsNewGame(tetris);
                    } break;
                    case TtsButtonType_Music: {
                        if (tetris->musicOff) {
                            platformResumeSound(tetris, TtsSoundType_Music);
                            tetris->musicOff= false;
                        } else {
                            platformPauseSound(tetris, TtsSoundType_Music);
                            tetris->musicOff= true;
                        }
                    } break;
                    case TtsButtonType_Sound: {
                        tetris->effectsOff = !tetris->effectsOff;
                    } break;
                    case TtsButtonType_Quit: {
                        tetris->shouldQuit = true;
                    } break;
                }
            }

            ttsDrawCellLikeQuad(
                tetris,
                buttonLeft, buttonTop,
                buttonWidth, buttonHeight,
                buttonPadding,
                buttonColor
            ) ;

            float maxLineHeight = buttonHeight * 0.75f;

            float fontScale = 1.0f;

            float lineHeight = tetris->atlas.lineHeightInPixels;

            if (maxLineHeight < tetris->atlas.lineHeightInPixels) {
                fontScale = maxLineHeight / tetris->atlas.lineHeightInPixels;
                lineHeight = maxLineHeight;
            }

            TtsColor labelColor = fontColor;

            if (tetris->hoveredButton == buttonType) {
                labelColor = ttsMakeColor(100.0f, 100.0f, 100.0f, 255.0f);
            }

            ttsDrawString(
                tetris,
                label,
                buttonLeft + (buttonWidth / 20.0f),
                buttonTop + ((buttonHeight - lineHeight) / 2.0f),
                fontScale,
                labelColor
            );
            buttonTop += (buttonHeight + buttonsGap);
        }

        if (!mouseDown) {
            tetris->pressedButton = TtsButtonType_None;
        }
    }

    for (uint32_t controlIndex = 1; controlIndex < TTS_ARRAYCOUNT(tetris->controls); controlIndex++) {
        tetris->controls[controlIndex].pressCount = 0;
        tetris->controls[controlIndex].releaseCount = 0;
    }
    tetris->frame++;
    tetris->previousMouseX = tetris->mouseX;
    tetris->previousMouseY = tetris->mouseY;
}

static bool ttsWavIsValid(Wav wav) {
    bool result = wav.riffChunk && wav.fmtChunk && wav.data;

    return result;
}
