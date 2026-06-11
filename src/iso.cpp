#include "iso.h"
#include <fstream>
#include <sstream>
#include <algorithm>

bool ValidateISO(const std::wstring& path, IsoInfo* info, LogCallback log) {
    if (log) log(L"Validating ISO: " + path);

    if (!FileExists(path)) {
        if (log) log(L"  Error: file does not exist");
        return false;
    }

    std::wstring ext = path.substr(path.find_last_of(L'.'));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    if (ext != L".iso") {
        if (log) log(L"  Error: file is not an ISO");
        return false;
    }

    uint64_t sz = FileSize(path);
    constexpr uint64_t MIN_ISO_SIZE = 3ULL * 1024 * 1024 * 1024;
    if (sz < MIN_ISO_SIZE) {
        if (log) log(L"  Error: ISO too small to be a valid Windows installer");
        return false;
    }

    if (info) {
        info->path = path;
        info->sizeBytes = sz;
        info->sizeGB = FormatSize(sz);
        info->isValid = true;
    }
    if (log) log(L"  ISO size: " + Utf8ToWide(FormatSize(sz)));
    return true;
}

bool MountISO(const std::wstring& path, std::wstring* mountPoint, LogCallback log) {
    if (log) log(L"Mounting ISO...");
    std::wostringstream cmd;
    cmd << L"powershell.exe -NoProfile -Command \"(Mount-DiskImage -ImagePath '";
    cmd << path;
    cmd << L"' -PassThru | Get-Volume).DriveLetter\"";

    std::string out;
    int exitCode = 0;
    if (!RunCommand(cmd.str(), &out, &exitCode, log) || exitCode != 0) {
        if (log) log(L"  Mount failed.");
        return false;
    }

    std::wstring letter = TrimW(Utf8ToWide(out));
    if (letter.empty()) {
        if (log) log(L"  Error: ISO mounted but drive letter not found");
        return false;
    }
    if (mountPoint) *mountPoint = letter + L":\\";
    if (log) log(L"  Mounted at " + letter + L":");
    return true;
}

bool UnmountISO(const std::wstring& path, LogCallback log) {
    if (log) log(L"Unmounting ISO...");
    std::wostringstream cmd;
    cmd << L"powershell.exe -NoProfile -Command \"Dismount-DiskImage -ImagePath '";
    cmd << path;
    cmd << L"'\"";

    std::string out;
    int exitCode = 0;
    if (!RunCommand(cmd.str(), &out, &exitCode, log) || exitCode != 0) {
        if (log) log(L"  Unmount warning: command returned non-zero.");
        return false;
    }
    if (log) log(L"  ISO unmounted.");
    return true;
}

bool ExtractISO(const std::wstring& mountPoint, const std::wstring& dest, LogCallback log) {
    if (log) log(L"Extracting ISO contents to: " + dest);

    if (!CreateDirectoryRecursive(dest)) {
        if (log) log(L"  Error: failed to create destination directory");
        return false;
    }

    std::wostringstream cmd;
    cmd << L"robocopy \"" << mountPoint << L"\" \"" << dest << L"\" /E /NFL /NDL /NJH /NJS /nc /ns /np";

    std::string out;
    int exitCode = 0;
    if (!RunCommand(cmd.str(), &out, &exitCode, log)) {
        if (log) log(L"  Error: failed to run robocopy");
        return false;
    }
    // Robocopy exit codes 0-7 are success
    if (exitCode >= 0 && exitCode <= 7) {
        if (log) log(L"  Extraction complete.");
        return true;
    }
    if (log) log(L"  Error: robocopy failed with exit code " + std::to_wstring(exitCode));
    return false;
}
