#include "bcd.h"
#include <sstream>
#include <chrono>
#include <iomanip>

std::wstring BackupBCD(LogCallback log) {
    if (log) log(L"Backing up BCD store...");

    std::wstring sysDrive = GetSystemDriveW();
    std::wstring backupDir = PathJoin(sysDrive, L"InternalBoot", L"Backups");
    if (!CreateDirectoryRecursive(backupDir)) {
        if (log) log(L"  Error: failed to create backup directory.");
        return {};
    }

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _MSC_VER
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::wostringstream ts;
    ts << std::put_time(&tm, L"%Y%m%d_%H%M%S");
    std::wstring backupPath = PathJoin(backupDir, L"BCD_backup_" + ts.str());

    std::wostringstream cmd;
    cmd << L"bcdedit /export \"" << backupPath << L"\"";

    std::string out;
    int exitCode = 0;
    if (!RunCommand(cmd.str(), &out, &exitCode, log) || exitCode != 0) {
        if (log) log(L"  BCD backup failed.");
        return {};
    }
    if (log) log(L"  Backup saved to: " + backupPath);
    return backupPath;
}

bool RestoreBCD(const std::wstring& backupPath, LogCallback log) {
    if (log) log(L"Restoring BCD from: " + backupPath);
    std::wostringstream cmd;
    cmd << L"bcdedit /import \"" << backupPath << L"\" /clean";
    std::string out;
    int exitCode = 0;
    bool ok = RunCommand(cmd.str(), &out, &exitCode, log) && exitCode == 0;
    if (log) log(ok ? L"  Restore successful." : L"  Restore failed.");
    return ok;
}

std::vector<BcdEntry> ListBcdEntries(LogCallback log) {
    if (log) log(L"Listing BCD entries...");
    std::vector<BcdEntry> entries;
    std::string out;
    int exitCode = 0;
    if (!RunCommand(L"bcdedit /enum firmware", &out, &exitCode, log) || exitCode != 0) {
        if (log) log(L"  Failed to list BCD entries.");
        return entries;
    }

    BcdEntry current;
    std::istringstream stream(out);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = Trim(line);
        if (trimmed.empty()) continue;
        if (trimmed.find("identifier") == 0) {
            if (!current.identifier.empty()) entries.push_back(current);
            current = BcdEntry{};
            size_t pos = trimmed.find(' ');
            if (pos != std::string::npos) {
                current.identifier = Utf8ToWide(Trim(trimmed.substr(pos)));
            }
        } else if (trimmed.find("description") == 0) {
            size_t pos = trimmed.find(' ');
            if (pos != std::string::npos) {
                current.description = Utf8ToWide(Trim(trimmed.substr(pos)));
            }
        } else if (trimmed.find("device") == 0) {
            size_t pos = trimmed.find(' ');
            if (pos != std::string::npos) {
                current.device = Utf8ToWide(Trim(trimmed.substr(pos)));
            }
        } else if (trimmed.find("path") == 0) {
            size_t pos = trimmed.find(' ');
            if (pos != std::string::npos) {
                current.path = Utf8ToWide(Trim(trimmed.substr(pos)));
            }
        }
    }
    if (!current.identifier.empty()) entries.push_back(current);
    if (log) log(L"  Found " + std::to_wstring(entries.size()) + L" entr(y/ies).");
    return entries;
}

bool RemoveBcdEntry(const std::wstring& identifier, LogCallback log) {
    if (log) log(L"Removing BCD entry: " + identifier);
    std::wostringstream cmd;
    cmd << L"bcdedit /delete " << identifier;
    std::string out;
    int exitCode = 0;
    bool ok = RunCommand(cmd.str(), &out, &exitCode, log) && exitCode == 0;
    if (log) log(ok ? L"  Entry removed." : L"  Failed to remove entry.");
    return ok;
}

bool FindBcdEntryByDescription(const std::wstring& description, BcdEntry* entry, LogCallback log) {
    auto entries = ListBcdEntries(log);
    for (const auto& e : entries) {
        std::wstring descLower = ToLowerW(e.description);
        std::wstring targetLower = ToLowerW(description);
        if (descLower == targetLower) {
            if (entry) *entry = e;
            return true;
        }
    }
    return false;
}
