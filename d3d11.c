static BOOL d3d11Compile(
    VOID *source,
    SIZE_T sourceSize,
    LPCSTR entryPoint,
    LPCSTR target,
    ID3DBlob **bytecode
) {
    UINT flags = D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_ENABLE_STRICTNESS;
    #ifdef TTS_DEBUG
    flags |= D3DCOMPILE_DEBUG;
    #endif

    ID3DBlob *errorMessages = 0;
    HRESULT hr = D3DCompile(
        source,
        sourceSize,
        0,
        0,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        flags,
        0,
        bytecode,
        &errorMessages
    );
    BOOL ok = SUCCEEDED(hr);

    if (!ok && errorMessages) {
        char *error = (char *) errorMessages->lpVtbl->GetBufferPointer(errorMessages);
        OutputDebugStringA(error);
        errorMessages->lpVtbl->Release(errorMessages);
    }

    return ok;
}

static BOOL d3d11Init(TtsTetris *tetris) {
    TtsPlatform *win32 = tetris->platform;

    HRESULT hr = E_FAIL;
    BOOL ok = 0;
    UINT flags = 0;

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

    #ifdef TTS_DEBUG
    {
        TTS_ASSERT(ok);

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

    if (ok) {
        IDXGIFactory2 *factory = 0;
        hr = CreateDXGIFactory(&IID_IDXGIFactory2, (void **)&factory);
        ok = SUCCEEDED(hr);
        if (ok) {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
            {
                swapChainDesc.Width = 0;
                swapChainDesc.Height = 0;
                swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                swapChainDesc.Stereo = 0;
                swapChainDesc.SampleDesc.Count = 4;
                swapChainDesc.SampleDesc.Quality = 0;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.BufferCount = 2;
                swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
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

            IDXGIFactory2_Release(factory);
        }
    }

    char source[] = TTS_QUOTE(
        cbuffer VsConstants : register(b0){
            float windowWidth;
            float windowHeight;
            double padding;
        };
        struct VsInput {
            float2 position : POS;
            float2 uv : TEX;
            float1 mask : MSK;
            float4 color : COL;
        };

        struct VsOutput {
            float4 position : SV_POSITION;
            float2 uv : TEX;
            float1 mask : MSK;
            float4 color: COLOR;
        };

        Texture2D<float> tex : register(t0);

        SamplerState samp: register(s0);

        VsOutput vertexMain(VsInput input) {
            VsOutput output = (VsOutput)0;
            float d3dX = ((input.position.x - (windowWidth / 2.0)) / windowWidth) * 2.0;
            float d3dY = -(((input.position.y - (windowHeight / 2.0)) / windowHeight) * 2.0);

            output.position = float4(d3dX, d3dY, 0.0, 1.0);
            output.color = input.color;
            output.mask = input.mask;
            output.uv = input.uv;

            return output;
        }

        float4 pixelMain(VsOutput input): SV_TARGET {
            float texAlpha = tex.Sample(samp, input.uv);

            float4 tek = (input.mask * (texAlpha * input.color));
            float4 col = ((1.0f - input.mask) * input.color);
            return (tek + col);
        }
    );

    if (ok) {
        ID3DBlob *shaderBytecode = 0;

        ok = d3d11Compile(
            source,
            sizeof(source) - 1,
            "vertexMain",
            "vs_5_0",
            &shaderBytecode
        );

        if (ok) {
            hr = ID3D11Device_CreateVertexShader(
                win32->device,
                shaderBytecode->lpVtbl->GetBufferPointer(shaderBytecode),
                shaderBytecode->lpVtbl->GetBufferSize(shaderBytecode),
                0,
                &win32->vertexShader
            );
            ok = SUCCEEDED(hr);

            if (ok) {
                D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
                    {
                        .SemanticName = "POS",
                        .Format = DXGI_FORMAT_R32G32_FLOAT,
                        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                    },
                    {
                        .SemanticName = "TEX",
                        .Format = DXGI_FORMAT_R32G32_FLOAT,
                        .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                    },
                    {
                        .SemanticName = "MSK",
                        .Format = DXGI_FORMAT_R32_FLOAT,
                        .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                    },
                    {
                        .SemanticName = "COL",
                        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
                        .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
                        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                    },
                };

                hr = ID3D11Device_CreateInputLayout(
                    win32->device,
                    inputDesc,
                    TTS_ARRAYCOUNT(inputDesc),
                    shaderBytecode->lpVtbl->GetBufferPointer(shaderBytecode),
                    shaderBytecode->lpVtbl->GetBufferSize(shaderBytecode),
                    &win32->inputLayout
                );
                ok = SUCCEEDED(hr);
            }
            shaderBytecode->lpVtbl->Release(shaderBytecode);
        }
    }

    if (ok) {
        ID3DBlob *shaderBytecode = 0;

        ok = d3d11Compile(
            source,
            sizeof(source) - 1,
            "pixelMain",
            "ps_5_0",
            &shaderBytecode
        );

        if (ok) {
            hr = ID3D11Device_CreatePixelShader(
                win32->device,
                shaderBytecode->lpVtbl->GetBufferPointer(shaderBytecode),
                shaderBytecode->lpVtbl->GetBufferSize(shaderBytecode),
                0,
                &win32->pixelShader
            );
            ok = SUCCEEDED(hr);
            shaderBytecode->lpVtbl->Release(shaderBytecode);
        }
    }

    TtsReadResult file = {0};

    if (ok) {
        file = platformReadEntireFile(TTS_ATLAS_PATH);
        ok = file.size > 0;
    }

    if (ok) {
        TtsAtlas *atlas = (TtsAtlas *)file.data;
        tetris->atlas = *atlas;
        uint8_t *textureData = (uint8_t *)file.data + sizeof(TtsAtlas);

        D3D11_TEXTURE2D_DESC atlastTextureDesc = {0};
        {
            atlastTextureDesc.Width = (UINT)atlas->width;
            atlastTextureDesc.Height = (UINT)atlas->height;
            atlastTextureDesc.MipLevels = 1;
            atlastTextureDesc.ArraySize = 1;
            atlastTextureDesc.Format = DXGI_FORMAT_R8_UNORM;
            atlastTextureDesc.SampleDesc.Count = 1;
            atlastTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
            atlastTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        }

        D3D11_SUBRESOURCE_DATA atlasSRD = {0};
        {
            atlasSRD.pSysMem = textureData;
            atlasSRD.SysMemPitch = (UINT)atlas->width * sizeof(uint8_t);
        }

        ID3D11Texture2D *atlasTexture;
        hr = ID3D11Device_CreateTexture2D(win32->device, &atlastTextureDesc, &atlasSRD, &atlasTexture);
        ok = SUCCEEDED(hr);

        if (ok) {
            hr = ID3D11Device_CreateShaderResourceView(win32->device, (ID3D11Resource *) atlasTexture, 0, &win32->textureView);
            ok = SUCCEEDED(hr);
        }
    }

    if (ok) {
        // Create sampler

        D3D11_SAMPLER_DESC samplerDesc = {0};
        {
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
            samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
            samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            samplerDesc.BorderColor[0] = 1.0f;
            samplerDesc.BorderColor[1] = 1.0f;
            samplerDesc.BorderColor[2] = 1.0f;
            samplerDesc.BorderColor[3] = 1.0f;
        }

        ID3D11Device_CreateSamplerState(win32->device, &samplerDesc, &win32->samplerState);

        ok = SUCCEEDED(hr);
    }

    if (ok)     // Create blend state
    {
        D3D11_BLEND_DESC blendDesc = {0};
        {
            blendDesc.AlphaToCoverageEnable = 0;
            blendDesc.IndependentBlendEnable = 0;
            blendDesc.RenderTarget[0].BlendEnable = 1;
            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        }
        hr = ID3D11Device_CreateBlendState(win32->device, &blendDesc, &win32->blendState);
        ok = SUCCEEDED(hr);
    }

    if (ok) {
        D3D11_BUFFER_DESC vertexBufferDesc = {0};
        {
            vertexBufferDesc.ByteWidth = sizeof(win32->vertices.vertices);
            vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }

        hr = ID3D11Device_CreateBuffer(
            win32->device,
            &vertexBufferDesc,
            0,
            &win32->vertexBuffer
        );
        ok = SUCCEEDED(hr);
    }

    if (ok) {
        D3D11_BUFFER_DESC constantBufferDesc = {0};
        {
            constantBufferDesc.ByteWidth = sizeof(VsConstants);
            constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        }

        hr = ID3D11Device_CreateBuffer(
            win32->device,
            &constantBufferDesc,
            0,
            &win32->constantBuffer
        );
        ok = SUCCEEDED(hr);
    }

    return ok;
}

static void d3d11AddVertex(
    float x, float y,
    float u, float v,
    float mask,
    TtsColor color,
    Vertices *vertices
) {
    if (vertices->vertexCount < TTS_ARRAYCOUNT(vertices->vertices)) {
        Vertex vertex = {0};
        {
            vertex.x = x;
            vertex.y = y;
            vertex.u = u;
            vertex.v = v;
            vertex.mask = mask;
            vertex.r = color.r / 255.0f;
            vertex.g = color.g / 255.0f;
            vertex.b = color.b / 255.0f;
            vertex.a = color.a / 255.0f;
        }

        vertices->vertices[vertices->vertexCount++] = vertex;
    } else {
        TTS_ASSERT(!"Too many vertices");
    }
}

static void d3d11DrawTriangle (
    float x0, float y0,
    float u0, float v0,
    float x1, float y1,
    float u1, float v1,
    float x2, float y2,
    float u2, float v2,
    float mask,
    TtsColor color,
    Vertices *vertices
) {
    d3d11AddVertex(x0, y0, u0, v0, mask, color, vertices);
    d3d11AddVertex(x1, y1, u1, v1, mask, color, vertices);
    d3d11AddVertex(x2, y2, u2, v2, mask, color, vertices);
}

static void d3d11DrawQuad(
    float x, float y,
    float width, float height,
    float u, float v,
    float uWidth, float vHeight,
    float mask,
    TtsColor color,
    Vertices *vertices
) {
    float left = x;
    float top = y;
    float right = x + width;
    float bottom = y + height;
    float uLeft = u;
    float uRight = u + uWidth;
    float vTop = v;
    float vBottom = v + vHeight;

    d3d11DrawTriangle(left, top, uLeft, vTop,  right, top, uRight, vTop, left, bottom, uLeft, vBottom, mask, color, vertices);
    d3d11DrawTriangle(right, top, uRight, vTop, right, bottom, uRight, vBottom, left, bottom, uLeft, vBottom, mask, color, vertices);
}

static void platformDrawTextureQuad(
    float x, float y,
    float width, float height,
    float xInTexture, float yInTexture,
    float widthInTexture, float heightInTexture,
    float textureWidth, float textureHeight,
    TtsColor color,
    TtsPlatform *win32
) {
    d3d11DrawQuad(
        x, y,
        width, height,
        xInTexture / textureWidth, yInTexture / textureHeight,
        widthInTexture / textureWidth, heightInTexture / textureHeight,
        1.0f,
        color,
        &win32->vertices
    );
}

static void platformDrawColorTriangle(
    float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    TtsColor color,
    TtsPlatform *win32
) {
    TTS_UNREFERENCED(x3);
    TTS_UNREFERENCED(y3);
    d3d11DrawTriangle(
        x0, y0,
        0.0f, 0.0f,
        x1, y1, 0.0f, 0.0f,
        x2, y2, 0.0f, 0.0f,
        0.0f,
        color,
        &win32->vertices
    );
}

static void d3d11Render(TtsTetris *tetris, UINT newWidth, UINT newHeight) {
    HRESULT hr = E_FAIL;
    TtsPlatform *win32 = tetris->platform;
    if (!win32->renderTargetView || tetris->windowWidth != newWidth || tetris->windowHeight != newHeight) {
        if (win32->renderTargetView) {
            ID3D11RenderTargetView_Release(win32->renderTargetView);
        }
        IDXGISwapChain1_ResizeBuffers(win32->swapChain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

        D3D11_MAPPED_SUBRESOURCE mappedSubresource = {0};
        hr = ID3D11DeviceContext_Map(
            win32->deviceContext,
            (ID3D11Resource *) win32->constantBuffer,
            0,
            D3D11_MAP_WRITE_DISCARD,

            0,
            &mappedSubresource
        );
        if (SUCCEEDED(hr) && mappedSubresource.pData) {
            VsConstants *destination = (VsConstants *) mappedSubresource.pData;
            destination->windowWidth = (float)newWidth;
            destination->windowHeight = (float)newHeight;

            ID3D11DeviceContext_Unmap(
                win32->deviceContext,
                (ID3D11Resource *) win32->constantBuffer,
                0
            );
        }

        ID3D11Texture2D *renderTarget = 0;
        if (SUCCEEDED(IDXGISwapChain1_GetBuffer(win32->swapChain, 0, &IID_ID3D11Texture2D, (void **)&renderTarget))) {
            ID3D11Device_CreateRenderTargetView(win32->device, (ID3D11Resource *) renderTarget, 0, &win32->renderTargetView);
            ID3D11Texture2D_Release(renderTarget);
        }
    }

    float normalizedBackground[4] = {
        tetris->backgroundColor.r / 255.0f,
        tetris->backgroundColor.g / 255.0f,
        tetris->backgroundColor.b / 255.0f,
        tetris->backgroundColor.a / 255.0f,
    };

    ID3D11DeviceContext_ClearRenderTargetView(
        win32->deviceContext,
        win32->renderTargetView,
        normalizedBackground
    );

    D3D11_MAPPED_SUBRESOURCE mappedSubresource = {0};

    hr = ID3D11DeviceContext_Map(
        win32->deviceContext,
        (ID3D11Resource *) win32->vertexBuffer,
        0,
        D3D11_MAP_WRITE_DISCARD,

        0,
        &mappedSubresource
    );

    if (SUCCEEDED(hr) && mappedSubresource.pData) {
        Vertex *destination = (Vertex *) mappedSubresource.pData;
        for (uint32_t vertexIndex = 0; vertexIndex < win32->vertices.vertexCount; vertexIndex++) {
            destination[vertexIndex] =  win32->vertices.vertices[vertexIndex];
        }

        ID3D11DeviceContext_Unmap(
            win32->deviceContext,
            (ID3D11Resource *) win32->vertexBuffer,
            0
        );
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    ID3D11DeviceContext_IASetVertexBuffers(
        win32->deviceContext,
        0,
        1,
        &win32->vertexBuffer,
        &stride,
        &offset
    );

    ID3D11DeviceContext_VSSetConstantBuffers(
        win32->deviceContext,
        0,
        1,
        &win32->constantBuffer
    );

    D3D11_VIEWPORT viewport = {0};
    {
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = (float)newWidth;
        viewport.Height = (float)newHeight;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 0.0f;
    }

    ID3D11DeviceContext_IASetPrimitiveTopology(win32->deviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11DeviceContext_IASetInputLayout(win32->deviceContext, win32->inputLayout);

    ID3D11DeviceContext_RSSetViewports(win32->deviceContext, 1, &viewport);

    ID3D11DeviceContext_VSSetShader(win32->deviceContext, win32->vertexShader, 0, 0);

    ID3D11DeviceContext_PSSetShader(win32->deviceContext, win32->pixelShader, 0, 0);

    ID3D11DeviceContext_PSSetSamplers(win32->deviceContext, 0, 1, &win32->samplerState);

    ID3D11DeviceContext_PSSetShaderResources(win32->deviceContext, 0, 1, &win32->textureView);

    ID3D11DeviceContext_OMSetRenderTargets(win32->deviceContext, 1, &win32->renderTargetView, 0);

    ID3D11DeviceContext_OMSetBlendState(win32->deviceContext, win32->blendState, 0, 0xffffffff);

    ID3D11DeviceContext_Draw(win32->deviceContext, win32->vertices.vertexCount, 0);

    IDXGISwapChain1_Present(win32->swapChain, 1, 0);

    ID3D11DeviceContext_ClearState(win32->deviceContext);

    win32->vertices.vertexCount = 0;
}
