#include <stdint.h>
#include <stdbool.h>

#include "tetris.h"
#include "platform.h"
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
#include <stdio.h>
#include <xaudio2.h>
#pragma warning(pop)

#include "win32.c"
#include "d3d11.c"
#include "xaudio2.c"

#pragma warning(push, 0)
#include "cdwrite.h"
#pragma warning(pop)

#pragma comment(lib, "Gdi32")
#pragma comment(lib, "User32")

typedef struct {
    uint32_t codepoint;
    uint32_t heightInDesignUnits;
} AtlGlyphHeight;

int compareGlyphHeightDescending(const void *l, const void *r) {
    return ((AtlGlyphHeight *)r)->heightInDesignUnits - ((AtlGlyphHeight *)l)->heightInDesignUnits;
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

    uint64_t bufferSize = 1024 * 1024;
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

    uint64_t fontFileSize = numberOfBytesRead;
    UINT32 codepoints[TTS_CODEPOINT_COUNT] = {0};
    for (uint32_t codepointIndex = 0; codepointIndex < TTS_ARRAYCOUNT(codepoints); codepointIndex++) {
        codepoints[codepointIndex] =  TTS_FIRST_CODEPOINT + codepointIndex;
    }

    DWRITE_FONT_METRICS fontFaceMetrics = {0};
    IDWriteFontFace_GetMetrics(
        fontFace,
        &fontFaceMetrics
    );

    int32_t fontHeightInDesignUnits = fontFaceMetrics.ascent + fontFaceMetrics.descent;
    float targetHeightInPixels = 50.0f;
    float pixelsPerDesignUnit =  targetHeightInPixels / (float)fontHeightInDesignUnits;
    float emSizeInPixels = (float)fontFaceMetrics.designUnitsPerEm * pixelsPerDesignUnit;
    float ascentInPixels =  fontFaceMetrics.ascent * pixelsPerDesignUnit;

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

    AtlGlyphHeight glyphHeights [TTS_CODEPOINT_COUNT] = {0};

    TtsAtlas atlas = {0};
    for (uint32_t codepointIndex = 0; codepointIndex < TTS_ARRAYCOUNT(codepoints); codepointIndex++) {
        atlas.glyphs[codepointIndex].codepoint = codepoints[codepointIndex];
        atlas.glyphs[codepointIndex].index = glyphIndices[codepointIndex];
        DWRITE_GLYPH_METRICS currentGlyphMetrics = glyphMetrics[codepointIndex];
        atlas.glyphs[codepointIndex].advanceWidthInPixels = (float)glyphMetrics[codepointIndex].advanceWidth * pixelsPerDesignUnit;
        glyphHeights[codepointIndex].heightInDesignUnits = currentGlyphMetrics.advanceHeight - currentGlyphMetrics.topSideBearing - currentGlyphMetrics.bottomSideBearing;
        glyphHeights[codepointIndex].codepoint = codepoints[codepointIndex];
    }

    qsort(
        glyphHeights,
        TTS_ARRAYCOUNT(glyphHeights),
        sizeof(*glyphHeights),
        compareGlyphHeightDescending
    );

    int32_t xOffset = 1;
    int32_t yOffset = 1;
    int32_t bitmapWidth = 256;
    int32_t lineHeight = 0;

    uint8_t *targetPixels = (uint8_t *)buffer + fontFileSize;

    for (uint32_t glyphIndex = 0; glyphIndex < TTS_ARRAYCOUNT(atlas.glyphs); glyphIndex++) {
        TtsGlyph *glyph = atlas.glyphs + glyphHeights[glyphIndex].codepoint - TTS_FIRST_CODEPOINT;

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

        glyph->xOffsetInPixels = (float)blackBoxRect.left - (float)originX;
        glyph->yOffsetInPixels = ascentInPixels + (float)blackBoxRect.top - (float)originY;
        glyph->bitmapWidthInPixels  = (float)glyphWidth;
        glyph->bitmapHeightInPixels = (float)glyphHeight;
        glyph->bitmapXInPixels = (float)xOffset;
        glyph->bitmapYInPixels = (float)yOffset;

        uint32_t *srcPixels = (uint32_t *)dib.dsBm.bmBits;

        for (LONG y = 0; y < glyphHeight; y++) {
            for (LONG x = 0; x < glyphWidth; x++) {
                LONG renderTargetY =  y + blackBoxRect.top;
                LONG renderTargetX =  x + blackBoxRect.left;
                uint32_t srcPixel = srcPixels[(renderTargetY * renderTargetWidth) + renderTargetX];
                uint8_t srcBlue = (uint8_t)srcPixel;
                uint8_t targetPixel = srcBlue;
                int32_t targetY = y + yOffset;
                int32_t targetX = x + xOffset;
                uint32_t targetOffsetInBytes = targetY * bitmapWidth + targetX;

                TTS_ASSERT(targetOffsetInBytes < bufferSize);
                targetPixels[(targetY * bitmapWidth) + targetX] = targetPixel;
            }
        }

        xOffset += glyphWidth + 1;
    }

    int32_t bitmapHeight = yOffset + lineHeight;
    {
        HANDLE file = CreateFileA(TTS_ATLAS_PATH, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        TTS_ASSERT(file);
        DWORD bytesWritten = 0;
        atlas.width = (float)bitmapWidth;
        atlas.height = (float)bitmapHeight;
        atlas.lineHeightInPixels = (float)(fontHeightInDesignUnits + fontFaceMetrics.lineGap) * pixelsPerDesignUnit;
        ok = WriteFile(file, &atlas, sizeof(atlas), &bytesWritten, 0);
        TTS_ASSERT(ok);
        ok = WriteFile(file, targetPixels, bitmapWidth * bitmapHeight, &bytesWritten, 0);
        DWORD error = GetLastError();
        platformDebugPrint("%d\n", error);
        TTS_ASSERT(ok);
        CloseHandle(file);
    }

    return 0;
}
