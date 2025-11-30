typedef struct {
    HWND window;
    HANDLE threadReady;
    ID3D11Device *device;
    ID3D11DeviceContext *deviceContext;
    ID3D11RenderTargetView *renderTargetView;
    IDXGISwapChain1 *swapChain;
    UINT width;
    UINT height;
} Win32;

static DWORD GlobalThreadId = 0;

BOOL win32D3d11Init(Win32 *win32) {
    HRESULT hr = E_FAIL;
    BOOL ok = 0;
    UINT flags = 0;

    IDXGIFactory2 *factory = 0;
    hr = CreateDXGIFactory(&IID_IDXGIFactory2, (void **)&factory);
    ok = SUCCEEDED(hr);
    if (ok) {
        #ifdef TTS_DEBUG
        flags = D3D11_CREATE_DEVICE_DEBUG;
        {
            IDXGIInfoQueue *infoQueue;
            hr = DXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (void**)&infoQueue);
            TTS_ASSERT(SUCCEEDED(hr));

            hr = IDXGIInfoQueue_SetBreakOnSeverity(infoQueue, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, 1);
            TTS_ASSERT(SUCCEEDED(hr));

            hr = IDXGIInfoQueue_SetBreakOnSeverity(infoQueue, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, 1);
            TTS_ASSERT(SUCCEEDED(hr));

            hr = IDXGIInfoQueue_Release(infoQueue);
            TTS_ASSERT(SUCCEEDED(hr));
        }
        #endif

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

        hr = D3D11CreateDevice(
            0,
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            flags,
            &featureLevel,
            1,
            D3D11_SDK_VERSION,
            &win32->device,
            0,
            &win32->deviceContext
        );
        ok = SUCCEEDED(hr);

        if (ok) {
            #ifdef TTS_DEBUG
            {
                ID3D11InfoQueue* infoQueue = 0;
                hr = ID3D11Device_QueryInterface(win32->device, &IID_ID3D11InfoQueue, (void**)&infoQueue);
                TTS_ASSERT(SUCCEEDED(hr));

                hr = ID3D11InfoQueue_SetBreakOnSeverity(infoQueue, D3D11_MESSAGE_SEVERITY_CORRUPTION, 1);
                TTS_ASSERT(SUCCEEDED(hr));

                hr = ID3D11InfoQueue_SetBreakOnSeverity(infoQueue, D3D11_MESSAGE_SEVERITY_ERROR, 1);
                TTS_ASSERT(SUCCEEDED(hr));

                ID3D11InfoQueue_Release(infoQueue);
            }
            #endif

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
            {
                swapChainDesc.Width = 0;
                swapChainDesc.Height = 0;
                swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                swapChainDesc.Stereo = 0;
                swapChainDesc.SampleDesc.Count = 1;
                swapChainDesc.SampleDesc.Quality = 0;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.BufferCount = 2;
                swapChainDesc.Scaling = DXGI_SCALING_NONE;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                swapChainDesc.Flags = 0;
            }
            hr = IDXGIFactory2_CreateSwapChainForHwnd(
                factory,
                (IUnknown *)win32->device,
                win32->window,
                &swapChainDesc,
                0,
                0,
                &win32->swapChain
            );
            ok = SUCCEEDED(hr);
        }
        IDXGIFactory2_Release(factory);
    }

    return ok;
}

void win32D3d11Render(Win32 *win32, UINT newWidth, UINT newHeight) {
    if (!win32->renderTargetView || win32->width != newWidth || win32->height != newHeight) {
		ID3D11DeviceContext_ClearState(win32->deviceContext);
        if (win32->renderTargetView) {
            ID3D11RenderTargetView_Release(win32->renderTargetView);
        }
        IDXGISwapChain1_ResizeBuffers(win32->swapChain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

        ID3D11Texture2D *renderTarget = 0;

        if (SUCCEEDED(IDXGISwapChain1_GetBuffer(win32->swapChain, 0, &IID_ID3D11Texture2D, (void **)&renderTarget))) {
            ID3D11Device_CreateRenderTargetView(win32->device, (ID3D11Resource *) renderTarget, 0, &win32->renderTargetView);
            ID3D11Texture2D_Release(renderTarget);
        }
    }

    float backgroundColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};

    ID3D11DeviceContext_ClearRenderTargetView(
        win32->deviceContext,
        win32->renderTargetView,
        backgroundColor
    );

    IDXGISwapChain1_Present(win32->swapChain, 1, 0);
}
