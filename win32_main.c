#include <stdint.h>
#include "platform.h"
#include "tetris.h"
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
#pragma warning(pop)

#include "win32.c"

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
    Platform *win32 = (Platform *)GetWindowLongPtrA(window, GWLP_USERDATA);
    switch (message) {
        // case WM_CLOSE: {
        // PostThreadMessageA(GlobalThreadId, message, wParam, lParam);
        // Platform *win32 = (Platform *)GetWindowLongPtrA(window, GWLP_USERDATA);
        // WaitForSingleObject(win32->threadEvent, 1000);
        // DestroyWindow(window);
        // } break;

        case WM_PAINT: {
            PAINTSTRUCT paint = {0};
            BeginPaint(window, &paint);
            win32Update(win32);
            EndPaint(window, &paint);
        } break;

        case WM_SIZE: {
            win32Update(win32);
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

DWORD threadProc(LPVOID parameter) {
    Platform *win32 = (Platform *)parameter;

    MSG message = {0};

    if (win32D3d11Init(win32)) {
        // Create message queue and signal it to the window thread
        {
            PeekMessage(&message, 0, WM_USER, WM_USER, PM_NOREMOVE);
            SetEvent(win32->threadEvent);
        }

        for (BOOL running = 1; running;) {
            while (running && PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                switch (message.message) {
                    case WM_CHAR: {
                        OutputDebugStringA("WM_CHAR\n");
                    } break;
                    case WM_CLOSE: {
                        running = 0;
                    } break;
                }
            }

            if (running) {
                win32Update(win32);
            }
        }

        SetEvent(win32->threadEvent);
    }

    return 0;
}

int WinMain(
    _In_     HINSTANCE instance,
    _In_opt_ HINSTANCE previousInstance,
    _In_     LPSTR     commandLine,
    _In_     int       showCommand
) {
    TTS_UNREFERENCED(previousInstance);
    TTS_UNREFERENCED(commandLine);

    Platform win32 = {0};

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

        if (win32.window && win32D3d11Init(&win32)) {
            //win32.threadEvent = CreateEventA(0, 0, 0, 0);
            SetWindowLongPtrA(win32.window, GWLP_USERDATA, (LONG_PTR) &win32);
            ShowWindow(win32.window, showCommand);
            // Wait until the thread has created its message queue
            // if (
            // CreateThread(0, 0, threadProc, &win32, 0, &GlobalThreadId)
            // && WAIT_OBJECT_0 == WaitForSingleObject(win32.threadEvent, 1000)
            // ) {

            // MSG message = {0};
            // BOOL ok = 0;
            // while ((ok = GetMessage(&message, 0, 0, 0))) {
            // if (ok == -1) {
            // break;
            // } else {
            // TranslateMessage(&message);
            // DispatchMessageA(&message);
            // }
            // }
            // }

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

                win32Update(&win32);
            }
        }
    }
    return 0;
}
