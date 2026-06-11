#pragma once
#include <windows.h>

namespace Theme {

constexpr COLORREF BG            = RGB(15, 23, 42);     // #0f172a
constexpr COLORREF PANEL         = RGB(30, 41, 59);     // #1e293b
constexpr COLORREF BORDER        = RGB(51, 65, 85);     // #334155
constexpr COLORREF PRIMARY       = RGB(56, 189, 248);   // #38bdf8
constexpr COLORREF SUCCESS       = RGB(34, 197, 94);    // #22c55e
constexpr COLORREF ERROR         = RGB(239, 68, 68);    // #ef4444
constexpr COLORREF TEXT_PRIMARY  = RGB(226, 232, 240);  // #e2e8f0
constexpr COLORREF TEXT_SECONDARY= RGB(148, 163, 184);  // #94a3b8
constexpr COLORREF TEXT_MUTED    = RGB(100, 116, 139);  // #64748b
constexpr COLORREF EDIT_BG       = RGB(15, 23, 42);     // #0f172a
constexpr COLORREF BTN_SECONDARY = RGB(51, 65, 85);     // #334155
constexpr COLORREF BTN_SECONDARY_HOVER = RGB(71, 85, 105); // #475569

bool EnableDarkMode(HWND hwnd);
HFONT Font(int ptSize, bool bold = false);
HFONT FontDefault();
HFONT FontBold(int ptSize);

void InitGDIPlus();
void ShutdownGDIPlus();

void FillRectColor(HDC hdc, const RECT& rc, COLORREF color);

} // namespace Theme
