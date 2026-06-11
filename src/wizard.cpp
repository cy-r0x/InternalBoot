#include "wizard.h"
#include "theme.h"
#include "iso.h"
#include "disk.h"
#include "bcd.h"
#include "boot.h"
#include "utils.h"
#include <commdlg.h>
#include <uxtheme.h>
#include <thread>
#include <sstream>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "uxtheme.lib")

#ifndef PBM_SETBARCOLOR
#define PBM_SETBARCOLOR (WM_USER+9)
#endif
#ifndef PBM_SETBKCOLOR
#define PBM_SETBKCOLOR  0x2001
#endif

static constexpr int MARGIN_X = 12;
static constexpr int MARGIN_Y = 12;
static constexpr int LABEL_W = 120;
static constexpr int ROW_H = 26;
static constexpr int ROW_GAP = 8;
static constexpr int BTN_H = 32;
static constexpr int PROGRESS_H = 20;
static constexpr int PROGRESS_GAP = 8;

static void RunInstallThread(
    std::wstring isoPath,
    std::wstring driveLetter,
    std::wstring targetPath,
    std::wstring entryName,
    bool setDefault,
    AppState* state
) {
    auto log = state->Logger();

    auto setProg = [&](InstallStage stage, const std::wstring& msg, int pct, bool running, bool err, const std::wstring& errMsg) {
        Progress p{};
        p.stage = stage;
        p.message = msg;
        p.percent = pct;
        p.isRunning = running;
        p.hasError = err;
        p.errorMsg = errMsg;
        p.canReboot = (stage == InstallStage::Complete);
        p.canCleanup = (stage == InstallStage::Complete);
        state->SetProgress(p);
    };

    log(L"=== Starting InternalBoot Installation ===");
    setProg(InstallStage::Preparing, L"Preparing installation...", 0, true, false, {});

    // 1. Validate ISO
    setProg(InstallStage::Validating, L"Validating ISO...", 5, true, false, {});
    IsoInfo info;
    if (!ValidateISO(isoPath, &info, log)) {
        setProg(InstallStage::Error, L"ISO validation failed", 0, false, true, L"ISO validation failed");
        log(L"=== Installation failed ===");
        return;
    }

    // 2. Check space
    setProg(InstallStage::Checking, L"Checking disk space...", 10, true, false, {});
    uint64_t required = info.sizeBytes + 2ULL * 1024 * 1024 * 1024;
    if (!ValidateSpace(driveLetter, required, log)) {
        setProg(InstallStage::Error, L"Insufficient disk space", 0, false, true, L"Insufficient disk space");
        log(L"=== Installation failed ===");
        return;
    }

    // 3. Backup BCD
    setProg(InstallStage::Backup, L"Backing up BCD store...", 15, true, false, {});
    std::wstring backupPath = BackupBCD(log);
    if (backupPath.empty()) {
        setProg(InstallStage::Error, L"BCD backup failed", 0, false, true, L"BCD backup failed");
        log(L"=== Installation failed ===");
        return;
    }

    // 4. Mount ISO
    setProg(InstallStage::Mounting, L"Mounting ISO...", 20, true, false, {});
    std::wstring mountPoint;
    if (!MountISO(isoPath, &mountPoint, log)) {
        setProg(InstallStage::Error, L"ISO mount failed", 0, false, true, L"ISO mount failed");
        log(L"=== Installation failed ===");
        return;
    }

    // 5. Extract
    setProg(InstallStage::Extracting, L"Extracting ISO contents...", 25, true, false, {});
    std::wstring dest = driveLetter + L":\\" + targetPath;
    bool extractOk = ExtractISO(mountPoint, dest, log);

    // Unmount regardless
    UnmountISO(isoPath, log);

    if (!extractOk) {
        setProg(InstallStage::Error, L"Extraction failed", 0, false, true, L"Extraction failed");
        log(L"=== Installation failed ===");
        return;
    }
    setProg(InstallStage::Extracting, L"Extraction complete", 75, true, false, {});

    // 6. Create boot entry
    setProg(InstallStage::BCD, L"Configuring Windows Boot Manager...", 80, true, false, {});
    BootOptions opts;
    opts.description = entryName;
    opts.driveLetter = driveLetter;
    opts.bootPath = L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi";
    std::wstring guid = AddInstallerEntry(opts, log);
    if (guid.empty()) {
        setProg(InstallStage::Error, L"Boot entry creation failed", 0, false, true, L"Boot entry creation failed");
        log(L"=== Installation failed ===");
        return;
    }

    if (setDefault) {
        setProg(InstallStage::BCD, L"Setting as default boot option...", 90, true, false, {});
        if (!SetDefaultBoot(guid, log)) {
            setProg(InstallStage::Error, L"Set default failed", 0, false, true, L"Set default failed");
            log(L"=== Installation failed ===");
            return;
        }
    }

    log(L"=== Installation complete ===");
    setProg(InstallStage::Complete, L"Installation ready! You can now reboot.", 100, false, false, {});
}

