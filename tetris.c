static uint32_t tetrisGetRandomNumber(Tetris *tetris) {
    uint32_t result = (tetris->seed * 69069) + 1;
    tetris->seed = result;
    return result;
}

static PlatformControl tetrisGetControl(PlatformInput input, PlatformControlType controlType) {
    BASE_ASSERT(controlType > PlatformControlType_None);
    BASE_ASSERT(controlType < PlatformControlType_Count);

    PlatformControl result = input.controls[controlType];

    return result;
}

static bool tetrisControlPressed(PlatformInput input, PlatformControlType controlType) {
    bool result = tetrisGetControl(input, controlType).pressCount > 0;

    return result;
}

static bool tetrisControlReleased(PlatformInput input, PlatformControlType controlType) {
    bool result = tetrisGetControl(input, controlType).releaseCount > 0;

    return result;
}

static bool tetrisControlDown(PlatformInput input, PlatformControlType controlType) {
    bool result = tetrisGetControl(input, controlType).isDown;

    return result;
}

static void tetrisDrawGlyph(
    Tetris *tetris,
    Platform *platform,
    TetrisGlyph glyph,
    float x, float y,
    float scale,
    BaseColor color
) {
    float quadX = x + glyph.xOffsetInPixels * scale;
    float quadY = y + glyph.yOffsetInPixels * scale;
    float quadWidth = glyph.bitmapWidthInPixels * scale;
    float quadHeight = glyph.bitmapHeightInPixels * scale;
    platformDrawTextureQuad(
        platform,
        quadX, quadY,
        quadWidth, quadHeight,
        glyph.bitmapXInPixels, glyph.bitmapYInPixels,
        glyph.bitmapWidthInPixels, glyph.bitmapHeightInPixels,
        tetris->atlas.width, tetris->atlas.height,
        color.r, color.g, color.b, color.a
    );
}

static void tetrisDrawString(
    Tetris *tetris,
    Platform *platform,
    BaseString string,
    float x, float y,
    float scale,
    BaseColor color
) {
    for (char codepointIndex = 0; codepointIndex < string.size; codepointIndex++) {
        char codepoint = string.text[codepointIndex];
        BASE_ASSERT(codepoint >= TETRIS_FIRST_CODEPOINT);
        BASE_ASSERT(codepoint <= TETRIS_LAST_CODEPOINT);

        uint32_t index = codepoint - TETRIS_FIRST_CODEPOINT;
        BASE_ASSERT(index < BASE_ARRAYCOUNT(tetris->atlas.glyphs));

        TetrisGlyph glyph = tetris->atlas.glyphs[index];
        tetrisDrawGlyph(
            tetris,
            platform,
            glyph,
            x,  y,
            scale,
            color
        );

        x += glyph.advanceWidthInPixels * scale;
    }
}

static bool tetrisInit(Tetris *tetris, uint64_t platformSize, uint32_t seed, Platform **platform, PlatformTexture *texture) {
    bool ok = false;

    if (baseArenaInit(&tetris->arena, TETRIS_ALLOCATION_SIZE)) {
        BaseReadResult file = {0};
        ok = platformReadEntireFile(TETRIS_ATLAS_PATH, &tetris->arena, &file);

        if (ok && file.data) {
            TetrisAtlas *atlas = (TetrisAtlas *)file.data;
            tetris->atlas = *atlas;
            tetris->seed = seed;
            texture->data = (uint8_t *)file.data + sizeof(TetrisAtlas);
            texture->width = (uint32_t)atlas->width;
            texture->height = (uint32_t)atlas->height;

            *platform = baseArenaPushSize(&tetris->arena , platformSize);

            char musicPaths[TetrisMusic_Count][256] = {
                [TetrisMusic_Theme] = TETRIS_DATA_DIR "theme.wav",
                [TetrisMusic_Celebrate] = TETRIS_DATA_DIR "celebrate.wav",
            };

            for (TetrisMusic music = TetrisMusic_None + 1; music < TetrisMusic_Count; music++) {
                BaseReadResult soundFile = {0};
                char *path = musicPaths[music];
                if (platformReadEntireFile(path, &tetris->arena, &soundFile)) {
                    tetris->musics[music] = wavParseFile(soundFile);
                }
            }

            char effectsPaths[TetrisSoundEffect_Count][256] = {
                [TetrisSoundEffect_Whoosh] = TETRIS_DATA_DIR "whoosh.wav",
                [TetrisSoundEffect_Click] = TETRIS_DATA_DIR "click.wav",
                [TetrisSoundEffect_GameOver] = TETRIS_DATA_DIR "game-over.wav",
                [TetrisSoundEffect_Yay] = TETRIS_DATA_DIR "yay.wav",
                [TetrisSoundEffect_LevelUp] = TETRIS_DATA_DIR "level-up.wav",
            };

            for (TetrisSoundEffect effect = TetrisSoundEffect_None + 1; effect < TetrisSoundEffect_Count; effect++) {
                BaseReadResult soundFile = {0};
                char *path = effectsPaths[effect];
                if (platformReadEntireFile(path, &tetris->arena, &soundFile)) {
                    tetris->soundEffects[effect] = wavParseFile(soundFile);
                }
            }
        }
    }

    return ok;
}

static TetrisPiecePattern tetrisGetPiecePattern(TetrisPieceType pieceType) {
    BASE_ASSERT(pieceType > TetrisPieceType_None);
    BASE_ASSERT(pieceType < TetrisPieceType_Count);

    TetrisPiecePattern patterns[TetrisPieceType_Count] = {
        [TetrisPieceType_I] = {{{-1.5f, -0.5f}, {-0.5f, -0.5f}, {0.5f, -0.5f}, {1.5f,  -0.5f}}},
        [TetrisPieceType_O] = {{{-0.5f, -0.5f}, {0.5f, -0.5f}, {-0.5f, 0.5f}, {0.5f, 0.5f}}},
        [TetrisPieceType_T] = {{{0.0f, -1.0f}, {-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}}},
        [TetrisPieceType_L] = {{{-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, -1.0f}}},
        [TetrisPieceType_J] = {{{-1.0f, -1.0f}, {-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}}},
        [TetrisPieceType_Z] = {{{-1.0f, -1.0f}, {0.0f, -1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}}},
        [TetrisPieceType_S] = {{{-1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, -1.0f}, {1.0f, -1.0f}}},
    };

    TetrisPiecePattern result = patterns[pieceType];

    return result;
}

static uint32_t tetrisGetCurrentLevel(Tetris *tetris) {
    uint32_t result = (tetris->clearedRows / TETRIS_LINES_PER_LEVEL) + 1;

    if (result >= TETRIS_LEVEL_COUNT_PLUS_ONE) {
        result = TETRIS_LEVEL_COUNT_PLUS_ONE - 1;
    }

    return result;
}

