#include "theme.h"
#include <dwmapi.h>
#include <vector>
#include <gdiplus.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

namespace Theme {

static ULONG_PTR gdiplusToken = 0;

static std::vector<HFONT> fontCache;

bool EnableDarkMode(HWND hwnd) {
    constexpr int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    BOOL dark = TRUE;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    return SUCCEEDED(hr);
}

HFONT Font(int ptSize, bool bold) {
    HFONT hFont = CreateFontW(
        -MulDiv(ptSize, GetDeviceCaps(GetDC(nullptr), LOGPIXELSY), 72),
        0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
        L"Segoe UI"
    );
    if (hFont) fontCache.push_back(hFont);
    return hFont;
}

HFONT FontDefault() {
    static HFONT f = Font(10, false);
    return f;
}

HFONT FontBold(int ptSize) {
    return Font(ptSize, true);
}

void InitGDIPlus() {
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup(&gdiplusToken, &input, nullptr);
}

void ShutdownGDIPlus() {
    Gdiplus::GdiplusShutdown(gdiplusToken);
    for (HFONT f : fontCache) {
        if (f) DeleteObject(f);
    }
    fontCache.clear();
}

void FillRectColor(HDC hdc, const RECT& rc, COLORREF color) {
    SetBkColor(hdc, color);
    ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);
}

} // namespace Theme
