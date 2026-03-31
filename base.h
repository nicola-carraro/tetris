#define BASE_ASSERT(a) do {if (!(a)) { __debugbreak();}} while (0);
#define BASE_QUOTE(s) #s
#define BASE_ARRAYCOUNT(a) (sizeof(a) / sizeof(*a))
#define BASE_UNREFERENCED(a) a
#define BASE_MAKE_STRING(a) baseMakeString((a), (sizeof(a) - 1))
#define BASE_MAX(a, b) ((a) > (b) ? (a) : (b))
#define BASE_MIN(a, b) ((a) < (b) ? (a) : (b))
#define BASE_ALIGNEMENT 8

typedef struct {
    char *text;
    uint64_t size;
} BaseString;

typedef struct {
    uint64_t used;
    uint64_t capacity;
    void *buffer;
} BaseArena;

typedef struct {
    void *data;
    uint64_t size;
} BaseReadResult;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} BaseColor;