static TetrisColorScheme tetrisGetColorScheme(Tetris *tetris) {
    uint32_t level = tetrisGetCurrentLevel(tetris);

    TetrisColorScheme schemes[] = {
        [1] = {
            {
                [TetrisPieceType_I] = {0.0f, 255.0f, 255.0f, 255.0f},
                [TetrisPieceType_O] = {255.0f, 255.0f, 0.0f, 255.0f},
                [TetrisPieceType_T] = {180.0f, 0.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 140.0f, 0.0f, 255.0f},
                [TetrisPieceType_J] = {0.0f, 120.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 0.0f, 80.0f, 255.0f},
                [TetrisPieceType_S] = {0.0f, 255.0f, 120.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {255.0f, 64.0f, 64.0f, 255.0f},
                [TetrisPieceType_O] = {64.0f, 255.0f, 64.0f, 255.0f},
                [TetrisPieceType_T] = {64.0f, 128.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 200.0f, 64.0f, 255.0f},
                [TetrisPieceType_J] = {200.0f, 64.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 64.0f, 160.0f, 255.0f},
                [TetrisPieceType_S] = {64.0f, 255.0f, 200.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {0.0f, 200.0f, 255.0f, 255.0f},
                [TetrisPieceType_O] = {255.0f, 180.0f, 0.0f, 255.0f},
                [TetrisPieceType_T] = {255.0f, 0.0f, 200.0f, 255.0f},
                [TetrisPieceType_L] = {120.0f, 255.0f, 0.0f, 255.0f},
                [TetrisPieceType_J] = {0.0f, 100.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 50.0f, 0.0f, 255.0f},
                [TetrisPieceType_S] = {0.0f, 255.0f, 150.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {255.0f, 0.0f, 0.0f, 255.0f},
                [TetrisPieceType_O] = {0.0f, 255.0f, 0.0f, 255.0f},
                [TetrisPieceType_T] = {0.0f, 0.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 255.0f, 0.0f, 255.0f},
                [TetrisPieceType_J] = {255.0f, 0.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {0.0f, 255.0f, 255.0f, 255.0f},
                [TetrisPieceType_S] = {255.0f, 140.0f, 0.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {255.0f, 120.0f, 120.0f, 255.0f},
                [TetrisPieceType_O] = {120.0f, 255.0f, 120.0f, 255.0f},
                [TetrisPieceType_T] = {120.0f, 120.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 200.0f, 120.0f, 255.0f},
                [TetrisPieceType_J] = {200.0f, 120.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 120.0f, 200.0f, 255.0f},
                [TetrisPieceType_S] = {120.0f, 255.0f, 200.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {0.0f, 255.0f, 200.0f, 255.0f},
                [TetrisPieceType_O] = {255.0f, 220.0f, 0.0f, 255.0f},
                [TetrisPieceType_T] = {180.0f, 0.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 100.0f, 0.0f, 255.0f},
                [TetrisPieceType_J] = {0.0f, 140.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 0.0f, 120.0f, 255.0f},
                [TetrisPieceType_S] = {0.0f, 255.0f, 80.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {173.0f, 255.0f, 47.0f, 255.0f},
                [TetrisPieceType_O] = {255.0f, 105.0f, 180.0f, 255.0f},
                [TetrisPieceType_T] = {65.0f, 105.0f, 225.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 165.0f, 0.0f, 255.0f},
                [TetrisPieceType_J] = {138.0f, 43.0f, 226.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 69.0f, 0.0f, 255.0f},
                [TetrisPieceType_S] = {0.0f, 255.0f, 180.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {255.0f, 80.0f, 0.0f, 255.0f},
                [TetrisPieceType_O] = {0.0f, 200.0f, 255.0f, 255.0f},
                [TetrisPieceType_T] = {200.0f, 0.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 255.0f, 80.0f, 255.0f},
                [TetrisPieceType_J] = {0.0f, 255.0f, 120.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 0.0f, 0.0f, 255.0f},
                [TetrisPieceType_S] = {80.0f, 160.0f, 255.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {0.0f, 255.0f, 255.0f, 255.0f},
                [TetrisPieceType_O] = {255.0f, 200.0f, 0.0f, 255.0f},
                [TetrisPieceType_T] = {255.0f, 0.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {0.0f, 255.0f, 120.0f, 255.0f},
                [TetrisPieceType_J] = {255.0f, 80.0f, 80.0f, 255.0f},
                [TetrisPieceType_Z] = {120.0f, 80.0f, 255.0f, 255.0f},
                [TetrisPieceType_S] = {80.0f, 255.0f, 80.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {255.0f, 64.0f, 128.0f, 255.0f},
                [TetrisPieceType_O] = {64.0f, 255.0f, 192.0f, 255.0f},
                [TetrisPieceType_T] = {192.0f, 64.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 192.0f, 64.0f, 255.0f},
                [TetrisPieceType_J] = {64.0f, 128.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 64.0f, 64.0f, 255.0f},
                [TetrisPieceType_S] = {128.0f, 255.0f, 64.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {0.0f, 180.0f, 255.0f, 255.0f},
                [TetrisPieceType_O] = {255.0f, 220.0f, 120.0f, 255.0f},
                [TetrisPieceType_T] = {200.0f, 120.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 120.0f, 60.0f, 255.0f},
                [TetrisPieceType_J] = {120.0f, 255.0f, 200.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 80.0f, 140.0f, 255.0f},
                [TetrisPieceType_S] = {120.0f, 255.0f, 120.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {255.0f, 0.0f, 120.0f, 255.0f},
                [TetrisPieceType_O] = {0.0f, 255.0f, 200.0f, 255.0f},
                [TetrisPieceType_T] = {120.0f, 0.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 200.0f, 0.0f, 255.0f},
                [TetrisPieceType_J] = {0.0f, 120.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 60.0f, 0.0f, 255.0f},
                [TetrisPieceType_S] = {120.0f, 255.0f, 0.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {120.0f, 255.0f, 255.0f, 255.0f},
                [TetrisPieceType_O] = {255.0f, 255.0f, 120.0f, 255.0f},
                [TetrisPieceType_T] = {255.0f, 120.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {120.0f, 255.0f, 120.0f, 255.0f},
                [TetrisPieceType_J] = {255.0f, 120.0f, 120.0f, 255.0f},
                [TetrisPieceType_Z] = {120.0f, 120.0f, 255.0f, 255.0f},
                [TetrisPieceType_S] = {255.0f, 180.0f, 120.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {0.0f, 255.0f, 100.0f, 255.0f},
                [TetrisPieceType_O] = {255.0f, 200.0f, 0.0f, 255.0f},
                [TetrisPieceType_T] = {200.0f, 0.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 80.0f, 80.0f, 255.0f},
                [TetrisPieceType_J] = {0.0f, 150.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 0.0f, 0.0f, 255.0f},
                [TetrisPieceType_S] = {120.0f, 255.0f, 0.0f, 255.0f},
            }
        },
        {
            {
                [TetrisPieceType_I] = {255.0f, 50.0f, 200.0f, 255.0f},
                [TetrisPieceType_O] = {50.0f, 255.0f, 200.0f, 255.0f},
                [TetrisPieceType_T] = {200.0f, 50.0f, 255.0f, 255.0f},
                [TetrisPieceType_L] = {255.0f, 200.0f, 50.0f, 255.0f},
                [TetrisPieceType_J] = {50.0f, 150.0f, 255.0f, 255.0f},
                [TetrisPieceType_Z] = {255.0f, 50.0f, 50.0f, 255.0f},
                [TetrisPieceType_S] = {50.0f, 255.0f, 50.0f, 255.0f},
            }
        }
    };

    static_assert(BASE_ARRAYCOUNT(schemes) == TETRIS_LEVEL_COUNT_PLUS_ONE);

    BASE_ASSERT(level < TETRIS_LEVEL_COUNT_PLUS_ONE);

    TetrisColorScheme result = schemes[level];

    return result;
}

static BaseColor tetrisGetPieceColor(Tetris *tetris, TetrisPieceType pieceType) {
    BASE_ASSERT(pieceType > TetrisPieceType_None);
    BASE_ASSERT(pieceType < TetrisPieceType_Count);

    TetrisColorScheme scheme = tetrisGetColorScheme(tetris);

    BaseColor result = scheme.colors[pieceType];

    return result;
}

static BaseString tetrisGetButtonLabel(Tetris *tetris, TetrisButtonType buttonType) {
    BASE_ASSERT(buttonType > TetrisButtonType_None);
    BASE_ASSERT(buttonType < TetrisButtonType_Count);

    BaseString result = {0};

    switch (buttonType) {
        case TetrisButtonType_New: {
            result = BASE_MAKE_STRING("New");
        } break;
        case TetrisButtonType_Resume: {
            result = BASE_MAKE_STRING("Resume");
        } break;
        case TetrisButtonType_Sound: {
            result = tetris->effectsOff ? BASE_MAKE_STRING("Sound on") : BASE_MAKE_STRING("Sound off");
        } break;
        case TetrisButtonType_Music: {
            result = tetris->musicOff ? BASE_MAKE_STRING("Music on") : BASE_MAKE_STRING("Music off");
        } break;
        case TetrisButtonType_Quit: {
            result =  BASE_MAKE_STRING("Quit");
        } break;
    }

    return result;
}

static TetrisPieceType tetrisGetButtonPieceType (TetrisButtonType buttonType) {
    BASE_ASSERT(buttonType > TetrisButtonType_None);
    BASE_ASSERT(buttonType < TetrisButtonType_Count);

    TetrisPieceType types[TetrisButtonType_Count] = {
        [TetrisButtonType_New] = TetrisPieceType_I,
        [TetrisButtonType_Resume] = TetrisPieceType_O,
        [TetrisButtonType_Sound] = TetrisPieceType_L,
        [TetrisButtonType_Music] = TetrisPieceType_S,
        [TetrisButtonType_Quit] = TetrisPieceType_Z,
    };

    TetrisPieceType result = types[buttonType];

    return result;
}

static void tetrisDrawColorTrapezoid(
    Platform *platform,
    float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    BaseColor color
) {
    platformDrawColorTriangle(
        platform,
        x0, y0,
        x1, y1,
        x2, y2,
        0.0f, 0.0f,
        color.r, color.g, color.b, color.a
    );

    platformDrawColorTriangle(
        platform,

        x2, y2,
        x3, y3,
        x0, y0,
        0.0f, 0.0f,
        color.r, color.g, color.b, color.a
    );
}

static bool tetrisHasWon(Tetris *tetris) {
    bool result = (tetris->clearedRows / TETRIS_LINES_PER_LEVEL) + 1 >= TETRIS_LEVEL_COUNT_PLUS_ONE;

    return result;
}

static void tetrisDrawColorQuad(
    Platform *platform,
    float x, float y,
    float width, float height,
    BaseColor color
) {
    tetrisDrawColorTrapezoid(
        platform,
        x, y,
        x + width, y,
        x + width, y + height,
        x, y + height,
        color
    );
}

static BaseColor tetrisMultiplyColor(BaseColor color, float multiplier) {
    BaseColor result  = color;

    result.r = ((result.r / 255.0f) * multiplier) * 255.0f;
    result.g = ((result.g / 255.0f) * multiplier) * 255.0f;
    result.b = ((result.b / 255.0f) * multiplier) * 255.0f;

    return result;
}

static void tetrisDrawCellLikeQuad(
    Platform *platform,
    float x, float y,
    float width, float height,
    float margin,
    BaseColor color
)  {
    float internalWidth = width - (margin * 2.0f);
    float internalHeight = height - (margin * 2.0f);
    float internalX = x + margin;
    float internalY = y + margin;
    tetrisDrawColorQuad(
        platform,
        internalX, internalY,
        internalWidth, internalHeight,
        color
    );

    float lightMultiplier = 1.5f;

    tetrisDrawColorTrapezoid(
        platform,
        x, y,
        internalX, internalY,
        internalX , internalY + internalHeight,
        x, y + height,
        tetrisMultiplyColor(color, lightMultiplier)
    );

    tetrisDrawColorTrapezoid(
        platform,
        x, y,
        x + width, y,
        internalX + internalWidth, internalY,
        internalX, internalY,
        tetrisMultiplyColor(color, lightMultiplier)
    );

    float darkMultiplier = 0.50f;

    tetrisDrawColorTrapezoid(
        platform,
        x + width, y,
        x + width, y + height,
        internalX + internalWidth, internalY + internalHeight,
        internalX + internalWidth, internalY,
        tetrisMultiplyColor(color, darkMultiplier)
    );

    tetrisDrawColorTrapezoid(
        platform,
        internalX, internalY + internalHeight,
        internalX + internalWidth, internalY + internalHeight,
        x + width, y + height,
        x, y + height,
        tetrisMultiplyColor(color, darkMultiplier)
    );
}

static float tetrisGetCellMargin(float cellSide) {
    float internalSide = cellSide * 0.7f;
    float result = (cellSide - internalSide) / 2.0f;

    return result;
}

static void tetrisDrawCell(
    Platform *platform,
    int32_t row, int32_t column,
    BaseColor color,
    float cellSide,
    float gridX, float gridY
)  {
    float x = gridX + (column * cellSide);
    float y = gridY + (row * cellSide);
    float margin = tetrisGetCellMargin(cellSide);

    tetrisDrawCellLikeQuad(
        platform,
        x,  y,
        cellSide, cellSide,
        margin,
        color
    );
}

static void tetrisStartMusic(Tetris *tetris, Platform *platform, TetrisMusic music) {
    BASE_ASSERT(music > TetrisMusic_None);
    BASE_ASSERT(music < TetrisMusic_Count);

    platformPlaySound(platform, tetris->musics[music], PlatformSoundType_Music);
}

static TetrisRotation tetrisGetRotation(TetrisRotationType rotationType) {
    BASE_ASSERT(rotationType > TetrisRotationType_None);
    BASE_ASSERT(rotationType < TetrisRotationType_Count);

    TetrisRotation rotations[TetrisRotationType_Count] = {
        [TetrisRotationType_S] = {false, false},
        [TetrisRotationType_R] = {true, false},
        [TetrisRotationType_2] = {false, true},
        [TetrisRotationType_L] = {true, true},
    };

    TetrisRotation rotation = rotations[rotationType];

    return rotation;
}

static TetrisPiecePattern tetrisGetRotatedCells(TetrisPieceType pieceType, TetrisRotationType rotationType) {
    TetrisPiecePattern pattern = tetrisGetPiecePattern(pieceType);

    TetrisRotation rotation = tetrisGetRotation(rotationType);

    TetrisPiecePattern result = {0};

    for (uint32_t cellIndex = 0; cellIndex < BASE_ARRAYCOUNT(pattern.cellCenters); cellIndex++) {
        TetrisFloatCoords srcCoords = pattern.cellCenters[cellIndex];
        TetrisFloatCoords rotatedCoords = srcCoords;
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

static TetrisPiece tetrisGetPieceCells(TetrisPieceType pieceType, TetrisRotationType rotationType, float x, float y) {
    TetrisPiecePattern rotatedCells = tetrisGetRotatedCells(pieceType, rotationType);
    TetrisPiece result = {0};

    for (uint32_t cellIndex = 0; cellIndex < BASE_ARRAYCOUNT(rotatedCells.cellCenters); cellIndex++) {
        TetrisFloatCoords cellCenter = rotatedCells.cellCenters[cellIndex];
        result.cells[cellIndex].x = (int32_t)(x + cellCenter.x - 0.5f);
        result.cells[cellIndex].y = (int32_t)(y + cellCenter.y - 0.5f);
    }

    return result;
}

static TetrisPiece tetrisGetPlayerCells(Tetris *tetris) {
    TetrisPiece result = tetrisGetPieceCells(
        tetris->playerType,
        tetris->playerRotationType,
        tetris->playerXInCells,
        tetris->playerYInCells
    );

    return result;
}

static bool tetrisIsCellAvailable(Tetris *tetris, int32_t x, int32_t y) {
    bool result = false;

    if (x >= 0 && x < BASE_ARRAYCOUNT(tetris->grid[0])) {
        if (y < 0) {
            result = true;
        } else if (y < BASE_ARRAYCOUNT(tetris->grid)) {
            result = tetris->grid[y][x] == TetrisPieceType_None;
        }
    }

    return result;
}

static bool tetrisIsPositionAvailable(Tetris *tetris, TetrisPiece position) {
    bool result = true;

    for (uint32_t cellIndex = 0; cellIndex < BASE_ARRAYCOUNT(position.cells); cellIndex++) {
        TetrisI32Coords cell = position.cells[cellIndex];
        if (!tetrisIsCellAvailable(tetris, cell.x, cell.y)) {
            result = false;
            break;
        }
    }

    return result;
}

static TetrisPiece tetrisOffsetCells(TetrisPiece cells, int32_t x, int32_t y) {
    TetrisPiece result = cells;

    for (uint32_t cellIndex = 0; cellIndex < BASE_ARRAYCOUNT(result.cells); cellIndex++) {
        result.cells[cellIndex].x += x;
        result.cells[cellIndex].y += y;
    }

    return result;
}

static bool tryRotation(Tetris *tetris, TetrisRotationType newRotation, float xOffsetInCells, float yOffsetInCells) {
    bool ok = false;
    TetrisPiece targetCells = tetrisGetPieceCells(
        tetris->playerType,
        newRotation,
        tetris->playerXInCells + xOffsetInCells,
        tetris->playerYInCells + yOffsetInCells
    );
    if (tetrisIsPositionAvailable(tetris, targetCells)) {
        tetris->playerRotationType = newRotation;
        tetris->playerXInCells += xOffsetInCells;
        tetris->playerYInCells += yOffsetInCells;
        ok = true;
    }

    return ok;
}

static void tetrisRotatePlayer(Tetris *tetris, int32_t rotation) {
    BASE_ASSERT(rotation >= -2);
    BASE_ASSERT(rotation <= 2);

    int32_t newRotation = (int32_t)tetris->playerRotationType + rotation;

    if ((newRotation) >= (int32_t)TetrisRotationType_Count) {
        newRotation = newRotation - (int32_t)TetrisRotationType_Count + 1;
    }

    if ((newRotation) <= 0) {
        newRotation =  (int32_t)TetrisRotationType_Count - 1 - newRotation;
    }

    bool ok = tryRotation(tetris, (TetrisRotationType)newRotation, 0.0f, 0.0f);

    if (!ok) {
        ok = tryRotation(tetris, (TetrisRotationType)newRotation, 1.0f, 0.0f);
    }

    if (!ok) {
        tryRotation(tetris, (TetrisRotationType)newRotation, -1.0f, 0.0f);
    }
}

static TetrisPieceType getNextType(Tetris *tetris) {
    uint32_t random = tetrisGetRandomNumber(tetris);
    TetrisPieceType result = (TetrisPieceType)((random % (TetrisPieceType_Count - 1)) + 1);

    return result;
}

static bool tetrisIsRowFull(Tetris *tetris, int32_t row) {
    bool result = true;

    if (row >= 0 && row < TETRIS_ROW_COUNT) {
        for (int32_t column = 0; column < TETRIS_COLUMN_COUNT && result; column++) {
            if (tetrisIsCellAvailable(tetris, column, row)) {
                result = false;
            }
        }
    }

    return result;
}

static bool tetrisIsRowEmpty(Tetris *tetris, int32_t row) {
    bool result = true;

    if (row >= 0 && row < TETRIS_ROW_COUNT) {
        for (int32_t column = 0; column < TETRIS_COLUMN_COUNT && result; column++) {
            if (!tetrisIsCellAvailable(tetris, column, row)) {
                result = false;
            }
        }
    }

    return result;
}
static void tetrisClearRow(Tetris *tetris, int32_t y) {
    for (int32_t x = 0; x < TETRIS_COLUMN_COUNT; x++) {
        TetrisPieceType pieceType = tetris->grid[y][x];
        if (pieceType) {
            tetris->grid[y][x] = TetrisPieceType_None;
        }
    }
}

static TetrisPatternFeatures tetrisGetPatternFeatures(TetrisPieceType type) {
    TetrisPiecePattern pattern = tetrisGetPiecePattern(type);

    float minCenterX = 5.0f;
    float minCenterY = 5.0f;
    float maxCenterX = -5.0f;
    float maxCenterY = -5.0f;

    for (uint32_t cellIndex = 0; cellIndex < BASE_ARRAYCOUNT(pattern.cellCenters); cellIndex++) {
        TetrisFloatCoords center = pattern.cellCenters[cellIndex];
        minCenterX = TETRIS_MIN(center.x, minCenterX);
        minCenterY = TETRIS_MIN(center.y, minCenterY);
        maxCenterX = TETRIS_MAX(center.x, maxCenterX);
        maxCenterY = TETRIS_MAX(center.y, maxCenterY);
    }

    TetrisPatternFeatures result = {0};

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

static float tetrisGetSpawnX(TetrisPieceType type) {
    BASE_ASSERT(type > TetrisPieceType_None);
    BASE_ASSERT(type < TetrisPieceType_Count);

    TetrisPatternFeatures features = tetrisGetPatternFeatures(type);

    float result = ((float)TETRIS_COLUMN_COUNT / 2.0f);

    if ((int32_t)features.width % 2 != 0) {
        result -= 0.5f;
    }

    return result;
}

static float tetrisGetSpawnY(TetrisPieceType type) {
    TetrisPatternFeatures features = tetrisGetPatternFeatures(type);

    float result = -features.maxY;

    return result;
}

static void tetrisSpawnPiece(Tetris *tetris) {
    if (tetris->nextPlayerType == TetrisPieceType_None) {
        tetris->nextPlayerType = getNextType(tetris);
    }

    tetris->playerType = tetris->nextPlayerType;
    tetris->nextPlayerType = getNextType(tetris);
    tetris->playerXInCells = tetrisGetSpawnX(tetris->playerType);
    tetris->playerYInCells = tetrisGetSpawnY(tetris->playerType);
    tetris->playerRotationType = TetrisRotationType_None + 1;
    tetris->horizontalDirection = TetrisHorizontalDirection_None;
    tetris->playerXProgression = 0.0f;
    tetris->playerYProgression = 0.0f;
    tetris->isSoftDropping = false;
    tetris->isHardDropping = false;
}

static void tetrisPlaySoundEffect(Tetris *tetris, Platform *platform, TetrisSoundEffect soundEffect) {
    BASE_ASSERT(soundEffect > TetrisSoundEffect_None);
    BASE_ASSERT(soundEffect < TetrisSoundEffect_Count);

    if (!tetris->effectsOff) {
        platformPlaySound(platform, tetris->soundEffects[soundEffect], PlatformSoundType_Effect);
    }
}
static void tetrisAddClearedRows(Tetris *tetris, Platform *platform, uint32_t rowsCount) {
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

    if (tetris->fadingRowsCount > 0) {
        tetris->secondsToFadeEnd = TETRIS_FADE_SECONDS;
        tetrisPlaySoundEffect(tetris, platform, TetrisSoundEffect_Whoosh);
    }

    tetris->score += scoreIncrement;
    uint32_t previousLevel = tetrisGetCurrentLevel(tetris);
    tetris->clearedRows += rowsCount;
    uint32_t currentLevel = tetrisGetCurrentLevel(tetris);

    if (currentLevel > previousLevel) {
        tetrisPlaySoundEffect(tetris, platform, TetrisSoundEffect_LevelUp);
    }

    if (tetrisHasWon(tetris)) {
        tetrisStartMusic(tetris, platform, TetrisMusic_Celebrate);
    }
}

static void tetrisMoveVertically(Tetris *tetris, Platform *platform) {
    TetrisPiece playerCells = tetrisGetPlayerCells(tetris);
    TetrisPiece cellsBelow = tetrisOffsetCells(playerCells, 0, 1);
    if (tetrisIsPositionAvailable(tetris, cellsBelow)) {
        tetris->playerYProgression -= 1.0f;
        tetris->playerYInCells += 1.0f;
        if (tetris->isHardDropping) {
            tetris->score += 2;
        } else if (tetris->isSoftDropping) {
            tetris->score += 1;
        }
    } else {
        int32_t minY = TETRIS_ROW_COUNT;
        int32_t maxY = -2;
        for (uint32_t cellIndex = 0; cellIndex < BASE_ARRAYCOUNT(playerCells.cells); cellIndex++) {
            TetrisI32Coords cell = playerCells.cells[cellIndex];
            if (cell.y < minY) {
                minY = cell.y;
            }
            if (cell.y > maxY) {
                maxY = cell.y;
            }
            if (
                cell.y >= 0 && cell.y < BASE_ARRAYCOUNT(tetris->grid)
                && cell.x >= 0 && cell.x < BASE_ARRAYCOUNT(tetris->grid[0])
            ){
                tetris->grid[cell.y][cell.x] = tetris->playerType;
            }
        }

        if (minY < 0) {
            tetris->gameOver = true;
            platformPauseSound(platform, PlatformSoundType_Music);
        }

        for (int32_t y = minY; y <= maxY; y++) {
            if (y >= 0) {
                if (tetrisIsRowFull(tetris, y)) {
                    tetris->fadingRows[tetris->fadingRowsCount++] = y;
                }
            }
        }
        BASE_ASSERT(tetris->fadingRowsCount <= BASE_ARRAYCOUNT(tetris->fadingRows));

        tetrisAddClearedRows(tetris, platform, tetris->fadingRowsCount);

        tetrisSpawnPiece(tetris);
    }
}

static TetrisFloatCoords tetrisCenterPieceInBox(TetrisPieceType type, float x, float y, float width, float height, float cellSideInPixels) {
    TetrisPatternFeatures features = tetrisGetPatternFeatures(type);

    TetrisFloatCoords result = {0};

    result.x = x + (width / 2.0f) - (features.esteticCenter.x * cellSideInPixels);
    result.y = y + (height / 2.0f) - (features.esteticCenter.y * cellSideInPixels);

    return result;
}

static void tetrisDrawCenteredPattern(Tetris *tetris, Platform *platform, TetrisPieceType type, float boxX, float  boxY, float boxWidth,float  boxHeight,float  cellSideInPixels) {
    TetrisFloatCoords offset = tetrisCenterPieceInBox(type, boxX, boxY, boxWidth, boxHeight, cellSideInPixels);

    BaseColor color = tetrisGetPieceColor(tetris, type);

    TetrisPiecePattern pattern = tetrisGetPiecePattern(type);

    for (uint32_t cellIndex = 0; cellIndex < BASE_ARRAYCOUNT(pattern.cellCenters); cellIndex++) {
        TetrisFloatCoords center = pattern.cellCenters[cellIndex];

        float cellX = ((center.x - 0.5f) * cellSideInPixels) + offset.x;
        float cellY = ((center.y - 0.5f) * cellSideInPixels) + offset.y;

        tetrisDrawCellLikeQuad(
            platform,
            cellX, cellY,
            cellSideInPixels, cellSideInPixels,
            tetrisGetCellMargin(cellSideInPixels),
            color
        );
    }
}

static void tetrisDrawNextPiece(Tetris * tetris, Platform *platform, float boxX, float boxY, float boxWidth, float boxHeight, float cellSideInPixels) {
    tetrisDrawCenteredPattern(
        tetris,
        platform,
        tetris->nextPlayerType,
        boxX, boxY,
        boxWidth, boxHeight,
        cellSideInPixels
    );
}

static float tetrisGetStringWidthInPixels(TetrisAtlas font, BaseString string) {
    float result = 0.0f;

    for (uint32_t glyphIndex = 0; glyphIndex < string.size; glyphIndex++) {
        char c = string.text[glyphIndex];
        TetrisGlyph glyph = font.glyphs[c - TETRIS_FIRST_CODEPOINT];

        if (glyphIndex == string.size - 1) {
            result += glyph.bitmapWidthInPixels;
        } else {
            result += glyph.advanceWidthInPixels;
        }
    }

    return result;
}

static BaseString tetrisFormatNumber(uint32_t number, char *dest, uint32_t destSize) {
    BaseString result = {0};

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

static bool tetrisIsFading(Tetris *tetris) {
    return tetris->secondsToFadeEnd > 0.0f;
}

static bool tetrisAreRowsFalling(Tetris *tetris) {
    return tetris->fadingRowsCount > 0 && !tetrisIsFading(tetris);
}

static bool shouldUpdate(Tetris *tetris) {
    return !tetris->wasResizing && !tetris->menuOpen && !tetris->paused && tetris->fadingRowsCount == 0 && !tetris->gameOver && !tetrisHasWon(tetris);
}

static void tetrisCloseMenu(Tetris *tetris) {
    tetris->menuOpen = false;
    tetris->paused = false;
    tetris->pressedButton = TetrisButtonType_None;
    tetris->hoveredButton = TetrisButtonType_None;
}

static void tetrisOpenMenu(Tetris *tetris) {
    tetris->menuOpen = true;
    tetris->hoveredButton = TetrisButtonType_None + 1;
}

static void tetrisNewGame(Tetris *tetris, Platform *platform) {
    platformMemset(tetris->grid, 0, sizeof(tetris->grid));
    tetris->score = 0;
    tetris->clearedRows = 0;
    tetris->isHardDropping = false;
    tetris->isSoftDropping = false;
    tetris->secondsToFadeEnd = 0;
    tetris->fadingRowsCount = 0;
    tetris->gameOver = false;
    tetris->gameOverAnimationSteps = 0;
    tetris->secondsToNextGameOverAnimation = 0.0f;
    tetris->secondsToOpenMenu = 0.0f;

    tetrisSpawnPiece(tetris);

    if (!tetris->musicOff) {
        tetrisStartMusic(tetris, platform, TetrisMusic_Theme);
    }
    tetrisCloseMenu(tetris);
}

void tetrisResumeGame(Tetris *tetris, Platform *platform) {
    if (tetris->gameOver || tetrisHasWon(tetris)) {
        tetrisNewGame(tetris, platform);
    } else {tetrisCloseMenu(tetris);}
}

static float tetrisGetVelocityMultiplier(Tetris *tetris) {
    float result = 1.0f;

    uint32_t currentLevel = tetrisGetCurrentLevel(tetris);

    for (uint32_t level = 1; level < currentLevel; level++) {
        result *= 1.2f;
    }

    return result;
}

static void tetrisDrawLabel(Tetris *tetris, Platform *platform, float x, float y, float width, float height, float margin, BaseString label, BaseColor backgroundColor, BaseColor fontColor) {
    tetrisDrawCellLikeQuad(
        platform,
        x, y,
        width, height,
        5.0f,
        backgroundColor
    );

    float labelX = x + margin;

    tetrisDrawString(
        tetris,
        platform,
        label,
        labelX,
        y,
        1.0f,
        fontColor
    );
}

static void tetrisDrawNumberLabel(Tetris *tetris, Platform *platform, float x, float y, float width, float height, float margin, BaseString label, BaseColor backgroundColor, BaseColor fontColor, uint32_t number) {
    {
        tetrisDrawLabel(tetris, platform, x,  y,  width,  height,  margin,  label,  backgroundColor,  fontColor);
        char buffer[256] = {0};

        float labelX = x + margin;

        tetrisDrawString(
            tetris,
            platform,
            tetrisFormatNumber(number, buffer, BASE_ARRAYCOUNT(buffer)),
            labelX,
            y + tetris->atlas.lineHeightInPixels,
            1.0f,
            fontColor
        );
    }
}

static void tetrisDrawNextPieceLabel(Tetris *tetris, Platform *platform, float x, float y, float width, float height, float margin,  BaseColor backgroundColor, BaseColor fontColor, float cellSideInPixels) {
    {
        float boxMargin = 5.0f;

        BaseString next = BASE_MAKE_STRING("Next:");

        tetrisDrawLabel(
            tetris, platform, x, y, width, height, margin,
            next,
            backgroundColor, fontColor
        );

        {
            float strWidth = tetrisGetStringWidthInPixels(tetris->atlas, next);
            float pieceX = x + margin + strWidth;
            float pieceY = y + boxMargin;
            float pieceWidth = width - (boxMargin * 2.0f) - margin - strWidth;
            float pieceHeight = height - (boxMargin * 2.0f);
            tetrisDrawNextPiece(tetris, platform, pieceX , pieceY,  pieceWidth,  pieceHeight,  cellSideInPixels);
        }
    }
}

static bool tetrisPointInQuad(float x, float y, float top, float left, float width, float height) {
    float right = left + width;
    float bottom = top + height;

    bool result = x >= left
    && x <= right
    && y >= top
    && y <= bottom;

    return result;
}

static TetrisLayout tetrisGetLayout(Tetris *tetris, PlatformInput input) {
    TetrisLayout result = {0};

    int32_t boardWidthInColumns = TETRIS_COLUMN_COUNT + 2;
    int32_t boardWidthInRows = TETRIS_ROW_COUNT + 2;
    float aspectRatio = ((float)input.windowWidth / boardWidthInColumns) / ((float)input.windowHeight / boardWidthInRows);
    result.cellSideInPixels = 0.0f;
    if (aspectRatio > 1.0f) {
        result.cellSideInPixels = ((float)input.windowHeight * TETRIS_MAX_HEIGTH_RATIO) / boardWidthInRows;
    } else {
        result.cellSideInPixels = ((float)input.windowWidth * TETRIS_MAX_WIDTH_RATIO) / boardWidthInColumns;
    }

    result.gridWidth = result.cellSideInPixels * TETRIS_COLUMN_COUNT;
    result.gridHeight = result.cellSideInPixels * TETRIS_ROW_COUNT;
    result.gridX = ((float)input.windowWidth - result.gridWidth) / 2.0f;

    result.gridMargin = 20.0f;

    result.boxHeight = tetris->atlas.lineHeightInPixels * 2.0f;
    result.boxWidth = result.boxHeight * 3.0f;

    result.drawLabels = (result.gridX + result.gridWidth + result.cellSideInPixels + result.gridMargin + result.boxWidth) <= input.windowWidth;

    result.gridY = result.drawLabels ? ((float)input.windowHeight - result.gridHeight) / 2.0f : ((float)input.windowHeight - result.gridHeight) - result.cellSideInPixels;

    return result;
}

static void tetrisDoBackground(
    Platform *platform
    , PlatformInput input, TetrisLayout layout
) {
    tetrisDrawColorQuad(
        platform,
        0.0f, 0.0f,
        (float)input.windowWidth, (float)input.windowHeight,
        baseMakeColor(34.0f, 67.0f, 74.0f, 255.0f)
    );
    tetrisDrawColorQuad(
        platform,
        layout.gridX, layout.gridY,
        layout.gridWidth, layout.gridHeight,
        baseMakeColor(0.0f, 0.0f, 0.0f, 255.0f)
    );
}

static void tetrisDoGridBorder(Platform *platform, TetrisLayout layout, BaseColor gridBorderColor) {
    for (int32_t column = -1; column < TETRIS_COLUMN_COUNT + 1; column++) {
        int32_t row = -1;
        tetrisDrawCell(
            platform,
            row,
            column,
            gridBorderColor,
            layout.cellSideInPixels,
            layout.gridX,
            layout.gridY
        );

        row = TETRIS_ROW_COUNT;
        tetrisDrawCell(
            platform,
            row,
            column,
            gridBorderColor,
            layout.cellSideInPixels,
            layout.gridX,
            layout.gridY
        );
    }

    for (int32_t row = 0; row < TETRIS_ROW_COUNT; row++) {
        int32_t column = -1;
        tetrisDrawCell(
            platform,
            row,
            column,
            gridBorderColor,
            layout.cellSideInPixels,
            layout.gridX,
            layout.gridY
        );

        column = TETRIS_COLUMN_COUNT;
        tetrisDrawCell(
            platform,
            row,
            column,
            gridBorderColor,
            layout.cellSideInPixels,
            layout.gridX,
            layout.gridY
        );
    }
}

static void tetrisDoPlayer(Tetris *tetris, Platform *platform, PlatformInput input, TetrisLayout layout) {
    {
        if (shouldUpdate(tetris)) {
            float velocityMultiplier = tetrisGetVelocityMultiplier(tetris);
            float verticalVelocity = 3.0f * velocityMultiplier;

            if (tetrisControlPressed(input, PlatformControlType_Space)) {
                tetris->isHardDropping = true;
                tetris->isSoftDropping = false;
                tetrisPlaySoundEffect(tetris, platform, TetrisSoundEffect_Click);
            }

            if (!tetris->isHardDropping && tetrisControlPressed(input, PlatformControlType_Down)) {
                tetris->isSoftDropping = true;
            }

            if (!tetrisControlDown(input, PlatformControlType_Down)) {
                tetris->isSoftDropping = false;
            }

            if (tetris->isHardDropping) {
                verticalVelocity = 200.0f;
            } else if (tetris->isSoftDropping) {
                verticalVelocity *= 10.0f;
            }
            float horizontalVelocity = 8.0f;
            tetris->playerYProgression += verticalVelocity * input.secondsElapsed;

            bool leftDown = tetrisControlDown(input, PlatformControlType_Left);
            bool rightDown = tetrisControlDown(input, PlatformControlType_Right);

            TetrisHorizontalDirection previousDirection = tetris->horizontalDirection;

            if (!tetris->isHardDropping) {
                if (leftDown && !rightDown) {
                    tetris->horizontalDirection = TetrisHorizontalDirection_Left;
                } else if (rightDown && !leftDown) {
                    tetris->horizontalDirection = TetrisHorizontalDirection_Right;
                } else {
                    tetris->horizontalDirection = TetrisHorizontalDirection_None;
                    tetris->playerXProgression = 0.0f;
                }
            }

            if (tetris->horizontalDirection != previousDirection) {
                if (tetris->horizontalDirection == TetrisHorizontalDirection_Left) {
                    tetris->playerXProgression -= 1.0f;
                }
                if (tetris->horizontalDirection == TetrisHorizontalDirection_Right) {
                    tetris->playerXProgression += 1.0f;
                }
            } else {
                float xDelta =  horizontalVelocity * input.secondsElapsed;

                if (tetris->horizontalDirection == TetrisHorizontalDirection_Left) {
                    tetris->playerXProgression -= xDelta;
                }
                if (tetris->horizontalDirection == TetrisHorizontalDirection_Right) {
                    tetris->playerXProgression += xDelta;
                }
            }

            for (uint32_t pressIndex = 0; pressIndex < input.controls[PlatformControlType_C].pressCount; pressIndex++) {
                tetrisRotatePlayer(tetris, -1);
            }

            for (uint32_t pressIndex = 0; pressIndex < input.controls[PlatformControlType_Up].pressCount; pressIndex++) {
                tetrisRotatePlayer(tetris, +1);
            }
        }

        while (tetris->playerYProgression > 1.0f) {
            tetrisMoveVertically(tetris, platform);
        }

        while (tetris->playerXProgression >= 1.0f || tetris->playerXProgression <= -1.0f) {
            bool moveRight = tetris->playerXProgression > 0.0f ;
            float increment = moveRight ? 1.0f : -1.0f;
            TetrisPiece playerCells = tetrisGetPlayerCells(tetris);
            TetrisPiece nextCells = tetrisOffsetCells(playerCells, moveRight ? 1 : -1, 0);

            if (tetrisIsPositionAvailable(tetris, nextCells)) {
                tetris->playerXProgression -= increment;
                tetris->playerXInCells += increment;
            } else {
                tetris->playerXProgression = 0.0f;
            }
        }

        TetrisPiece playerCells = tetrisGetPlayerCells(tetris);
        BaseColor color = tetrisGetPieceColor(tetris, tetris->playerType);
        for (int32_t cellIndex = 0; cellIndex < BASE_ARRAYCOUNT(playerCells.cells); cellIndex++) {
            if (playerCells.cells[cellIndex].y >= 0) {
                tetrisDrawCell(
                    platform,
                    playerCells.cells[cellIndex].y,
                    playerCells.cells[cellIndex].x,
                    color,
                    layout.cellSideInPixels,
                    layout.gridX,
                    layout.gridY
                );
            }
        }
    }
}

static void tetrisDoFading(Tetris *tetris, Platform *platform, PlatformInput input) {
    bool wasFading = tetrisIsFading(tetris);
    if (tetrisIsFading(tetris)) {
        tetris->secondsToFadeEnd -= input.secondsElapsed;
    } else {
        tetris->secondsToFadeEnd = 0.0f;
    }
    bool isFading = tetrisIsFading(tetris);

    if (wasFading && !isFading) {
        for (int32_t rowIndex = 0; rowIndex < tetris->fadingRowsCount; rowIndex++) {
            int32_t clearedRow = tetris->fadingRows[rowIndex];
            tetrisClearRow(tetris, clearedRow);
        }
        tetrisPlaySoundEffect(tetris, platform, TetrisSoundEffect_Click);
    }
}

static void tetrisDoRowsFalling(Tetris *tetris, PlatformInput input) {
    float fallingVelocity = 20.0f;
    tetris->fallingYProgression += (fallingVelocity * input.secondsElapsed);

    while (tetris->fallingYProgression >= 1.0f && tetris->fadingRowsCount > 0) {
        int32_t clearedRow = tetris->fadingRows[tetris->fadingRowsCount - 1];

        for (int32_t y = clearedRow - 1; y >= 0; y--) {
            for (int32_t x = 0; x < TETRIS_COLUMN_COUNT; x++) {
                tetris->grid[y + 1][x] = tetris->grid[y][x];
            }
        }

        tetrisClearRow(tetris, 0);

        tetris->fadingRowsCount--;
        for (int32_t rowIndex = 0; rowIndex < tetris->fadingRowsCount; rowIndex++) {
            tetris->fadingRows[rowIndex]++;
        }
        tetris->fallingYProgression -= 1.0f;
    }
}

static void tetrisDoGrid(Tetris *tetris, Platform *platform, TetrisLayout layout)   {
    for (int32_t rowIndex = 0; rowIndex < BASE_ARRAYCOUNT(tetris->grid); rowIndex++) {
        bool isClearedRow = false;

        for (int32_t clearedRowIndex = 0; clearedRowIndex < tetris->fadingRowsCount; clearedRowIndex++) {
            if (tetris->fadingRows[clearedRowIndex] == rowIndex) {
                isClearedRow = true;
                break;
            }
        }

        for (int32_t columnIndex = 0; columnIndex < BASE_ARRAYCOUNT(tetris->grid[0]); columnIndex++) {
            TetrisPieceType cell = tetris->grid[rowIndex][columnIndex];

            if (!tetrisIsCellAvailable(tetris, columnIndex, rowIndex)) {
                BaseColor color = tetrisGetPieceColor(tetris, cell);

                if (isClearedRow && tetrisIsFading(tetris)) {
                    float fadeRatio = 1.0f - (tetris->secondsToFadeEnd / TETRIS_FADE_SECONDS);
                    float alphaRatio = fadeRatio * fadeRatio * fadeRatio;
                    float alpha = 255 - (alphaRatio * 255.0f);
                    color.a = alpha;
                }

                tetrisDrawCell(
                    platform,
                    rowIndex,
                    columnIndex,
                    color,
                    layout.cellSideInPixels,
                    layout.gridX,
                    layout.gridY
                );
            }
        }
    }
}

static void tetrisDoGameOver(Tetris *tetris, Platform *platform, PlatformInput input, TetrisLayout layout, BaseColor gridBorderColor) {
    if (tetris->gameOverAnimationSteps < TETRIS_ROW_COUNT) {
        float secondsForGameOverRow = 0.1f;
        if (tetris->secondsToNextGameOverAnimation <= 0.0f) {
            tetris->gameOverAnimationSteps++;
            tetris->secondsToNextGameOverAnimation += secondsForGameOverRow;
            TetrisSoundEffect soundEffect  = TetrisSoundEffect_Click;

            if (tetris->gameOverAnimationSteps >= TETRIS_ROW_COUNT - 1){
                soundEffect = TetrisSoundEffect_GameOver;
                tetris->secondsToOpenMenu = 1.5f;
            }

            tetrisPlaySoundEffect(tetris,  platform, soundEffect);
        }
        tetris->secondsToNextGameOverAnimation -= input.secondsElapsed;
    }

    for (uint32_t gameOverRowIndex = 0; gameOverRowIndex <tetris->gameOverAnimationSteps; gameOverRowIndex++) {
        for (uint32_t column = 0; column < TETRIS_COLUMN_COUNT; column++) {
            uint32_t row = TETRIS_ROW_COUNT - 1 - gameOverRowIndex;
            tetrisDrawCell(
                platform,
                row,
                column,
                gridBorderColor,
                layout.cellSideInPixels,
                layout.gridX,
                layout.gridY
            );
        }
    }
}

static void tetrisDoCelebrate(Tetris *tetris, Platform *platform, PlatformInput input) {
    uint32_t cellCount = TETRIS_ROW_COUNT * TETRIS_COLUMN_COUNT;

    float secondsForGameOverStep = 0.05f;
    if (tetris->secondsToNextGameOverAnimation <= 0.0f) {
        if (tetris->gameOverAnimationSteps < cellCount) {
            tetris->gameOverAnimationSteps++;
            if (tetris->gameOverAnimationSteps >= cellCount){
                tetris->secondsToOpenMenu = 1.5f;
                platformPauseSound(platform, PlatformSoundType_Music);
                tetrisPlaySoundEffect(tetris, platform, TetrisSoundEffect_Yay);
            }
        }
        tetris->secondsToNextGameOverAnimation += secondsForGameOverStep;

        for (uint32_t gameOverCellIndex = 0; gameOverCellIndex <tetris->gameOverAnimationSteps; gameOverCellIndex++) {
            TetrisPieceType type = getNextType(tetris);

            uint32_t gameOverRowIndex = gameOverCellIndex / TETRIS_COLUMN_COUNT;
            uint32_t gameOverColumnIndex = gameOverCellIndex % TETRIS_COLUMN_COUNT;

            tetris->grid[TETRIS_ROW_COUNT - gameOverRowIndex - 1][gameOverColumnIndex] = type;
        }
    }
    tetris->secondsToNextGameOverAnimation -= input.secondsElapsed;
}

static void tetrisDoInfo(Tetris *tetris, Platform *platform, PlatformInput input, TetrisLayout layout, BaseColor boxColor, BaseColor fontColor) {
    {
        if (layout.drawLabels) {
            float rightBoxX = layout.gridX + layout.gridWidth + (layout.gridMargin * 2.0f);
            float leftBoxX = layout.gridX - (layout.gridMargin * 2.0f) - layout.boxWidth;
            float upperBoxY = layout.gridY;
            float lowerBoxY = layout.gridY + layout.gridHeight - layout.boxHeight;

            tetrisDrawNumberLabel(
                tetris, platform, leftBoxX, upperBoxY, layout.boxWidth, layout.boxHeight, layout.gridMargin,
                BASE_MAKE_STRING("Rows:"),
                boxColor, fontColor,
                tetris->clearedRows
            );

            tetrisDrawNextPieceLabel(
                tetris, platform, rightBoxX, upperBoxY,layout.boxWidth, layout.boxHeight, layout.gridMargin,
                boxColor,  fontColor,
                layout.cellSideInPixels
            );

            tetrisDrawNumberLabel(
                tetris, platform, leftBoxX, lowerBoxY,layout.boxWidth, layout.boxHeight, layout.gridMargin,
                BASE_MAKE_STRING("Level:"),
                boxColor, fontColor,
                tetrisGetCurrentLevel(tetris)
            );

            tetrisDrawNumberLabel(
                tetris, platform, rightBoxX, lowerBoxY, layout.boxWidth, layout.boxHeight, layout.gridMargin,
                BASE_MAKE_STRING("Score:"),
                boxColor, fontColor,
                tetris->score
            );
        } else {
            tetrisDrawNextPiece(
                tetris, platform,
                0.0f, 0.0f,
                (float) input.windowWidth, layout.gridY - layout.cellSideInPixels,
                layout.cellSideInPixels
            );
        }
    }
}

static void tetrisDoMenu(Tetris *tetris, Platform *platform, PlatformInput input, TetrisLayout layout, BaseColor boxColor, BaseColor fontColor) {
    float menuWidth = layout.gridWidth * 1.2f;
    float menuLeft = (input.windowWidth - menuWidth) / 2.0f;
    float menuHeight = layout.gridHeight * 1.2f;
    float menuTop = (input.windowHeight - menuHeight) / 2.0f;

    tetrisDrawCellLikeQuad(
        platform,
        menuLeft, menuTop,
        menuWidth, menuHeight,
        5.0f,
        boxColor
    ) ;

    float buttonWidth = menuWidth * 0.8f;
    float buttonMargin = (menuWidth - buttonWidth) / 2.0f;
    float buttonHeight = buttonWidth * 0.4f;

    uint32_t buttonCount = TetrisButtonType_Count - 1;
    float buttonsGap = (menuHeight - (2.0f * buttonMargin) - ((float)buttonCount * buttonHeight)) / ((float) (buttonCount - 1));

    float buttonLeft = menuLeft + buttonMargin;
    float buttonTop = menuTop + buttonMargin;

    bool mouseDown = tetrisControlDown(input, PlatformControlType_MouseLeft);
    bool enterPressed = tetrisControlPressed(input, PlatformControlType_Enter);
    bool mousePressed = tetrisControlPressed(input, PlatformControlType_MouseLeft);
    bool mouseReleased = tetrisControlReleased(input, PlatformControlType_MouseLeft);

    if (tetris->hoveredButton > TetrisButtonType_None + 1 && tetrisControlPressed(input, PlatformControlType_Up)) {
        tetris->hoveredButton--;
    }

    if (tetris->hoveredButton < TetrisButtonType_Count - 1 && tetrisControlPressed(input, PlatformControlType_Down)) {
        tetris->hoveredButton++;
    }

    for (TetrisButtonType buttonType = TetrisButtonType_None + 1; buttonType < TetrisButtonType_Count; buttonType++) {
        BaseString label = tetrisGetButtonLabel(tetris, buttonType);
        TetrisPieceType pieceType = tetrisGetButtonPieceType(buttonType);
        BaseColor buttonColor = tetrisGetPieceColor(tetris, pieceType);

        float mouseX = (float) input.mouseX;
        float mouseY = (float) input.mouseY;
        float previousMouseX = (float) tetris->previousMouseX;
        float previousMouseY = (float) tetris->previousMouseY;

        bool isMouseOver = tetrisPointInQuad(mouseX, mouseY, buttonTop, buttonLeft, buttonWidth, buttonHeight);
        bool wasMouseOver = tetrisPointInQuad(previousMouseX, previousMouseY, buttonTop, buttonLeft, buttonWidth, buttonHeight);

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
            if (!tetrisPointInQuad(mouseX, mouseY, buttonTop - releaseThreshold, buttonLeft - releaseThreshold, buttonWidth + (2.0f * releaseThreshold), buttonHeight * (2.0f * releaseThreshold))) {
                tetris->pressedButton = TetrisButtonType_None;
            }

            buttonPadding = 3.0f;
        } else {
            buttonPadding = 5.0f;
        }

        bool doAction = (tetris->pressedButton == buttonType && mouseReleased) || (tetris->hoveredButton == buttonType && enterPressed);

        if (doAction) {
            tetrisPlaySoundEffect(tetris, platform, TetrisSoundEffect_Click);

            switch (buttonType) {
                case TetrisButtonType_Resume: {
                    tetrisResumeGame(tetris, platform);
                } break;
                case TetrisButtonType_New: {
                    tetrisNewGame(tetris, platform);
                } break;
                case TetrisButtonType_Music: {
                    if (tetris->musicOff) {
                        platformResumeSound(platform, PlatformSoundType_Music);
                        tetris->musicOff= false;
                    } else {
                        platformPauseSound(platform, PlatformSoundType_Music);
                        tetris->musicOff= true;
                    }
                } break;
                case TetrisButtonType_Sound: {
                    tetris->effectsOff = !tetris->effectsOff;
                } break;
                case TetrisButtonType_Quit: {
                    tetris->shouldQuit = true;
                } break;
            }
        }

        tetrisDrawCellLikeQuad(
            platform,
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

        BaseColor labelColor = fontColor;

        if (tetris->hoveredButton == buttonType) {
            labelColor = baseMakeColor(100.0f, 100.0f, 100.0f, 255.0f);
        }

        tetrisDrawString(
            tetris,
            platform,
            label,
            buttonLeft + (buttonWidth / 20.0f),
            buttonTop + ((buttonHeight - lineHeight) / 2.0f),
            fontScale,
            labelColor
        );
        buttonTop += (buttonHeight + buttonsGap);
    }

    if (!mouseDown) {
        tetris->pressedButton = TetrisButtonType_None;
    }
}

static void tetrisDoAutoOpenMenu(Tetris *tetris,  Platform *platform, PlatformInput input) {
    if (tetris->secondsToOpenMenu > 0.0f) {
        tetris->secondsToOpenMenu -= input.secondsElapsed;
    }

    if (tetris->secondsToOpenMenu < 0.0f) {
        tetrisOpenMenu(tetris);
        tetris->secondsToOpenMenu = 0.0f;
        if (!tetris->musicOff) {
            platformResumeSound(platform, PlatformSoundType_Music);
        }
    }
}

static void tetrisUpdate(Tetris *tetris, Platform *platform, PlatformInput input, bool *shouldQuit) {
    if (tetris->frame == 0) {
        tetrisNewGame(tetris, platform);
    }

    if (!tetris->menuOpen && tetrisControlPressed(input, PlatformControlType_P)) {
        tetris->paused = !tetris->paused;
    }

    if (tetrisControlPressed(input, PlatformControlType_Esc)) {
        if (tetris->menuOpen) {
            tetrisResumeGame(tetris, platform);
        } else {
            tetrisOpenMenu(tetris);
        }
    }

    #ifdef TETRIS_ENABLE_CHEAT
    uint32_t currentLevel =  tetrisGetCurrentLevel(tetris);
    if (tetrisControlPressed(input, PlatformControlType_L) && !tetrisHasWon(tetris)) {
        tetrisAddClearedRows(tetris, platform, (currentLevel * TETRIS_LINES_PER_LEVEL) - tetris->clearedRows);
    }
    #endif

    TetrisLayout layout = tetrisGetLayout(tetris, input);

    tetrisDoBackground(platform, input, layout);

    BaseColor gridBorderColor = {102.0f, 102.0f, 102.0f, 255.0f};

    tetrisDoGridBorder(platform, layout, gridBorderColor);

    tetrisDoPlayer(tetris, platform, input, layout);

    tetrisDoFading(tetris, platform, input);

    if (tetrisAreRowsFalling(tetris)){
        tetrisDoRowsFalling(tetris, input);
    }

    tetrisDoGrid(tetris, platform, layout);

    if (tetris->gameOver) {
        tetrisDoGameOver(tetris, platform, input, layout, gridBorderColor);
    }

    if (tetrisHasWon(tetris)) {
        tetrisDoCelebrate(tetris,  platform, input);
    }

    tetrisDoAutoOpenMenu(tetris, platform, input);

    BaseColor boxColor = baseMakeColor(223.0f, 240.0f, 216.0f, 255.0f);
    BaseColor fontColor = baseMakeColor(0.0f, 0.0f, 0.0f, 255.0f);

    tetrisDoInfo(tetris, platform, input, layout, boxColor, fontColor);

    if (tetris->menuOpen) {
        tetrisDoMenu(tetris, platform, input, layout, boxColor, fontColor);
    }

    tetris->frame++;
    tetris->previousMouseX = input.mouseX;
    tetris->previousMouseY = input.mouseY;
    tetris->wasResizing = input.isResizing;
    *shouldQuit = tetris->shouldQuit;
}
