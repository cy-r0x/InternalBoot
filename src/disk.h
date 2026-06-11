#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "utils.h"

struct Partition {
    int index = 0;
    std::wstring driveLetter;
    std::wstring label;
    std::wstring fileSystem;
    uint64_t sizeBytes = 0;
    std::string sizeGB;
    bool isBootable = false;
    std::wstring type;
    uint64_t freeBytes = 0;
};

std::vector<Partition> GetPartitions(LogCallback log = nullptr);
bool ValidateSpace(const std::wstring& driveLetter, uint64_t requiredBytes, LogCallback log = nullptr);
