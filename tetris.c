#define TTS_ASSERT(a) do {if (!(a)) { __debugbreak();}} while (0);
#define TTS_QUOTE(s) #s
#define TTS_ARRAYCOUNT(a) (sizeof(a) / sizeof(*a))
#define TTS_UNREFERENCED(a) a

void ttsUpdate(Platform *platform) {
    platformDrawQuad(
        10.0f, 10.0f,
        100.0f, 50.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        platform
    );
}
