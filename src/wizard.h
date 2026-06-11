#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "appstate.h"
#include "disk.h"

class Wizard {
public:
    Wizard();
    ~Wizard();

    bool Create(HINSTANCE hInstance, int nCmdShow);
    int Run();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_;
    HINSTANCE hInst_;

    // Controls
    HWND hPartitionCombo_;
    HWND hIsoPath_;
    HWND hBrowseBtn_;
    HWND hEntryName_;
    HWND hSetDefault_;
    HWND hStartBtn_;
    HWND hRebootBtn_;
    HWND hCleanupBtn_;
    HWND hProgress_;
    HWND hLog_;

    // Data
    std::vector<Partition> partitions_;
    std::unique_ptr<AppState> appState_;
    UINT_PTR timerId_;
    bool setDefault_ = false;
    bool hasCompleted_ = false;
    HBRUSH hDarkBrush_ = nullptr;
    HBRUSH hEditBrush_ = nullptr;

    void OnCreate();
    void OnSize();
    void OnPaint(HDC hdc);
    void OnDrawItem(const DRAWITEMSTRUCT* dis);
    void OnCommand(int id, int code, HWND ctl);
    void OnTimer();
    HBRUSH OnCtlColor(HDC hdc, HWND ctl, UINT type);

    void LayoutControls();
    void LoadPartitions();
    void BrowseISO();
    void StartInstallation();
    void PollProgress();
    void UpdateLog();
    void PerformReboot();
    void PerformCleanup();

    void SetControlsEnabled(bool enabled);

    static constexpr int IDC_PARTITION_COMBO = 101;
    static constexpr int IDC_ISO_PATH         = 102;
    static constexpr int IDC_BROWSE_BTN       = 103;
    static constexpr int IDC_ENTRY_NAME       = 104;
    static constexpr int IDC_SET_DEFAULT      = 105;
    static constexpr int IDC_START_BTN        = 106;
    static constexpr int IDC_REBOOT_BTN       = 107;
    static constexpr int IDC_CLEANUP_BTN      = 108;
    static constexpr int IDC_PROGRESS         = 109;
    static constexpr int IDC_LOG              = 110;
};