Wizard::Wizard() {}

Wizard::~Wizard() {
    if (hDarkBrush_) DeleteObject(hDarkBrush_);
    if (hEditBrush_) DeleteObject(hEditBrush_);
}

bool Wizard::Create(HINSTANCE hInstance, int nCmdShow) {
    hInst_ = hInstance;
    appState_ = std::make_unique<AppState>();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"InternalBootWizard";
    if (!RegisterClassExW(&wc)) return false;

    hwnd_ = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"InternalBootWizard",
        L"InternalBoot - USB-free Windows Installation",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 720,
        nullptr, nullptr, hInstance, this
    );

    if (!hwnd_) return false;

    Theme::EnableDarkMode(hwnd_);
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
    return true;
}

int Wizard::Run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK Wizard::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<Wizard*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
        return TRUE;
    }

    auto* self = reinterpret_cast<Wizard*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) {
        return self->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Wizard::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        OnCreate();
        return 0;
    case WM_SIZE:
        OnSize();
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);
        OnPaint(hdc);
        EndPaint(hwnd_, &ps);
        return 0;
    }
    case WM_DRAWITEM: {
        auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        OnDrawItem(dis);
        return TRUE;
    }
    case WM_COMMAND:
        OnCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
        return 0;
    case WM_TIMER:
        OnTimer();
        return 0;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        return reinterpret_cast<LRESULT>(OnCtlColor(reinterpret_cast<HDC>(wParam), reinterpret_cast<HWND>(lParam), msg));
    case WM_DESTROY:
        if (timerId_) KillTimer(hwnd_, timerId_);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

void Wizard::OnCreate() {
    hDarkBrush_ = CreateSolidBrush(Theme::BG);
    hEditBrush_ = CreateSolidBrush(Theme::EDIT_BG);

    // Partition combo
    hPartitionCombo_ = CreateWindowW(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | CBS_HASSTRINGS,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_PARTITION_COMBO), hInst_, nullptr);
    SendMessageW(hPartitionCombo_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::FontDefault()), TRUE);

    // ISO path edit
    hIsoPath_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_ISO_PATH), hInst_, nullptr);
    SendMessageW(hIsoPath_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::FontDefault()), TRUE);

    // Browse button
    hBrowseBtn_ = CreateWindowW(L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_BROWSE_BTN), hInst_, nullptr);
    SendMessageW(hBrowseBtn_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::FontDefault()), TRUE);

    // Entry name edit
    hEntryName_ = CreateWindowW(L"EDIT", L"InternalBoot Installer",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_ENTRY_NAME), hInst_, nullptr);
    SendMessageW(hEntryName_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::FontDefault()), TRUE);

    // Set default button (owner-draw toggle)
    hSetDefault_ = CreateWindowW(L"BUTTON", L"Set as default boot entry",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_SET_DEFAULT), hInst_, nullptr);
    SendMessageW(hSetDefault_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::FontDefault()), TRUE);

    // Start button
    hStartBtn_ = CreateWindowW(L"BUTTON", L"START",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_START_BTN), hInst_, nullptr);
    SendMessageW(hStartBtn_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::FontBold(11)), TRUE);

    // Reboot button (hidden)
    hRebootBtn_ = CreateWindowW(L"BUTTON", L"Reboot Now",
        WS_CHILD | BS_OWNERDRAW,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_REBOOT_BTN), hInst_, nullptr);
    SendMessageW(hRebootBtn_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::FontBold(11)), TRUE);

    // Cleanup button (hidden)
    hCleanupBtn_ = CreateWindowW(L"BUTTON", L"Remove Boot Entry",
        WS_CHILD | BS_OWNERDRAW,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_CLEANUP_BTN), hInst_, nullptr);
    SendMessageW(hCleanupBtn_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::FontDefault()), TRUE);

    // Progress bar
    hProgress_ = CreateWindowW(PROGRESS_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_PROGRESS), hInst_, nullptr);
    // Disable visual styles so PBM_SETBARCOLOR works
    SetWindowTheme(hProgress_, L"", L"");
    SendMessageW(hProgress_, PBM_SETBARCOLOR, 0, Theme::PRIMARY);
    SendMessageW(hProgress_, PBM_SETBKCOLOR, 0, Theme::PANEL);
    SendMessageW(hProgress_, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    // Log edit
    hLog_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(IDC_LOG), hInst_, nullptr);
    SendMessageW(hLog_, WM_SETFONT, reinterpret_cast<WPARAM>(Theme::Font(9, false)), TRUE);

    LoadPartitions();
    LayoutControls();

    timerId_ = SetTimer(hwnd_, 1, 200, nullptr);
}

