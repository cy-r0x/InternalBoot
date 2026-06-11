#pragma once
#include <string>
#include "utils.h"

struct BootOptions {
    std::wstring description;
    std::wstring driveLetter;
    std::wstring bootPath;
};

std::wstring AddInstallerEntry(const BootOptions& opts, LogCallback log = nullptr);
bool SetDefaultBoot(const std::wstring& identifier, LogCallback log = nullptr);
bool RemoveInstallerEntry(LogCallback log = nullptr);
bool RebootSystem(LogCallback log = nullptr);
