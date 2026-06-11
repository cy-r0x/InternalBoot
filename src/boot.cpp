#include "boot.h"
#include "bcd.h"
#include <sstream>

std::wstring AddInstallerEntry(const BootOptions& opts, LogCallback log) {
    if (log) log(L"Adding boot entry: " + opts.description);

    std::wstring bootPath = opts.bootPath;
    if (bootPath.empty() || bootPath[0] != L'\\') {
        bootPath = L"\\" + bootPath;
    }

    std::wostringstream copyCmd;
    copyCmd << L"bcdedit /copy {current} /d \"" << opts.description << L"\"";

    std::string copyOut;
    int exitCode = 0;
    if (!RunCommand(copyCmd.str(), &copyOut, &exitCode, log) || exitCode != 0) {
        if (log) log(L"  Failed to copy current entry.");
        return {};
    }

    // Parse GUID from output: "The entry was successfully copied to {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}."
    std::string out = copyOut;
    size_t start = out.find('{');
    size_t end = out.find('}');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        if (log) log(L"  Failed to parse new BCD GUID.");
        return {};
    }
    std::wstring guid = Utf8ToWide(out.substr(start, end - start + 1));
    if (log) log(L"  Created entry with GUID: " + guid);

    std::wstring devicePath = L"partition=" + opts.driveLetter + L":";

    auto setProp = [&](const wchar_t* prop, const std::wstring& val) -> bool {
        std::wostringstream c;
        c << L"bcdedit /set " << guid << L" " << prop << L" " << val;
        std::string o;
        int ec = 0;
        bool ok = RunCommand(c.str(), &o, &ec, log) && ec == 0;
        if (log) log(ok ? (std::wstring(L"  Set ") + prop + L" OK.") : (std::wstring(L"  Failed to set ") + prop));
        return ok;
    };

    if (!setProp(L"device", devicePath)) return {};
    if (!setProp(L"path", bootPath)) return {};
    if (!setProp(L"osdevice", devicePath)) return {};

    std::wostringstream dispCmd;
    dispCmd << L"bcdedit /displayorder " << guid << L" /addlast";
    std::string dispOut;
    int dispEc = 0;
    bool dispOk = RunCommand(dispCmd.str(), &dispOut, &dispEc, log) && dispEc == 0;
    if (log) log(dispOk ? L"  Added to display order." : L"  Failed to set display order.");
    if (!dispOk) return {};

    return guid;
}

bool SetDefaultBoot(const std::wstring& identifier, LogCallback log) {
    if (log) log(L"Setting default boot entry: " + identifier);
    std::wostringstream cmd;
    cmd << L"bcdedit /default " << identifier;
    std::string out;
    int exitCode = 0;
    bool ok = RunCommand(cmd.str(), &out, &exitCode, log) && exitCode == 0;
    if (log) log(ok ? L"  Default set." : L"  Failed to set default.");
    return ok;
}

bool RemoveInstallerEntry(LogCallback log) {
    if (log) log(L"Removing InternalBoot installer entry...");
    BcdEntry entry;
    if (!FindBcdEntryByDescription(L"InternalBoot Installer", &entry, log)) {
        if (log) log(L"  Entry not found (already removed).");
        return true;
    }
    return RemoveBcdEntry(entry.identifier, log);
}

bool RebootSystem(LogCallback log) {
    if (log) log(L"Triggering system reboot...");
    std::string out;
    int exitCode = 0;
    bool ok = RunCommand(L"shutdown /r /t 0", &out, &exitCode, log) && exitCode == 0;
    if (log) log(ok ? L"  Reboot command issued." : L"  Reboot command failed.");
    return ok;
}
