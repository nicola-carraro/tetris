static void d3d11Render(TtsTetris *tetris, UINT newWidth, UINT newHeight);

typedef struct {
    float x;
    float y;
    float u;
    float v;
    float mask;
    float r;
    float g;
    float b;
    float a;
} Vertex;

typedef struct {
    float windowWidth;
    float windowHeight;
    double padding;
} VsConstants;

typedef struct  {
    Vertex vertices[1024];
    UINT vertexCount;
} Vertices;

struct TtsPlatform {
    HWND window;
    ID3D11Device *device;
    ID3D11DeviceContext *deviceContext;
    ID3D11RenderTargetView *renderTargetView;
    IDXGISwapChain1 *swapChain;
    ID3D11VertexShader *vertexShader;
    ID3D11Buffer *vertexBuffer;
    ID3D11PixelShader *pixelShader;
    ID3D11InputLayout *inputLayout;
    ID3D11Buffer *constantBuffer;
    ID3D11SamplerState *samplerState;
    ID3D11ShaderResourceView *textureView;
    ID3D11BlendState *blendState;
    IXAudio2 *xaudio;
    IXAudio2MasteringVoice *masteringVoice;
    IXAudio2SourceVoice *music;
    IXAudio2SourceVoice *sound;
    Vertices vertices;
    LONGLONG performanceFrequency;
    LONGLONG previousTicks;
};

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

static TtsReadResult platformReadEntireFile(char *path) {
    TtsReadResult result = {0};

    HANDLE file = CreateFile(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (file != INVALID_HANDLE_VALUE) {
        LONGLONG fileSize = win32GetFileSize(file);

        void *destination = VirtualAlloc(
            0,
            fileSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE
        );

        if (destination) {
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
                result.data = destination;
                result.size = fileSize;
            }
            CloseHandle(file);
        }
    }
    return result;
}

static void win32Update(TtsTetris *tetris) {
    TtsPlatform *win32 = tetris->platform;
    RECT rect = {0};
    GetClientRect(win32->window, &rect);
    UINT newWidth = rect.right - rect.left;
    UINT newHeight = rect.bottom - rect.top;

    POINT mousePosition = {0};
    GetCursorPos(&mousePosition);
    ScreenToClient(tetris->platform->window, &mousePosition);
    tetris->mouseX = mousePosition.x;
    tetris->mouseY = mousePosition.y;

    float secondsElapsed = 0.0f;

    LONGLONG currentTicks = win32QueryPerformanceCounter();

    if (win32->previousTicks) {
        LONGLONG elapsedTicks = currentTicks - win32->previousTicks;

        secondsElapsed = (float) elapsedTicks / (float)win32->performanceFrequency;
    }

    win32->previousTicks = currentTicks;

    ttsUpdate(tetris, secondsElapsed);

    d3d11Render(tetris, newWidth, newHeight);
    tetris->windowWidth = newWidth;
    tetris->windowHeight = newHeight;
    tetris->wasResizing = tetris->isResizing;
}
