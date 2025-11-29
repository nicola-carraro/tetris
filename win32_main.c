#include <windows.h>

#pragma comment(lib, "User32")

#define TTS_UNREFERENCED(a) a

LRESULT windowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    LRESULT result = 0;
    switch (message) {
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;

        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        }
    }

    return result;
}

int WinMain(
    _In_     HINSTANCE instance,
    _In_opt_ HINSTANCE previousInstance,
    _In_     LPSTR     commandLine,
    _In_     int       showCommand
) {
    TTS_UNREFERENCED(previousInstance);
    TTS_UNREFERENCED(commandLine);
    TTS_UNREFERENCED(showCommand);

    WNDCLASSEXA windowClass = {0};
    char className[] = "tetris";
    {
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = 0;
        windowClass.lpfnWndProc = windowProc;
        windowClass.cbClsExtra = 0;
        windowClass.cbWndExtra = 0;
        windowClass.hInstance = instance;
        windowClass.hIcon = 0;
        windowClass.hCursor = LoadCursor(0, IDC_ARROW);
        windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        windowClass.lpszMenuName = 0;
        windowClass.lpszClassName = className;
        windowClass.hIconSm = 0;
    }

    if (RegisterClassExA(&windowClass)) {
        HWND window = CreateWindowExA(
            WS_EX_NOREDIRECTIONBITMAP,
            className,
            "Tetris",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0
        );

        if (window) {
            ShowWindow(window, showCommand);

            MSG message = {0};
            BOOL ok = 0;
            while ((ok = GetMessage(&message, 0, 0, 0))) {
                if (ok == -1) {
                    break;
                } else {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
            }
        }
    }

    DWORD error = GetLastError();
    TTS_UNREFERENCED(error);

    return 0;
}
