#include "wizard.h"
#include "theme.h"
#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    Theme::InitGDIPlus();

    Wizard wizard;
    if (!wizard.Create(hInstance, nCmdShow)) {
        MessageBoxW(nullptr, L"Failed to create application window.", L"InternalBoot", MB_OK | MB_ICONERROR);
        Theme::ShutdownGDIPlus();
        return 1;
    }

    int ret = wizard.Run();
    Theme::ShutdownGDIPlus();
    return ret;
}
