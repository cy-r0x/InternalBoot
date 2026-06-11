#pragma once
#include <string>
#include <cstdint>
#include <functional>
#include "utils.h"

struct IsoInfo {
    std::wstring path;
    uint64_t sizeBytes = 0;
    std::string sizeGB;
    bool isValid = false;
};

bool ValidateISO(const std::wstring& path, IsoInfo* info, LogCallback log = nullptr);
bool MountISO(const std::wstring& path, std::wstring* mountPoint, LogCallback log = nullptr);
bool UnmountISO(const std::wstring& path, LogCallback log = nullptr);
bool ExtractISO(const std::wstring& mountPoint, const std::wstring& dest, LogCallback log = nullptr);
