#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

#include "base.h"
#include "wav.h"
#include "atlas.h"
#include "platform.h"
#include "tetris.h"
#include "app.h"

#include <sanitizer/asan_interface.h>
#include "base.c"

#include "wav.c"
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

#include "win32.h"
#include "d3d11.h"

#include "win32.c"
#include "d3d11.c"
#include "xaudio2.c"

#pragma comment(lib, "User32")
#pragma comment(lib, "D3D11")
#pragma comment(lib, "DXGI")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "Ole32")

static Win32WindowProcParams GlobalWindowProcParams = {0};

LRESULT CALLBACK windowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    LRESULT result = 0;

    switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT paint = {0};
            BeginPaint(window, &paint);
            win32Update(&GlobalWindowProcParams.state, GlobalWindowProcParams.platform, &GlobalWindowProcParams.input);
            EndPaint(window, &paint);
        } break;

        case WM_SIZE: {
            win32Update(&GlobalWindowProcParams.state, GlobalWindowProcParams.platform, &GlobalWindowProcParams.input);
        } break;

        case WM_ENTERSIZEMOVE: {
            GlobalWindowProcParams.input.isResizing = true;
        } break;

        case WM_EXITSIZEMOVE: {
            GlobalWindowProcParams.input.isResizing = false;
        } break;

        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;

        case WM_KEYUP:
        case WM_KEYDOWN: {
            PlatformControlType controlType = win32MapVirtualKeyToControl((int)wParam);

            if (controlType) {
                bool wasDown = ((lParam >> 30) & 1);
                if (message == WM_KEYDOWN && !wasDown) {
                    GlobalWindowProcParams.input.controls[controlType].pressCount++;
                }

                if (message == WM_KEYUP) {
                    GlobalWindowProcParams.input.controls[controlType].releaseCount++;
                }
            }
        } break;

        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDOWN: {
            PlatformControlType controlType = PlatformControlType_None;

            switch(message) {
                case WM_LBUTTONUP:
                case WM_LBUTTONDOWN:
                {
                    controlType = PlatformControlType_MouseLeft;
                } break;
                case WM_RBUTTONUP:
                case WM_RBUTTONDOWN:
                {
                    controlType = PlatformControlType_MouseRight;
                } break;
                case WM_MBUTTONUP:
                case WM_MBUTTONDOWN:{
                    controlType = PlatformControlType_MouseCenter;
                } break;
            }

            if (controlType) {
                bool isDownMessage = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN;
                if (isDownMessage) {
                    GlobalWindowProcParams.input.controls[controlType].pressCount++;
                } else {
                    GlobalWindowProcParams.input.controls[controlType].releaseCount++;
                }
            }
        } break;

        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        }
    }

    return result;
}

int __stdcall WinMain(
    _In_     HINSTANCE instance,
    _In_opt_ HINSTANCE previousInstance,
    _In_     LPSTR     commandLine,
    _In_     int       showCommand
) {
    BASE_UNREFERENCED(previousInstance);
    BASE_UNREFERENCED(commandLine);
    BASE_UNREFERENCED(showCommand);

    PlatformTexture texture = {0};

    FILETIME systemTime = {0};
    GetSystemTimePreciseAsFileTime(&systemTime);
    DWORD seed = systemTime.dwLowDateTime;

    if (APP_INIT(&GlobalWindowProcParams.state, sizeof(Platform), seed, &GlobalWindowProcParams.platform, &texture)){
        xaudio2Init(GlobalWindowProcParams.platform);
        WNDCLASSEXA windowClass = {0};

        GlobalWindowProcParams.platform->performanceFrequency = win32QueryPerformanceFrequency();

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
            GlobalWindowProcParams.platform->window = CreateWindowExA(
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

            if (GlobalWindowProcParams.platform->window) {
                if (d3d11Init(GlobalWindowProcParams.platform, texture)) {
                    ShowWindow(GlobalWindowProcParams.platform->window, SW_MAXIMIZE);

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
                            win32Update(&GlobalWindowProcParams.state, GlobalWindowProcParams.platform, &GlobalWindowProcParams.input);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
