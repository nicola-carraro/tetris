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
#include <stdio.h>
#pragma warning(pop)

#include "win32.c"
#include "d3d11.c"

#pragma comment(lib, "User32")
#pragma comment(lib, "D3D11")
#pragma comment(lib, "DXGI")
#pragma comment(lib, "d3dcompiler")

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

        case WM_CHAR: {
            PostThreadMessageA(GlobalThreadId, message, wParam, lParam);
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

    TtsTetris tetris = {0};
    TtsPlatform win32 = {0};
    tetris.platform = &win32;

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

        if (win32.window && d3d11Init(&tetris)) {
            SetWindowLongPtrA(win32.window, GWLP_USERDATA, (LONG_PTR) &tetris);
            ShowWindow(win32.window, showCommand);

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
