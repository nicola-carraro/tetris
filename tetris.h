#define TTS_ASSERT(a) do {if (!(a)) { __debugbreak();}} while (0);
#define TTS_QUOTE(s) #s
#define TTS_ARRAYCOUNT(a) (sizeof(a) / sizeof(*a))
#define TTS_UNREFERENCED(a) a
#define TTS_FIRST_CODEPOINT L' '
#define TTS_LAST_CODEPOINT  L'~'
#define TTS_CODEPOINT_COUNT (TTS_LAST_CODEPOINT - TTS_FIRST_CODEPOINT + 1)
#define TTS_PIXELS_PER_POINT 1.33333333333333333f
#define TTS_POINTS_PER_PIXEL 0.75f
#define TTS_MAKE_STRING(a) ttsMakeString((a), (sizeof(a) - 1))
#define TTS_FONT_PATH L"../data/Handjet-Regular.ttf"
#define TTS_ATLAS_PATH "../data/atlas.dat"
#define TTS_COLUMN_COUNT 10
#define TTS_ROW_COUNT    19
#define TTS_MAX_WIDTH_RATIO 0.8f
#define TTS_MAX_HEIGTH_RATIO 0.8f
#define TTS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define TTS_MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct TtsPlatform TtsPlatform;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} TtsColor;

typedef struct {
    char *text;
    uint64_t size;
} TtsString;

typedef struct {
    uint32_t codepoint;
    uint16_t index;
    float xOffsetInPixels;
    float yOffsetInPixels;
    float advanceWidthInPixels;
    float bitmapXInPixels;
    float bitmapYInPixels;
    float bitmapWidthInPixels;
    float bitmapHeightInPixels;
} TtsGlyph;

typedef struct {
    float width;
    float height;
    float lineHeightInPixels;
    TtsGlyph glyphs[TTS_CODEPOINT_COUNT];
} TtsAtlas;

typedef struct {
    void *data;
    uint64_t size;
} TtsReadResult;

typedef struct {
    bool wasDown;
    bool endedDown;
    uint32_t transitions;
} TtsControl;

typedef enum {
    TtsControlType_None,

    TtsControlType_Left,
    TtsControlType_Right,
    TtsControlType_Up,
    TtsControlType_Down,
    TtsControlType_Esc,
    TtsControlType_Space,
    TtsControlType_Enter,
    TtsControlType_C,
    TtsControlType_P,
    TtsControlType_MouseLeft,
    TtsControlType_MouseRight,
    TtsControlType_MouseCenter,

    TtsControlType_Count,
} TtsControlType;

typedef enum {
    TtsHorizontalDirection_None,

    TtsHorizontalDirection_Left,
    TtsHorizontalDirection_Right,

    TtsHorizontalDirection_Count,
} TtsHorizontalDirection;

typedef enum  {
    TtsTetraminoType_None,

    TtsTetraminoType_I,
    TtsTetraminoType_O,
    TtsTetraminoType_T,
    TtsTetraminoType_L,
    TtsTetraminoType_J,
    TtsTetraminoType_Z,
    TtsTetraminoType_S,

    TtsTetraminoType_Count,
} TtsTetraminoType;

typedef enum {
    TtsRotationType_None,

    TtsRotationType_S,
    TtsRotationType_R,
    TtsRotationType_2,
    TtsRotationType_L,

    TtsRotationType_Count,
} TtsRotationType;

typedef struct {
    bool flipCoordinates;
    bool flipSign;
} TtsRotation;

typedef struct {
    uint32_t chunkId;
    uint32_t chunkSize;
    uint32_t waveId;
} RiffChunk;

typedef struct {
    uint32_t chunkId;
    uint32_t chunkSize;
} WavChunkHeader;

typedef union {
    uint32_t  n;
    char s[4];
} WavTag;

typedef struct guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} Guid;

#pragma pack(push, 1)
typedef struct {
    uint16_t  formatTag;
    uint16_t  channels;
    uint32_t  samplesPerSec;
    uint32_t  avgBytesPerSec;
    uint16_t  blockAlign;
    uint16_t  bitsPerSample;
    uint16_t  extensionSize;
    uint16_t  validBitsPerSample;
    uint32_t  channelMask;
    Guid subFormat;
} WavFmtChunk;
#pragma pack(pop)

typedef struct {
    RiffChunk   *riffChunk;
    WavFmtChunk *fmtChunk;
    void        *data;
    uint32_t         dataSize;
} Wav;

typedef struct Platform Platform;

typedef struct {
    float x;
    float y;
} TtsFloatCoords;

typedef struct {
    int32_t x;
    int32_t y;
} TtsI32Coords;

typedef struct {
    TtsFloatCoords cellCenters[4];
} TtsTetraminoPattern;

typedef struct {
    TtsI32Coords cells[4];
} TtsTetramino;

typedef struct {
    TtsFloatCoords esteticCenter;
    float minX;
    float minY;
    float maxX;
    float maxY;
    float width;
    float height;
} TtsPatternFeatures;

typedef enum {
    TtsSoundEffect_None,

    TtsSoundEffect_Whoosh,
    TtsSoundEffect_Click,

    TtsSoundEffect_Count,
} TtsSoundEffect;

typedef struct {
    TtsPlatform *platform;
    TtsAtlas atlas;
    uint32_t windowWidth;
    uint32_t windowHeight;
    bool isResizing;
    bool wasResizing;
    bool hasSound;
    Wav music;
    Wav soundEffects[TtsSoundEffect_Count];
    uint64_t frame;
    TtsControl controls[TtsControlType_Count];
    int32_t mouseX;
    int32_t mouseY;
    TtsColor backgroundColor;
    float playerXInCells;
    float playerYInCells;
    float playerXProgression;
    float playerYProgression;
    bool paused;
    TtsTetraminoType playerType;
    TtsTetraminoType nextPlayerType;
    TtsTetraminoType grid[TTS_ROW_COUNT][TTS_COLUMN_COUNT];
    TtsRotationType playerRotationType;
    TtsHorizontalDirection horizontalDirection;
    uint32_t score;
    uint32_t clearedLines;
    uint32_t seed;
    bool isHardDropping;
} TtsTetris;
