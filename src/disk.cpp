#include "disk.h"
#include <sstream>
#include <regex>

std::vector<Partition> GetPartitions(LogCallback log) {
    if (log) log(L"Enumerating partitions...");
    std::vector<Partition> result;

    std::wstring cmd = L"powershell.exe -NoProfile -Command \"Get-Volume | Where-Object {$_.DriveLetter} | Select-Object DriveLetter,FileSystemLabel,FileSystem,Size,SizeRemaining | ConvertTo-Json -Compress\"";

    std::string out;
    int exitCode = 0;
    if (!RunCommand(cmd, &out, &exitCode, log) || exitCode != 0) {
        if (log) log(L"  Failed to enumerate partitions.");
        return result;
    }

    // PowerShell JSON may be a single object or an array.
    // For simplicity, parse the key fields with regex since we control the output shape.
    std::regex objRegex(R"(\{([^}]*)\})");
    std::regex driveRegex(R"("DriveLetter"\s*:\s*"?([^",}]+)"?)");
    std::regex labelRegex(R"("FileSystemLabel"\s*:\s*"([^"]*)")");
    std::regex fsRegex(R"("FileSystem"\s*:\s*"([^"]*)")");
    std::regex sizeRegex(R"("Size"\s*:\s*(\d+))");
    std::regex freeRegex(R"("SizeRemaining"\s*:\s*(\d+))");

    std::string::const_iterator searchStart(out.cbegin());
    std::smatch m;
    while (std::regex_search(searchStart, out.cend(), m, objRegex)) {
        std::string block = m[1].str();
        Partition p;

        std::smatch dm;
        if (std::regex_search(block, dm, driveRegex)) {
            p.driveLetter = Utf8ToWide(Trim(dm[1].str()));
        }
        std::smatch lm;
        if (std::regex_search(block, lm, labelRegex)) {
            p.label = Utf8ToWide(lm[1].str());
        }
        std::smatch fm;
        if (std::regex_search(block, fm, fsRegex)) {
            p.fileSystem = Utf8ToWide(fm[1].str());
        }
        std::smatch sm;
        if (std::regex_search(block, sm, sizeRegex)) {
            p.sizeBytes = std::stoull(sm[1].str());
            p.sizeGB = FormatSize(p.sizeBytes);
        }
        std::smatch rm;
        if (std::regex_search(block, rm, freeRegex)) {
            p.freeBytes = std::stoull(rm[1].str());
        }

        if (!p.driveLetter.empty()) {
            result.push_back(p);
        }
        searchStart = m.suffix().first;
    }

    if (log) log(L"  Found " + std::to_wstring(result.size()) + L" partition(s).");
    return result;
}

bool ValidateSpace(const std::wstring& driveLetter, uint64_t requiredBytes, LogCallback log) {
    if (log) log(L"Checking free space on " + driveLetter + L":...");
    std::wostringstream cmd;
    cmd << L"powershell.exe -NoProfile -Command \"(Get-Volume -DriveLetter " << driveLetter << L").SizeRemaining\"";

    std::string out;
    int exitCode = 0;
    if (!RunCommand(cmd.str(), &out, &exitCode, log) || exitCode != 0) {
        if (log) log(L"  Failed to query free space.");
        return false;
    }

    std::string num = Trim(out);
    if (num.empty()) {
        if (log) log(L"  Error: could not parse free space.");
        return false;
    }
    try {
        uint64_t remaining = std::stoull(num);
        if (remaining < requiredBytes) {
            if (log) {
                log(L"  Insufficient space: required " + Utf8ToWide(FormatSize(requiredBytes)) +
                    L", available " + Utf8ToWide(FormatSize(remaining)));
            }
            return false;
        }
        if (log) log(L"  Space OK.");
        return true;
    } catch (...) {
        if (log) log(L"  Error: failed to parse free space value.");
        return false;
    }
}
