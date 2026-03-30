typedef struct Platform Platform;

typedef enum {
    PlatformControlType_None,

    PlatformControlType_Left,
    PlatformControlType_Right,
    PlatformControlType_Up,
    PlatformControlType_Down,
    PlatformControlType_Esc,
    PlatformControlType_Space,
    PlatformControlType_Enter,
    PlatformControlType_C,
    PlatformControlType_P,
    PlatformControlType_L,
    PlatformControlType_MouseLeft,
    PlatformControlType_MouseRight,
    PlatformControlType_MouseCenter,

    PlatformControlType_Count,
} PlatformControlType;

typedef struct {
    uint32_t pressCount;
    uint32_t releaseCount;
    bool isDown;
} PlatformControl;

typedef struct {
    PlatformControl controls[PlatformControlType_Count];

    uint32_t windowWidth;
    uint32_t windowHeight;
    bool isResizing;
    int32_t mouseX;
    int32_t mouseY;
    float secondsElapsed;
} PlatformInput;

typedef struct {
    void* data;
    uint32_t width;
    uint32_t height;
} PlatformTexture;

static void platformMemset(void * pointer, int value, size_t count);

static void *platformAllocate(uint64_t size);

static bool platformReadEntireFile(char *path, BaseArena *arena, BaseReadResult *readResult);

static void platformDebugPrint(_Printf_format_string_ const char *format, ...);

#ifdef PLATFORM_GRAPHICS
static void platformDrawTextureQuad(
    Platform *platform,
    float x, float y,
    float width, float height,
    float xInTexture, float yInTexture,
    float widthInTexture, float heightInTexture,
    float textureWidth, float textureHeight,
    float r, float g, float b, float a
);

static void platformDrawColorTriangle(
    Platform *platform,
    float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    float r, float g, float b, float a
);
#endif

#ifdef PLATFORM_SOUND
typedef enum {
    PlatformSoundType_None,

    PlatformSoundType_Effect,
    PlatformSoundType_Music,

    PlatformSoundType_Count,
} PlatformSoundType;

static void platformPlaySound(Platform *platform, Wav wav, PlatformSoundType soundType);

static void platformPauseSound(Platform *platform, PlatformSoundType soundType);

static void platformResumeSound(Platform *platform, PlatformSoundType soundType);
#endif
