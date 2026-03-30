#define ATLAS_FIRST_CODEPOINT L' '
#define ATLAS_LAST_CODEPOINT  L'~'
#define ATLAS_CODEPOINT_COUNT (ATLAS_LAST_CODEPOINT - ATLAS_FIRST_CODEPOINT + 1)
#define ATLAS_PIXELS_PER_POINT 1.33333333333333333f
#define ATLAS_POINTS_PER_PIXEL 0.75f
#define ATLAS_FONT_PATH L"../data/Handjet-Regular.ttf"
#define ATLAS_PATH "../data/atlas.dat"

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
} AtlasGlyph;

typedef struct {
    float width;
    float height;
    float lineHeightInPixels;
    AtlasGlyph glyphs[ATLAS_CODEPOINT_COUNT];
} Atlas;

typedef struct {
    uint32_t codepoint;
    uint32_t heightInDesignUnits;
} AtlasGlyphHeight;
