#define COBJMACROS

#include <stdint.h>

#include "platform.h"
#include "tetris.h"

#pragma warning(push, 0)

#include <windows.h>
#include <initguid.h>
#include "cdwrite.h"
#include <stdio.h>

#pragma warning(pop)

#pragma comment(lib, "Gdi32")
#pragma comment(lib, "User32")

typedef int32_t i32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef unsigned char uchar;

typedef struct {
    i32 x;
    i32 y;
} V2I32;

typedef struct {
    u32 codepoint;
    u16 index;
    i32 xOffsetInPixels;
    i32 yOffsetInPixels;
    i32 advanceWidthInPixels;
    i32 bitmapXInPixels;
    i32 bitmapYInPixels;
    i32 bitmapWidthInPixels;
    i32 bitmapHeightInPixels;
} Glyph;

typedef struct {
    u32 codepoint;
    u32 heightInDesignUnits;
} GlyphHeight;

typedef struct {
    u32 codepoints[1024];
    i32 codepointCount;
    i32 lineHeightInPixels;
    BITMAPINFO bitmapInfo;
    void *bitmapBits;
    Glyph glyphs[TTS_CODEPOINT_COUNT];
} State;

int compareGlyphHeightDescending(const void *l, const void *r) {
    return ((GlyphHeight *)r)->heightInDesignUnits - ((GlyphHeight *)l)->heightInDesignUnits;
}

void win32DebugPrint(_Printf_format_string_ const char *format, ...) {
    char buffer[1024] = {0};

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(buffer, sizeof(buffer) - 1, format, arguments);
    OutputDebugStringA(buffer);
    va_end(arguments);
}

i32 roundF32ToI32(f32 f) {
    i32 result = (i32)f;

    if (f - (f32)result > 0.5f) {
        result++;
    }

    return result;
}

LRESULT windowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    LRESULT result = 0;

    State *state = (State *)GetWindowLongPtrA(window, GWLP_USERDATA);

    switch (message) {
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint = {0};
            HDC dc =   BeginPaint(window, &paint);

            RECT rect = {0};
            GetClientRect(window, &rect);

            int windowWidth = rect.right - rect.left;
            int windowHeight = rect.bottom - rect.top;

            BOOL ok = PatBlt(
                dc,
                0,
                0,
                windowWidth,
                windowHeight,
                BLACKNESS
            );
            TTS_ASSERT(ok);

            i32 leftMargin = 10;
            i32 topMargin = 10;

            i32 x = leftMargin;
            i32 y = topMargin;

            for (i32 codepointIndex = 0; codepointIndex < state->codepointCount; codepointIndex++) {
                u32 codepoint = state->codepoints[codepointIndex];
                if (codepoint == '\r') {
                    y += state->lineHeightInPixels;
                    x = leftMargin;
                } else {
                    u32 totalHeight = -state->bitmapInfo.bmiHeader.biHeight;
                    Glyph glyph = state->glyphs[codepoint - TTS_FIRST_CODEPOINT];
                    StretchDIBits(
                        dc,
                        x + glyph.xOffsetInPixels,
                        y + glyph.yOffsetInPixels,
                        glyph.bitmapWidthInPixels,
                        glyph.bitmapHeightInPixels,
                        glyph.bitmapXInPixels,
                        totalHeight - glyph.bitmapYInPixels - glyph.bitmapHeightInPixels,
                        glyph.bitmapWidthInPixels,
                        glyph.bitmapHeightInPixels,
                        state->bitmapBits,
                        &state->bitmapInfo,
                        DIB_RGB_COLORS,
                        SRCCOPY
                    );
                    x += glyph.advanceWidthInPixels;
                }
            }

            EndPaint(window, &paint);
        } break;

        case WM_CHAR: {
            if (
                ((wParam >= TTS_FIRST_CODEPOINT && wParam <= TTS_LAST_CODEPOINT) || wParam == '\r')
                && state->codepointCount < TTS_ARRAYCOUNT(state->codepoints)
            ) {
                state->codepoints[state->codepointCount++] = (u32)wParam;
                InvalidateRect(window, 0, 0);
            }
        } break;

        case WM_KEYDOWN: {
            if (wParam == VK_BACK && state->codepointCount > 0) {
                state->codepointCount--;
                InvalidateRect(window, 0, 0);
            }
        } break;

        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        }
    }

    return result;
}

