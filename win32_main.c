#include <stdint.h>
#include <stdbool.h>

#include "base.h"
#include "wav.h"
#include "atlas.h"
#include "platform.h"
#include "tetris.h"
#include "app.h"

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

LRESULT windowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    LRESULT result = 0;

    Win32WindowProcParams *windowProcParams = (Win32WindowProcParams *)GetWindowLongPtrA(window, GWLP_USERDATA);

    if (!windowProcParams) {
        result = DefWindowProcA(window, message, wParam, lParam);
    } else {
        switch (message) {
            case WM_PAINT: {
                PAINTSTRUCT paint = {0};
                BeginPaint(window, &paint);
                win32Update(&windowProcParams->state, windowProcParams->platform, &windowProcParams->input);
                EndPaint(window, &paint);
            } break;

            case WM_SIZE: {
                win32Update(&windowProcParams->state, windowProcParams->platform, &windowProcParams->input);
            } break;

            case WM_ENTERSIZEMOVE: {
                windowProcParams->input.isResizing = true;
            } break;

            case WM_EXITSIZEMOVE: {
                windowProcParams->input.isResizing = false;
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
                        windowProcParams->input.controls[controlType].pressCount++;
                    }

                    if (message == WM_KEYUP) {
                        windowProcParams->input.controls[controlType].releaseCount++;
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
                        windowProcParams->input.controls[controlType].pressCount++;
                    } else {
                        windowProcParams->input.controls[controlType].releaseCount++;
                    }
                }
            } break;

            default: {
                result = DefWindowProcA(window, message, wParam, lParam);
            }
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
    BASE_UNREFERENCED(previousInstance);
    BASE_UNREFERENCED(commandLine);
    BASE_UNREFERENCED(showCommand);

    PlatformTexture texture = {0};
    Win32WindowProcParams windowProcParams = {0};

    FILETIME systemTime = {0};
    GetSystemTimePreciseAsFileTime(&systemTime);
    DWORD seed = systemTime.dwLowDateTime;

    if (APP_INIT(&windowProcParams.state, sizeof(Platform), seed, &windowProcParams.platform, &texture)){
        WNDCLASSEXA windowClass = {0};

        windowProcParams.platform->performanceFrequency = win32QueryPerformanceFrequency();

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
            windowProcParams.platform->window = CreateWindowExA(
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
                &windowProcParams
            );

            if (windowProcParams.platform->window) {
                SetWindowLongPtrA(windowProcParams.platform->window, GWLP_USERDATA, (LONG_PTR) &windowProcParams);

                xaudio2Init(windowProcParams.platform);

                if (d3d11Init(windowProcParams.platform, texture)) {
                    ShowWindow(windowProcParams.platform->window, SW_MAXIMIZE);

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
                            win32Update(&windowProcParams.state, windowProcParams.platform, &windowProcParams.input);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
