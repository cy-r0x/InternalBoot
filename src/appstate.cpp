#include "appstate.h"

void AppState::SetProgress(const Progress& p) {
    std::lock_guard<std::mutex> lock(mutex_);
    progress_ = p;
}

Progress AppState::GetProgress() {
    std::lock_guard<std::mutex> lock(mutex_);
    return progress_;
}

void AppState::AddLog(const std::wstring& line) {
    std::lock_guard<std::mutex> lock(mutex_);
    logs_.push_back(line);
}

std::vector<std::wstring> AppState::FlushLogs() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::wstring> out;
    out.swap(logs_);
    return out;
}

void AppState::ClearLogs() {
    std::lock_guard<std::mutex> lock(mutex_);
    logs_.clear();
}

bool AppState::IsRunning() {
    std::lock_guard<std::mutex> lock(mutex_);
    return progress_.isRunning;
}

void AppState::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    progress_ = Progress{};
    logs_.clear();
}

std::function<void(const std::wstring&)> AppState::Logger() {
    return [this](const std::wstring& line) {
        this->AddLog(line);
    };
}
