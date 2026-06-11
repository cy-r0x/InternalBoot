#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include "iso.h"
#include "disk.h"

enum class InstallStage {
    Idle,
    Preparing,
    Validating,
    Checking,
    Backup,
    Mounting,
    Extracting,
    BCD,
    Complete,
    Error,
    Cleanup
};

struct Progress {
    InstallStage stage = InstallStage::Idle;
    std::wstring message;
    int percent = 0;
    bool isRunning = false;
    bool hasError = false;
    std::wstring errorMsg;
    bool canReboot = false;
    bool canCleanup = false;
};

class AppState {
public:
    void SetProgress(const Progress& p);
    Progress GetProgress();

    void AddLog(const std::wstring& line);
    std::vector<std::wstring> FlushLogs();
    void ClearLogs();

    bool IsRunning();
    void Reset();

    // Convenience logger usable as LogCallback
    std::function<void(const std::wstring&)> Logger();

private:
    std::mutex mutex_;
    Progress progress_;
    std::vector<std::wstring> logs_;
};
