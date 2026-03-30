static void platformDebugPrint(_Printf_format_string_ const char *format, ...) {
    char buffer[1024] = {0};

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(buffer, sizeof(buffer) - 1, format, arguments);
    OutputDebugStringA(buffer);
    va_end(arguments);
}

static LONGLONG win32GetFileSize(HANDLE file) {
    LARGE_INTEGER fileSize = {};

    GetFileSizeEx(file, &fileSize);

    return fileSize.QuadPart;
}

static LONGLONG win32QueryPerformanceFrequency() {
    LARGE_INTEGER performanceFrequency = {0};

    QueryPerformanceFrequency(&performanceFrequency);

    return performanceFrequency.QuadPart;
}

static LONGLONG win32QueryPerformanceCounter() {
    LARGE_INTEGER performanceCounter = {0};

    QueryPerformanceCounter(&performanceCounter);

    return performanceCounter.QuadPart;
}

static bool platformReadEntireFile(char *path, BaseArena *arena, BaseReadResult *readResult) {
    HANDLE file = CreateFile(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    bool result = false;
    if (file != INVALID_HANDLE_VALUE) {
        LONGLONG fileSize = win32GetFileSize(file);

        void *destination = baseArenaPushSize(arena, fileSize);

        LONGLONG remainingBytesToRead = fileSize;

        BOOL ok = 1;

        uint8_t *bytes = (uint8_t *) destination;

        while (ok && remainingBytesToRead > 0) {
            DWORD readSize = remainingBytesToRead > MAXDWORD ? MAXDWORD : (DWORD) remainingBytesToRead;
            DWORD bytesRead = 0;

            ok = ReadFile(file, bytes + fileSize - remainingBytesToRead, readSize, &bytesRead, 0);

            if (ok) {
                remainingBytesToRead -= bytesRead;
            }
        }

        if (remainingBytesToRead == 0) {
            result = true;
            readResult->data = destination;
            readResult->size = fileSize;
        }
        CloseHandle(file);
    }
    return result;
}

static void platformMemset(void * pointer, int value, size_t count) {
    memset(pointer, value, count);
}

static void *platformAllocate(uint64_t size) {
    void *result = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    return result;
}

#ifdef PLATFORM_GRAPHICS
static int win32MapControlToVirtualKey(PlatformControlType controlType) {
    BASE_ASSERT(controlType > PlatformControlType_None);
    BASE_ASSERT(controlType < PlatformControlType_Count);

    int virtualKeys[PlatformControlType_Count] = {
        [PlatformControlType_Left] = VK_LEFT,
        [PlatformControlType_Right] = VK_RIGHT,
        [PlatformControlType_Up] = VK_UP,
        [PlatformControlType_Down] = VK_DOWN,
        [PlatformControlType_Esc] = VK_ESCAPE,
        [PlatformControlType_Space] = VK_SPACE,
        [PlatformControlType_Enter] = VK_RETURN,
        [PlatformControlType_C] = 'C',
        [PlatformControlType_L] = 'L',
        [PlatformControlType_P] = 'P',
        [PlatformControlType_MouseLeft] = VK_LBUTTON,
        [PlatformControlType_MouseRight] = VK_RBUTTON,
        [PlatformControlType_MouseCenter] = VK_MBUTTON,
    };

    int result = virtualKeys[controlType];

    return result;
}

static PlatformControlType win32MapVirtualKeyToControl(int virtualKey) {
    PlatformControlType result = PlatformControlType_None;

    for (PlatformControlType controlType = PlatformControlType_None + 1; controlType < PlatformControlType_Count; controlType++) {
        if (win32MapControlToVirtualKey(controlType) == virtualKey) {
            result = controlType;
            break;
        }
    }

    return result;
}

static void win32Update(AppState *state, Platform *platform, PlatformInput *input) {
    BASE_ASSERT(state);
    BASE_ASSERT(platform);
    RECT rect = {0};
    GetClientRect(platform->window, &rect);
    UINT newWidth = rect.right - rect.left;
    UINT newHeight = rect.bottom - rect.top;

    input->windowWidth = newWidth;
    input->windowHeight = newHeight;

    POINT mousePosition = {0};
    GetCursorPos(&mousePosition);
    ScreenToClient(platform->window, &mousePosition);
    input->mouseX = mousePosition.x;
    input->mouseY = mousePosition.y;

    for (PlatformControlType controlType = PlatformControlType_None + 1; controlType < PlatformControlType_Count; controlType++) {
        int virtualKey = win32MapControlToVirtualKey(controlType);
        input->controls[controlType].isDown = GetAsyncKeyState(virtualKey) < 0;
    }

    LONGLONG currentTicks = win32QueryPerformanceCounter();

    if (platform->previousTicks) {
        LONGLONG elapsedTicks = currentTicks - platform->previousTicks;

        input->secondsElapsed = (float) elapsedTicks / (float)platform->performanceFrequency;
    }

    platform->previousTicks = currentTicks;

    bool shouldQuit = false;
    APP_UPDATE(state, platform, *input, &shouldQuit);

    if (shouldQuit) {
        PostQuitMessage(0);
    }

    d3d11Render(platform, platform->windowWidth, platform->windowHeight, newWidth, newHeight);
    platform->windowWidth = newWidth;
    platform->windowHeight = newHeight;
    platformMemset(input, 0, sizeof(PlatformInput));
}
#endif
