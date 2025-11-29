#include <windows.h>

#define TTS_UNREFERENCED(a) a

int WinMain(
    _In_     HINSTANCE instance,
    _In_opt_ HINSTANCE previousInstance,
    _In_     LPSTR     commandLine,
    _In_     int       showCommand
) {
    TTS_UNREFERENCED(instance);
    TTS_UNREFERENCED(previousInstance);
    TTS_UNREFERENCED(commandLine);
    TTS_UNREFERENCED(showCommand);

    OutputDebugStringA("Tetris\n");

    return 0;
}
