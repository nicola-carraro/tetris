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

typedef struct TtsPlatform TtsPlatform;

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
	bool isDown;
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
    TtsControlType_MouseLeft,
    TtsControlType_MouseRight,
    TtsControlType_MouseCenter,

    TtsControlType_Count,	
} TtsControlType;


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
    TtsPlatform *platform;
    TtsAtlas atlas;
    uint32_t windowWidth;
    uint32_t windowHeight;
    bool isResizing;
    bool wasResizing;
    bool hasSound;
	Wav music;
	Wav sound;
	uint64_t frame;
    TtsControl controls[TtsControlType_Count];
	int32_t mouseX;
	int32_t mouseY;
} TtsTetris;

typedef struct {
    int32_t x;
    int32_t y;
} TtsV2I32;
