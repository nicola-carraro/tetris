#define TETRIS_COLUMN_COUNT 10
#define TETRIS_ROW_COUNT    19
#define TETRIS_MAX_WIDTH_RATIO 0.8f
#define TETRIS_MAX_HEIGTH_RATIO 0.8f
#define TETRIS_FADE_SECONDS 0.5f
#define TETRIS_DATA_DIR "../data/"
#define TETRIS_ALLOCATION_SIZE (512 * 1024 * 1024)
#define TETRIS_LEVEL_COUNT_PLUS_ONE 16
#define TETRIS_LINES_PER_LEVEL 10

typedef enum {
    TetrisHorizontalDirection_None,

    TetrisHorizontalDirection_Left,
    TetrisHorizontalDirection_Right,

    TetrisHorizontalDirection_Count,
} TetrisHorizontalDirection;

typedef enum  {
    TetrisPieceType_None,

    TetrisPieceType_I,
    TetrisPieceType_O,
    TetrisPieceType_T,
    TetrisPieceType_L,
    TetrisPieceType_J,
    TetrisPieceType_Z,
    TetrisPieceType_S,

    TetrisPieceType_Count,
} TetrisPieceType;

typedef enum {
    TetrisRotationType_None,

    TetrisRotationType_S,
    TetrisRotationType_R,
    TetrisRotationType_2,
    TetrisRotationType_L,

    TetrisRotationType_Count,
} TetrisRotationType;

typedef struct {
    bool flipCoordinates;
    bool flipSign;
} TetrisRotation;

typedef struct Platform Platform;

typedef struct {
    float x;
    float y;
} TetrisFloatCoords;

typedef struct {
    int32_t x;
    int32_t y;
} TetrisI32Coords;

typedef struct {
    TetrisFloatCoords cellCenters[4];
} TetrisPiecePattern;

typedef struct {
    TetrisI32Coords cells[4];
} TetrisPiece;

typedef struct {
    TetrisFloatCoords esteticCenter;
    float minX;
    float minY;
    float maxX;
    float maxY;
    float width;
    float height;
} TetrisPatternFeatures;

typedef enum {
    TetrisSoundEffect_None,

    TetrisSoundEffect_Whoosh,
    TetrisSoundEffect_Click,
    TetrisSoundEffect_GameOver,
    TetrisSoundEffect_Yay,
    TetrisSoundEffect_LevelUp,

    TetrisSoundEffect_Count,
} TetrisSoundEffect;

typedef enum {
    TetrisMusic_None,

    TetrisMusic_Theme,
    TetrisMusic_Celebrate,

    TetrisMusic_Count,
} TetrisMusic;

typedef enum {
    TetrisButtonType_None,

    TetrisButtonType_Resume,
    TetrisButtonType_New,
    TetrisButtonType_Sound,
    TetrisButtonType_Music,
    TetrisButtonType_Quit,

    TetrisButtonType_Count,
} TetrisButtonType;

typedef struct {
    BaseColor colors[TetrisPieceType_Count];
} TetrisColorScheme;

typedef struct  {
    float gridX;
    float gridY ;
    float gridWidth;
    float gridHeight;
    float boxWidth;
    float boxHeight;
    float gridMargin;
    float cellSideInPixels;
    bool drawLabels;
}TetrisLayout;

typedef struct {
    Atlas atlas;
    Wav musics[TetrisMusic_Count];
    Wav soundEffects[TetrisSoundEffect_Count];
    BaseArena arena;
    uint64_t frame;
    uint32_t score;
    uint32_t clearedRows;
    uint32_t seed;
    int32_t previousMouseX;
    int32_t previousMouseY;
    float playerXInCells;
    float playerYInCells;
    float playerXProgression;
    float playerYProgression;
    float fallingYProgression;
    float secondsToNextGameOverAnimation;
    float secondsToOpenMenu;
    bool paused;
    bool musicOff;
    bool effectsOff;
    bool shouldQuit;
    bool gameOver;
    bool wasResizing;
    bool menuOpen;
    bool isHardDropping;
    bool isSoftDropping;
    TetrisPieceType playerType;
    TetrisPieceType nextPlayerType;
    TetrisPieceType grid[TETRIS_ROW_COUNT][TETRIS_COLUMN_COUNT];
    TetrisRotationType playerRotationType;
    TetrisHorizontalDirection horizontalDirection;
    float secondsToFadeEnd;
    int32_t fadingRows[4];
    int32_t fadingRowsCount;
    TetrisButtonType pressedButton;
    TetrisButtonType hoveredButton;
    uint16_t gameOverAnimationSteps;
} Tetris;
