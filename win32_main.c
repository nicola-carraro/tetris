#include "tetris.c"

#define COBJMACROS

#pragma warning(push, 0)
#include <windows.h>
#include <initguid.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#pragma warning(pop)

#include "win32.c"

#pragma comment(lib, "User32")
#pragma comment(lib, "D3D11")
#pragma comment(lib, "DXGI")

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
    Win32 *win32 = (Win32 *)parameter;

    if (win32D3d11Init(win32)) {
        MSG message = {0};
        // Create message queue and signal it to the window thread
        {
            PeekMessage(&message, 0, WM_USER, WM_USER, PM_NOREMOVE);
            SetEvent(win32->threadReady);
        }

        for (BOOL running = 1; running;) {
            while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                switch (message.message) {
                    case WM_CHAR: {
                        OutputDebugStringA("WM_CHAR\n");
                    } break;
                }
            }

            if (running) {
                RECT rect = {0};
                GetClientRect(win32->window, &rect);

                UINT newWidth = rect.right - rect.left;
                UINT newHeight = rect.bottom - rect.top;

                win32D3d11Render(win32, newWidth, newHeight);
            }
        }
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

    Win32 win32 = {0};

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
        win32.window = CreateWindowExA(
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

        if (win32.window) {
            win32.threadReady = CreateEventA(0, 0, 0, 0);
            // Wait until the thread has created its message queue
            if (
                CreateThread(0, 0, threadProc, &win32, 0, &GlobalThreadId)
                && WAIT_OBJECT_0 == WaitForSingleObject(win32.threadReady, 1000)
            ) {
                ShowWindow(win32.window, showCommand);

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
    }
    return 0;
}