void Wizard::OnSize() {
    LayoutControls();
}

void Wizard::LayoutControls() {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int cx = rc.right;
    int cy = rc.bottom;

    int x = MARGIN_X;
    int y = MARGIN_Y;
    int ctrlW = cx - MARGIN_X * 2 - LABEL_W - 8;
    int btnW = 80;

    // Row 1: Partition
    SetWindowPos(hPartitionCombo_, nullptr, x + LABEL_W + 8, y, ctrlW, 160, SWP_NOZORDER);
    y += ROW_H + ROW_GAP;

    // Row 2: ISO path + Browse
    SetWindowPos(hIsoPath_, nullptr, x + LABEL_W + 8, y, ctrlW - btnW - 8, ROW_H, SWP_NOZORDER);
    SetWindowPos(hBrowseBtn_, nullptr, x + LABEL_W + 8 + ctrlW - btnW, y, btnW, ROW_H, SWP_NOZORDER);
    y += ROW_H + ROW_GAP;

    // Row 3: Entry name
    SetWindowPos(hEntryName_, nullptr, x + LABEL_W + 8, y, ctrlW, ROW_H, SWP_NOZORDER);
    y += ROW_H + ROW_GAP;

    // Row 4: Set default
    SetWindowPos(hSetDefault_, nullptr, x + LABEL_W + 8, y, 200, ROW_H, SWP_NOZORDER);
    y += ROW_H + ROW_GAP * 2;

    // Buttons area
    int btnY = y;
    int startW = 120;
    SetWindowPos(hStartBtn_, nullptr, x + LABEL_W + 8, btnY, startW, BTN_H, SWP_NOZORDER);
    SetWindowPos(hRebootBtn_, nullptr, x + LABEL_W + 8, btnY, startW, BTN_H, SWP_NOZORDER);
    SetWindowPos(hCleanupBtn_, nullptr, x + LABEL_W + 8 + startW + 8, btnY, 140, BTN_H, SWP_NOZORDER);
    y += BTN_H + ROW_GAP;

    // Progress bar
    SetWindowPos(hProgress_, nullptr, x, y, cx - MARGIN_X * 2, PROGRESS_H, SWP_NOZORDER);
    y += PROGRESS_H + PROGRESS_GAP;

    // Log area (fill remaining)
    int logH = cy - y - MARGIN_Y;
    if (logH < 60) logH = 60;
    SetWindowPos(hLog_, nullptr, x, y, cx - MARGIN_X * 2, logH, SWP_NOZORDER);
}