int  WinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR     commandLine,
    int       showCommand
) {
    TTS_UNREFERENCED(instance);
    TTS_UNREFERENCED(previousInstance);
    TTS_UNREFERENCED(commandLine);
    TTS_UNREFERENCED(showCommand);

    BOOL ok = 0;
    HRESULT hr = E_FAIL;
    State state = {0};

    IDWriteFactory* factory = 0;
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        &IID_IDWriteFactory,
        (IUnknown **)(&factory)
    );
    TTS_ASSERT(SUCCEEDED(hr));

    IDWriteGdiInterop *gdiInterop = 0;
    hr = IDWriteFactory_GetGdiInterop(factory, &gdiInterop);
    TTS_ASSERT(SUCCEEDED(hr));

    IDWriteFontFile* fontFile = 0;
    hr = IDWriteFactory_CreateFontFileReference(
        factory,
        TTS_FONT_PATH,
        0,
        &fontFile
    );
    TTS_ASSERT(SUCCEEDED(hr));

    IDWriteFontFace *fontFace = 0;
    hr = IDWriteFactory_CreateFontFace(
        factory,
        DWRITE_FONT_FACE_TYPE_TRUETYPE,
        1,
        &fontFile,
        0,
        DWRITE_FONT_SIMULATIONS_NONE,
        &fontFace
    );
    TTS_ASSERT(SUCCEEDED(hr));

    UINT32 renderTargetHeight = 256;
    UINT32 renderTargetWidth  = 256;
    IDWriteBitmapRenderTarget *renderTarget = 0;
    hr = IDWriteGdiInterop_CreateBitmapRenderTarget(gdiInterop, 0, renderTargetWidth, renderTargetHeight, &renderTarget);
    TTS_ASSERT(SUCCEEDED(hr));

    IDWriteRenderingParams *renderingParams = 0;
    hr = IDWriteFactory_CreateCustomRenderingParams(
        factory,
        1.8f,
        0.5f,
        1.0f,
        DWRITE_PIXEL_GEOMETRY_RGB,
        DWRITE_RENDERING_MODE_GDI_NATURAL,
        &renderingParams
    );
    TTS_ASSERT(SUCCEEDED(hr));

    u64 bufferSize = 1024 * 1024;
    void *buffer = VirtualAlloc(
        0,
        bufferSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    TTS_ASSERT(buffer);

    HANDLE fontFileHandle = CreateFileW(
        TTS_FONT_PATH,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );
    TTS_ASSERT(fontFileHandle != INVALID_HANDLE_VALUE);

    DWORD numberOfBytesRead  = 0;

    ok = ReadFile(
        fontFileHandle,
        buffer,
        (DWORD)bufferSize,
        &numberOfBytesRead,
        0
    );
    TTS_ASSERT(ok);

    u64 fontFileSize = numberOfBytesRead;
    UINT32 codepoints[TTS_CODEPOINT_COUNT] = {0};
    for (u32 codepointIndex = 0; codepointIndex < TTS_ARRAYCOUNT(codepoints); codepointIndex++) {
        codepoints[codepointIndex] =  TTS_FIRST_CODEPOINT + codepointIndex;
    }

    DWRITE_FONT_METRICS fontFaceMetrics = {0};
    IDWriteFontFace_GetMetrics(
        fontFace,
        &fontFaceMetrics
    );

    i32 fontHeightInDesignUnits = fontFaceMetrics.ascent + fontFaceMetrics.descent;
    f32 targetHeightInPixels = 50.0f;
    f32 pixelsPerDesignUnit =  targetHeightInPixels / (f32)fontHeightInDesignUnits;
    f32 emSizeInPixels = (f32)fontFaceMetrics.designUnitsPerEm * pixelsPerDesignUnit;
    state.lineHeightInPixels = roundF32ToI32((f32)(fontHeightInDesignUnits + fontFaceMetrics.lineGap) * pixelsPerDesignUnit);
    f32 ascentInPixels =  fontFaceMetrics.ascent * pixelsPerDesignUnit;

    UINT16 glyphIndices[TTS_ARRAYCOUNT(codepoints)] = {0};
    hr = IDWriteFontFace_GetGlyphIndices(
        fontFace,
        codepoints,
        TTS_ARRAYCOUNT(codepoints),
        glyphIndices
    );
    TTS_ASSERT(SUCCEEDED(hr));

    DWRITE_GLYPH_METRICS glyphMetrics[TTS_ARRAYCOUNT(glyphIndices)] = {0};
    hr = IDWriteFontFace_GetGdiCompatibleGlyphMetrics(
        fontFace,
        emSizeInPixels,
        1.0f,
        0,
        1,
        glyphIndices,
        TTS_ARRAYCOUNT(glyphIndices),
        glyphMetrics,
        0
    );
    TTS_ASSERT(SUCCEEDED(hr));

    GlyphHeight glyphHeights [TTS_ARRAYCOUNT(state.glyphs)] = {0};

    for (u32 codepointIndex = 0; codepointIndex < TTS_ARRAYCOUNT(codepoints); codepointIndex++) {
        state.glyphs[codepointIndex].codepoint = codepoints[codepointIndex];
        state.glyphs[codepointIndex].index = glyphIndices[codepointIndex];
        DWRITE_GLYPH_METRICS currentGlyphMetrics = glyphMetrics[codepointIndex];
        state.glyphs[codepointIndex].advanceWidthInPixels = roundF32ToI32((f32)glyphMetrics[codepointIndex].advanceWidth * pixelsPerDesignUnit);
        glyphHeights[codepointIndex].heightInDesignUnits = currentGlyphMetrics.advanceHeight - currentGlyphMetrics.topSideBearing - currentGlyphMetrics.bottomSideBearing;
        glyphHeights[codepointIndex].codepoint = codepoints[codepointIndex];
    }

    qsort(
        glyphHeights,
        TTS_ARRAYCOUNT(glyphHeights),
        sizeof(*glyphHeights),
        compareGlyphHeightDescending
    );

    i32 xOffset = 1;
    i32 yOffset = 1;
    i32 bitmapWidth = 256;
    i32 lineHeight = 0;

    u8 *targetPixels = (u8 *)buffer + fontFileSize;

    for (u32 glyphIndex = 0; glyphIndex < TTS_ARRAYCOUNT(state.glyphs); glyphIndex++) {
        Glyph *glyph = state.glyphs + glyphHeights[glyphIndex].codepoint - TTS_FIRST_CODEPOINT;

        HDC dc = IDWriteBitmapRenderTarget_GetMemoryDC(renderTarget);

        HBITMAP bitmap = (HBITMAP)GetCurrentObject(dc, OBJ_BITMAP);

        ok = PatBlt(
            dc,
            0,
            0,
            renderTargetWidth,
            renderTargetHeight,
            BLACKNESS
        );
        TTS_ASSERT(ok);

        TTS_ASSERT(SUCCEEDED(hr));

        FLOAT glyphAdvance = 0.0f;

        DWRITE_GLYPH_RUN glyphRun = {0};
        {
            glyphRun.fontFace = fontFace;
            glyphRun.fontEmSize = emSizeInPixels;
            glyphRun.glyphCount = 1;
            glyphRun.glyphIndices = &(glyph->index);
            glyphRun.glyphAdvances = &glyphAdvance;
            glyphRun.glyphOffsets = 0;
            glyphRun.isSideways = 0;
            glyphRun.bidiLevel = 0;
        }

        RECT blackBoxRect = {0};

        int originX = renderTargetWidth / 2;
        int originY = renderTargetHeight / 2;

        hr = IDWriteBitmapRenderTarget_DrawGlyphRun(
            renderTarget,
            (FLOAT)originX,
            (FLOAT)originY,
            DWRITE_MEASURING_MODE_GDI_NATURAL,
            &glyphRun,
            renderingParams,
            RGB(0, 0, 255),
            &blackBoxRect
        );
        TTS_ASSERT(SUCCEEDED(hr));
        TTS_ASSERT(blackBoxRect.left >= 0);
        TTS_ASSERT(blackBoxRect.top >= 0);
        TTS_ASSERT(blackBoxRect.right < (LONG)renderTargetWidth);
        TTS_ASSERT(blackBoxRect.bottom < (LONG)renderTargetHeight);

        DIBSECTION dib = {0};
        GetObject(bitmap, sizeof(dib), &dib);

        LONG glyphWidth = blackBoxRect.right - blackBoxRect.left;
        LONG glyphHeight = blackBoxRect.bottom - blackBoxRect.top;

        if ((glyphHeight + 1) > lineHeight) {
            lineHeight = glyphHeight + 1;
        }

        if (xOffset + glyphWidth > bitmapWidth) {
            xOffset = 1;
            yOffset += lineHeight;
            lineHeight = glyphHeight + 1;
        }

        glyph->xOffsetInPixels = blackBoxRect.left - originX;
        glyph->yOffsetInPixels = roundF32ToI32(ascentInPixels) + blackBoxRect.top - originY;
        glyph->bitmapWidthInPixels  = glyphWidth;
        glyph->bitmapHeightInPixels = glyphHeight;
        glyph->bitmapXInPixels = xOffset;
        glyph->bitmapYInPixels = yOffset;

        // u32 *srcPixels = (u32 *)dib.dsBm.bmBits;
        // u32 *targetPixels = (u32 *)((uchar *)buffer + fontFileSize);

        // for (LONG y = 0; y < glyphHeight; y++) {
        // for (LONG x = 0; x < glyphWidth; x++) {
        // LONG renderTargetY =  y + blackBoxRect.top;
        // LONG renderTargetX =  x + blackBoxRect.left;
        // u32 srcPixel = srcPixels[(renderTargetY * renderTargetWidth) + renderTargetX];
        // u8 srcBlue = (u8)srcPixel;
        // u32 targetPixel = srcBlue << 0 | srcBlue << 8 | srcBlue << 16 | srcBlue << 24;
        // i32 targetY = y + yOffset;
        // i32 targetX = x + xOffset;
        // u32 targetOffsetInBytes = (targetY * bitmapWidth + targetX) * sizeof(u32);

        // TTS_ASSERT((targetOffsetInBytes <= (bufferSize  - sizeof(u32))));
        // targetPixels[(targetY * bitmapWidth) + targetX] = targetPixel;
        // }
        // }

        u32 *srcPixels = (u32 *)dib.dsBm.bmBits;

        for (LONG y = 0; y < glyphHeight; y++) {
            for (LONG x = 0; x < glyphWidth; x++) {
                LONG renderTargetY =  y + blackBoxRect.top;
                LONG renderTargetX =  x + blackBoxRect.left;
                u32 srcPixel = srcPixels[(renderTargetY * renderTargetWidth) + renderTargetX];
                u8 srcBlue = (u8)srcPixel;
                u8 targetPixel = srcBlue;
                i32 targetY = y + yOffset;
                i32 targetX = x + xOffset;
                u32 targetOffsetInBytes = targetY * bitmapWidth + targetX;

                TTS_ASSERT(targetOffsetInBytes < bufferSize);
                targetPixels[(targetY * bitmapWidth) + targetX] = targetPixel;
            }
        }

        xOffset += glyphWidth + 1;
    }

    i32 bitmapHeight = yOffset + lineHeight;
    // u32 pixelsSizeInBytes = bitmapWidth * bitmapHeight * sizeof(u32);

    // BITMAPINFOHEADER bitmapInfoHeader = {0};
    // bitmapInfoHeader.biSize = sizeof(bitmapInfoHeader);
    // bitmapInfoHeader.biWidth = bitmapWidth;
    // bitmapInfoHeader.biHeight = -bitmapHeight;
    // bitmapInfoHeader.biPlanes = 1;
    // bitmapInfoHeader.biBitCount = 32;
    // bitmapInfoHeader.biCompression = BI_RGB;

    // state.bitmapInfo.bmiHeader = bitmapInfoHeader;
    // state.bitmapBits = (uchar *)buffer + fontFileSize;

    // BITMAPFILEHEADER fileHeader = {0};
    // fileHeader.bfType = 'MB';
    // fileHeader.bfSize = sizeof(fileHeader) + sizeof(bitmapInfoHeader) + pixelsSizeInBytes;
    // fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(bitmapInfoHeader);

    // char fileName[] = "gdi.bmp";

    // {
    // HANDLE file = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    // TTS_ASSERT(file);
    // DWORD bytesWritten = 0;
    // ok = WriteFile(file, &fileHeader, sizeof(fileHeader), &bytesWritten, 0);
    // TTS_ASSERT(ok);
    // ok = WriteFile(file, &bitmapInfoHeader, sizeof(bitmapInfoHeader), &bytesWritten, 0);
    // TTS_ASSERT(ok);
    // ok = WriteFile(file, state.bitmapBits, pixelsSizeInBytes, &bytesWritten, 0);
    // TTS_ASSERT(ok);
    // CloseHandle(file);
    // }

    {
        HANDLE file = CreateFileA(TTS_ATLAS_PATH, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        TTS_ASSERT(file);
        DWORD bytesWritten = 0;
        Atlas atlas = {0};
        atlas.width = bitmapWidth;
        atlas.height = bitmapHeight;
        ok = WriteFile(file, &atlas, sizeof(atlas), &bytesWritten, 0);
        TTS_ASSERT(ok);
        ok = WriteFile(file, targetPixels, bitmapWidth * bitmapHeight, &bytesWritten, 0);
        DWORD error = GetLastError();
        win32DebugPrint("%d\n", error);
        TTS_ASSERT(ok);
        CloseHandle(file);
    }

    // WNDCLASSEXA windowClass = {0};
    // char className[] = "main";
    // {
    // windowClass.cbSize = sizeof(windowClass);
    // windowClass.lpfnWndProc = windowProc;
    // windowClass.hInstance = instance;
    // windowClass.lpszClassName = className;
    // windowClass.hCursor = LoadCursorA(0, IDC_ARROW);
    // }

    // ok = RegisterClassExA(&windowClass);
    // TTS_ASSERT(ok);

    // HWND window = CreateWindowExA(
    // 0,
    // windowClass.lpszClassName,
    // "Direct Write",
    // WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
    // CW_USEDEFAULT,
    // CW_USEDEFAULT,
    // CW_USEDEFAULT,
    // CW_USEDEFAULT,
    // 0,
    // 0,
    // instance,
    // 0
    // );
    // TTS_ASSERT(window);

    // SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)(&state));
    // ShowWindow(window, showCommand);

    // MSG message = {0};
    // while ((ok = GetMessage(&message, 0, 0, 0))) {
    // if (ok == -1) {
    // break;
    // } else {
    // TranslateMessage(&message);
    // DispatchMessageA(&message);
    // }
    // }

    return 0;
}
