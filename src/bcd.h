#pragma once
#include <string>
#include <vector>
#include "utils.h"

struct BcdEntry {
    std::wstring identifier;
    std::wstring description;
    std::wstring device;
    std::wstring path;
};

std::wstring BackupBCD(LogCallback log = nullptr);
bool RestoreBCD(const std::wstring& backupPath, LogCallback log = nullptr);
std::vector<BcdEntry> ListBcdEntries(LogCallback log = nullptr);
bool RemoveBcdEntry(const std::wstring& identifier, LogCallback log = nullptr);
bool FindBcdEntryByDescription(const std::wstring& description, BcdEntry* entry, LogCallback log = nullptr);
