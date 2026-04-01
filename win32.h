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
} Win32Vertex;

typedef struct {
    float windowWidth;
    float windowHeight;
    double padding;
} Win32VsConstants;

typedef struct  {
    Win32Vertex vertices[64 * 1024];
    UINT vertexCount;
} Win32Vertices;

typedef struct {
    AppState state;
    PlatformInput input;
    Platform *platform;
} Win32WindowProcParams;

struct Platform {
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
	bool hasGraphics;

    #ifdef PLATFORM_SOUND
    IXAudio2 *xaudio;
    IXAudio2MasteringVoice *masteringVoice;
    IXAudio2SourceVoice *music;
    IXAudio2SourceVoice *effects;
    Win32Vertices vertices;
    #endif

    LONGLONG performanceFrequency;
    LONGLONG previousTicks;
    bool hasSound;
    UINT windowWidth;
    UINT windowHeight;
};
