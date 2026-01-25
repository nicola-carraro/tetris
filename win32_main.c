#include <stdint.h>
#include <stdbool.h>

#include "tetris.h"
#include "platform.h"
#include "tetris.c"

#define COBJMACROS

#pragma warning(push, 0)
#include <windows.h>
#include <initguid.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <xaudio2.h>
#include <stdio.h>
#pragma warning(pop)

#include "win32.c"
#include "d3d11.c"
#include "xaudio2.c"

#pragma comment(lib, "User32")
#pragma comment(lib, "D3D11")
#pragma comment(lib, "DXGI")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "Ole32")

LRESULT windowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    LRESULT result = 0;
    TtsTetris *tetris = (TtsTetris *)GetWindowLongPtrA(window, GWLP_USERDATA);
    switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT paint = {0};
            BeginPaint(window, &paint);
            win32Update(tetris);
            EndPaint(window, &paint);
        } break;

        case WM_SIZE: {
            win32Update(tetris);
        } break;

        case WM_ENTERSIZEMOVE: {
            tetris->wasResizing = true;
            tetris->isResizing = true;
        } break;

        case WM_EXITSIZEMOVE: {
            tetris->isResizing = false;
        } break;

        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;

        case WM_KEYUP:
        case WM_KEYDOWN: {
            TtsControlType controlType = TtsControlType_None;

            switch (wParam) {
                case VK_LEFT : {
                    controlType = TtsControlType_Left;
                } break;

                case VK_RIGHT: {
                    controlType = TtsControlType_Right;
                } break;

                case VK_UP: {
                    controlType = TtsControlType_Up;
                } break;

                case VK_DOWN: {
                    controlType = TtsControlType_Down;
                } break;

                case VK_ESCAPE: {
                    controlType = TtsControlType_Esc;
                } break;

                case VK_SPACE: {
                    controlType = TtsControlType_Space;
                } break;

                case VK_RETURN: {
                    controlType = TtsControlType_Enter;
                } break;

                case 'C': {
                    controlType = TtsControlType_C;
                } break;

                case 'P': {
                    controlType = TtsControlType_P;
                }
            }

            if (controlType) {
                TtsControl *control = &tetris->controls[controlType];
                control->endedDown = message == WM_KEYDOWN;
                control->wasDown = true;
                bool wasDown = (wParam >> 30) & 1;

                if (message == WM_KEYUP || !wasDown) {
                    control->transitions++;
                }
            }
        } break;

        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDOWN: {
            TtsControlType controlType = TtsControlType_None;

            switch(message) {
                case WM_LBUTTONUP:
                case WM_LBUTTONDOWN:
                {
                    controlType = TtsControlType_MouseLeft;
                } break;
                case WM_RBUTTONUP:
                case WM_RBUTTONDOWN:
                {
                    controlType = TtsControlType_MouseRight;
                } break;
                case WM_MBUTTONUP:
                case WM_MBUTTONDOWN:{
                    controlType = TtsControlType_MouseCenter;
                } break;
            }

            if (controlType) {
                TtsControl *control = &tetris->controls[controlType];
                bool isDownMessage = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN;
                if (isDownMessage) {
                    control->wasDown = true;
                }

                control->endedDown = isDownMessage;

                control->transitions++;
            }
        } break;

        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        }
    }

    return result;
}

#pragma warning(push)
#pragma warning(disable :6262)
int WinMain(
    _In_     HINSTANCE instance,
    _In_opt_ HINSTANCE previousInstance,
    _In_     LPSTR     commandLine,
    _In_     int       showCommand
) {
    TTS_UNREFERENCED(previousInstance);
    TTS_UNREFERENCED(commandLine);
    TTS_UNREFERENCED(showCommand);
    TtsPlatform win32 = {0};

    WNDCLASSEXA windowClass = {0};

    win32.performanceFrequency = win32QueryPerformanceFrequency();

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
        windowClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
        windowClass.lpszMenuName = 0;
        windowClass.lpszClassName = className;
        windowClass.hIconSm = 0;
    }

    if (RegisterClassExA(&windowClass)) {
        win32.window = CreateWindowExA(
            0,
            className,
            "Tetris",
            WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0
        );
        bool hasSound = xaudio2Init(&win32);
        TtsTetris tetris = ttsInit(&win32, hasSound);

        FILETIME systemTime = {0};
        GetSystemTimePreciseAsFileTime(&systemTime);

        tetris.seed = systemTime.dwLowDateTime;

        if (win32.window && d3d11Init(&tetris)) {
            SetWindowLongPtrA(win32.window, GWLP_USERDATA, (LONG_PTR) &tetris);
            ShowWindow(win32.window, SW_MAXIMIZE);

            for (BOOL running = 1; running;) {
                MSG message = {0};
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        running = 0;
                        break;
                    } else {
                        TranslateMessage(&message);
                        DispatchMessageA(&message);
                    }
                }

                if (running) {
                    win32Update(&tetris);
                }
            }
        }
    }
    return 0;
}
#pragma warning(pop)