void Wizard::OnPaint(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    Theme::FillRectColor(hdc, rc, Theme::BG);

    int x = MARGIN_X;
    int y = MARGIN_Y;
    HFONT bold = Theme::FontBold(10);

    auto drawLabel = [&](const wchar_t* text, int rowY) {
        RECT lr = { x, rowY, x + LABEL_W, rowY + ROW_H };
        SetTextColor(hdc, Theme::TEXT_SECONDARY);
        SetBkMode(hdc, TRANSPARENT);
        HFONT old = (HFONT)SelectObject(hdc, bold);
        DrawTextW(hdc, text, -1, &lr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);
    };

    drawLabel(L"Device", y);
    y += ROW_H + ROW_GAP;
    drawLabel(L"Boot selection", y);
    y += ROW_H + ROW_GAP;
    drawLabel(L"Volume label", y);
    y += ROW_H + ROW_GAP;
    drawLabel(L"Options", y);
}

void Wizard::OnDrawItem(const DRAWITEMSTRUCT* dis) {
    if (dis->CtlType != ODT_BUTTON) return;

    bool isDefault = (dis->itemState & ODS_DEFAULT) != 0;
    bool isPressed = (dis->itemState & ODS_SELECTED) != 0;
    bool isHover = (dis->itemState & ODS_HOTLIGHT) != 0;
    bool isDisabled = (dis->itemState & ODS_DISABLED) != 0;

    COLORREF bg, text;
    if (dis->CtlID == IDC_START_BTN) {
        bg = isPressed ? RGB(40, 160, 220) : isHover ? RGB(70, 200, 255) : Theme::PRIMARY;
        text = Theme::BG;
    } else if (dis->CtlID == IDC_REBOOT_BTN) {
        bg = isPressed ? RGB(30, 170, 70) : isHover ? RGB(50, 210, 100) : Theme::CLR_SUCCESS;
        text = RGB(255, 255, 255);
    } else if (dis->CtlID == IDC_CLEANUP_BTN || dis->CtlID == IDC_BROWSE_BTN) {
        bg = isPressed ? Theme::BTN_SECONDARY_HOVER : isHover ? Theme::BTN_SECONDARY_HOVER : Theme::BTN_SECONDARY;
        text = Theme::TEXT_PRIMARY;
    } else if (dis->CtlID == IDC_SET_DEFAULT) {
        // Checkbox-like toggle
        bg = setDefault_ ? Theme::PRIMARY : Theme::BTN_SECONDARY;
        text = setDefault_ ? Theme::BG : Theme::TEXT_PRIMARY;
    } else {
        bg = Theme::BTN_SECONDARY;
        text = Theme::TEXT_PRIMARY;
    }

    if (isDisabled) {
        bg = Theme::BTN_SECONDARY;
        text = Theme::TEXT_MUTED;
    }

    HBRUSH brush = CreateSolidBrush(bg);
    FillRect(dis->hDC, &dis->rcItem, brush);
    DeleteObject(brush);

    HPEN pen = CreatePen(PS_SOLID, 1, Theme::BORDER);
    HPEN oldPen = (HPEN)SelectObject(dis->hDC, pen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBrush = (HBRUSH)SelectObject(dis->hDC, nullBrush);
    RoundRect(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom, 6, 6);
    SelectObject(dis->hDC, oldBrush);
    SelectObject(dis->hDC, oldPen);
    DeleteObject(pen);

    RECT tr = dis->rcItem;
    InflateRect(&tr, -4, -4);
    SetTextColor(dis->hDC, text);
    SetBkMode(dis->hDC, TRANSPARENT);
    HFONT oldFont = nullptr;
    if (dis->CtlID == IDC_START_BTN || dis->CtlID == IDC_REBOOT_BTN)
        oldFont = (HFONT)SelectObject(dis->hDC, Theme::FontBold(11));
    else
        oldFont = (HFONT)SelectObject(dis->hDC, Theme::FontDefault());
    wchar_t btnText[256] = {};
    GetWindowTextW(dis->hwndItem, btnText, 256);
    const wchar_t* btnLabel = dis->CtlID == IDC_SET_DEFAULT ? (setDefault_ ? L"[x] Set as default boot entry" : L"[ ] Set as default boot entry") : btnText;
    DrawTextW(dis->hDC, btnLabel, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dis->hDC, oldFont);
}

void Wizard::OnCommand(int id, int code, HWND ctl) {
    if (code == CBN_SELCHANGE && ctl == hPartitionCombo_) {
        int idx = static_cast<int>(SendMessageW(hPartitionCombo_, CB_GETCURSEL, 0, 0));
        if (idx >= 0 && idx < static_cast<int>(partitions_.size())) {
            // selected
        }
        return;
    }

    if (code != BN_CLICKED) return;

    switch (id) {
    case IDC_BROWSE_BTN:
        BrowseISO();
        break;
    case IDC_SET_DEFAULT:
        setDefault_ = !setDefault_;
        InvalidateRect(hSetDefault_, nullptr, FALSE);
        break;
    case IDC_START_BTN:
        StartInstallation();
        break;
    case IDC_REBOOT_BTN:
        PerformReboot();
        break;
    case IDC_CLEANUP_BTN:
        PerformCleanup();
        break;
    }
}

void Wizard::OnTimer() {
    PollProgress();
    UpdateLog();
}

HBRUSH Wizard::OnCtlColor(HDC hdc, HWND ctl, UINT type) {
    if (type == WM_CTLCOLOREDIT || type == WM_CTLCOLORLISTBOX) {
        SetBkColor(hdc, Theme::EDIT_BG);
        SetTextColor(hdc, Theme::TEXT_PRIMARY);
        return hEditBrush_;
    }
    if (type == WM_CTLCOLORSTATIC) {
        SetBkColor(hdc, Theme::BG);
        SetTextColor(hdc, Theme::TEXT_PRIMARY);
        return hDarkBrush_;
    }
    if (type == WM_CTLCOLORBTN) {
        SetBkColor(hdc, Theme::BG);
        return hDarkBrush_;
    }
    return hDarkBrush_;
}

void Wizard::LoadPartitions() {
    partitions_ = GetPartitions(nullptr);
    SendMessageW(hPartitionCombo_, CB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < partitions_.size(); ++i) {
        const auto& p = partitions_[i];
        std::wostringstream oss;
        oss << p.driveLetter << L": " << (p.label.empty() ? L"Local Disk" : p.label)
            << L"  (" << p.fileSystem << L", " << Utf8ToWide(p.sizeGB)
            << L", " << Utf8ToWide(FormatSize(p.freeBytes)) << L" free)";
        int idx = static_cast<int>(SendMessageW(hPartitionCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(oss.str().c_str())));
        SendMessageW(hPartitionCombo_, CB_SETITEMDATA, idx, static_cast<LPARAM>(i));
    }
    if (!partitions_.empty()) {
        SendMessageW(hPartitionCombo_, CB_SETCURSEL, 0, 0);
    }
}

void Wizard::BrowseISO() {
    wchar_t fileName[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"ISO Files (*.iso)\0*.iso\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = L"Select Windows ISO";

    if (GetOpenFileNameW(&ofn)) {
        SetWindowTextW(hIsoPath_, fileName);
    }
}

void Wizard::StartInstallation() {
    if (appState_->IsRunning()) return;

    // Gather inputs
    wchar_t isoPath[MAX_PATH];
    GetWindowTextW(hIsoPath_, isoPath, MAX_PATH);
    if (wcslen(isoPath) == 0) {
        MessageBoxW(hwnd_, L"Please select a Windows ISO file.", L"Missing ISO", MB_OK | MB_ICONWARNING);
        return;
    }

    int idx = static_cast<int>(SendMessageW(hPartitionCombo_, CB_GETCURSEL, 0, 0));
    if (idx < 0 || idx >= static_cast<int>(partitions_.size())) {
        MessageBoxW(hwnd_, L"Please select a target partition.", L"Missing Partition", MB_OK | MB_ICONWARNING);
        return;
    }
    std::wstring driveLetter = partitions_[idx].driveLetter;

    wchar_t entryName[256];
    GetWindowTextW(hEntryName_, entryName, 256);
    if (wcslen(entryName) == 0) {
        wcscpy_s(entryName, L"InternalBoot Installer");
    }

    appState_->Reset();
    SetWindowTextW(hLog_, L"");
    hasCompleted_ = false;
    SetControlsEnabled(false);

    std::thread t(RunInstallThread,
        std::wstring(isoPath),
        driveLetter,
        std::wstring(L"InternalBoot_Installer"),
        std::wstring(entryName),
        setDefault_,
        appState_.get()
    );
    t.detach();
}

void Wizard::PollProgress() {
    auto prog = appState_->GetProgress();
    SendMessageW(hProgress_, PBM_SETPOS, prog.percent, 0);

    if (prog.hasError) {
        SetControlsEnabled(true);
        ShowWindow(hStartBtn_, SW_SHOW);
        ShowWindow(hRebootBtn_, SW_HIDE);
        ShowWindow(hCleanupBtn_, SW_HIDE);
        // Show error in caption temporarily or rely on log
        return;
    }

    if (prog.stage == InstallStage::Complete && !hasCompleted_) {
        hasCompleted_ = true;
        SetControlsEnabled(true);
        ShowWindow(hStartBtn_, SW_HIDE);
        ShowWindow(hRebootBtn_, SW_SHOW);
        ShowWindow(hCleanupBtn_, SW_SHOW);
    }
}

void Wizard::UpdateLog() {
    auto lines = appState_->FlushLogs();
    if (lines.empty()) return;

    std::wostringstream oss;
    for (const auto& line : lines) {
        oss << line << L"\r\n";
    }

    int len = GetWindowTextLengthW(hLog_);
    SendMessageW(hLog_, EM_SETSEL, len, len);
    SendMessageW(hLog_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(oss.str().c_str()));
    SendMessageW(hLog_, EM_SCROLLCARET, 0, 0);

    // Limit log size to ~5000 lines
    int lineCount = static_cast<int>(SendMessageW(hLog_, EM_GETLINECOUNT, 0, 0));
    if (lineCount > 5000) {
        int idx = static_cast<int>(SendMessageW(hLog_, EM_LINEINDEX, lineCount - 5000, 0));
        SendMessageW(hLog_, EM_SETSEL, 0, idx);
        SendMessageW(hLog_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L""));
        SendMessageW(hLog_, EM_SETSEL, -1, -1);
    }
}

void Wizard::SetControlsEnabled(bool enabled) {
    EnableWindow(hPartitionCombo_, enabled);
    EnableWindow(hBrowseBtn_, enabled);
    EnableWindow(hEntryName_, enabled);
    EnableWindow(hSetDefault_, enabled);
    EnableWindow(hStartBtn_, enabled);
    EnableWindow(hRebootBtn_, enabled && hasCompleted_);
    EnableWindow(hCleanupBtn_, enabled && hasCompleted_);
    // Force repaint of owner-drawn buttons
    InvalidateRect(hBrowseBtn_, nullptr, FALSE);
    InvalidateRect(hSetDefault_, nullptr, FALSE);
    InvalidateRect(hStartBtn_, nullptr, FALSE);
    InvalidateRect(hRebootBtn_, nullptr, FALSE);
    InvalidateRect(hCleanupBtn_, nullptr, FALSE);
}

void Wizard::PerformReboot() {
    if (MessageBoxW(hwnd_, L"The system will restart now. Continue?", L"Reboot", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        RebootSystem([](const std::wstring&) {});
    }
}

void Wizard::PerformCleanup() {
    if (MessageBoxW(hwnd_, L"Remove the InternalBoot installer boot entry?", L"Cleanup", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        SetControlsEnabled(false);
        appState_->Reset();
        SetWindowTextW(hLog_, L"");

        std::thread t([this]() {
            auto log = appState_->Logger();
            log(L"Removing boot entry...");
            bool ok = RemoveInstallerEntry(log);
            log(ok ? L"Boot entry removed." : L"Failed to remove boot entry.");
            appState_->SetProgress({InstallStage::Idle, ok ? L"Cleanup complete." : L"Cleanup failed.", 100, false, !ok, ok ? std::wstring{} : L"Cleanup failed", false, false});
        });
        t.detach();
    }
}
