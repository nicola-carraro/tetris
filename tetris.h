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

typedef struct {
	uint32_t width;
	uint32_t height;
} Atlas;

typedef struct {
	void *data;
	uint64_t size;
} ReadResult;