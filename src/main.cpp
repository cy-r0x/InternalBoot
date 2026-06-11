#include "wizard.h"
#include "theme.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
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


// c++.exe -std=c++17 -DUNICODE -D_UNICODE -mwindows src/main.cpp src/utils.cpp src/theme.cpp src/iso.cpp src/disk.cpp src/bcd.cpp src/boot.cpp src/appstate.cpp src/wizard.cpp -o /tmp/InternalBoot.exe -lcomctl32 -lgdi32 -lgdiplus -lshell32 -lshlwapi -luxtheme -ldwmapi
