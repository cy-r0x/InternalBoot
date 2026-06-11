#include "utils.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 0) return {};
    std::wstring wide(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);
    return wide;
}

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string utf8(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], size, nullptr, nullptr);
    return utf8;
}

std::wstring GetEnvVarW(const wchar_t* name) {
    DWORD len = GetEnvironmentVariableW(name, nullptr, 0);
    if (len == 0) return {};
    std::wstring value(len - 1, L'\0');
    GetEnvironmentVariableW(name, &value[0], len);
    return value;
}

std::string GetEnvVarA(const char* name) {
    char* val = getenv(name);
    return val ? val : "";
}

static void FlushLogBuffer(const std::string& buf, std::string* output, LogCallback log) {
    if (output) output->append(buf);
    if (log) {
        size_t start = 0;
        while (start < buf.size()) {
            size_t end = buf.find('\n', start);
            if (end == std::string::npos) end = buf.size();
            std::string line = buf.substr(start, end - start);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            log(Utf8ToWide(line));
            start = end + 1;
        }
    }
}

bool RunCommand(const std::wstring& cmd, std::string* output, int* exitCode) {
    return RunCommand(cmd, output, exitCode, nullptr);
}

bool RunCommand(const std::wstring& cmd, std::string* output, int* exitCode, LogCallback log) {
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return false;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    std::wstring cmdLine = cmd;

    BOOL created = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, TRUE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);

    if (!created) {
        CloseHandle(hRead);
        return false;
    }

    std::string buffer;
    buffer.reserve(4096);
    char buf[4096];
    DWORD read;
    while (ReadFile(hRead, buf, sizeof(buf), &read, nullptr) && read > 0) {
        buffer.append(buf, read);
        size_t lastNl = buffer.rfind('\n');
        if (lastNl != std::string::npos) {
            std::string complete = buffer.substr(0, lastNl + 1);
            buffer.erase(0, lastNl + 1);
            FlushLogBuffer(complete, output, log);
        }
    }
    if (!buffer.empty()) {
        FlushLogBuffer(buffer, output, log);
    }

    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    *exitCode = static_cast<int>(code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool RunCommand(const std::wstring& cmd) {
    std::string discard;
    int exitCode = 0;
    return RunCommand(cmd, &discard, &exitCode) && exitCode == 0;
}

std::string FormatSize(uint64_t bytes) {
    double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f GB", gb);
    return buf;
}

std::wstring GetExecutableDirW() {
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0) return {};
    std::wstring p(path, len);
    size_t pos = p.find_last_of(L"\\/");
    if (pos != std::wstring::npos) p = p.substr(0, pos);
    return p;
}

std::wstring PathJoin(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == L'\\' || a.back() == L'/') return a + b;
    return a + L"\\" + b;
}

std::wstring PathJoin(const std::wstring& a, const std::wstring& b, const std::wstring& c) {
    return PathJoin(PathJoin(a, b), c);
}

std::wstring ToLowerW(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::towlower);
    return r;
}

std::string Trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::wstring TrimW(const std::wstring& s) {
    size_t start = 0;
    while (start < s.size() && iswspace(s[start])) ++start;
    size_t end = s.size();
    while (end > start && iswspace(s[end - 1])) --end;
    return s.substr(start, end - start);
}

bool FileExists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

uint64_t FileSize(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
        return 0;
    ULARGE_INTEGER size;
    size.LowPart = data.nFileSizeLow;
    size.HighPart = data.nFileSizeHigh;
    return size.QuadPart;
}

bool CreateDirectoryRecursive(const std::wstring& path) {
    if (path.empty()) return false;
    if (CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS)
        return true;
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos || pos == 0) return false;
    if (!CreateDirectoryRecursive(path.substr(0, pos))) return false;
    return CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool DeleteDirectoryRecursive(const std::wstring& path) {
    return RemoveDirectoryW(path.c_str()) != 0;
}

std::wstring GetSystemDriveW() {
    std::wstring sysDrive = GetEnvVarW(L"SystemDrive");
    if (sysDrive.empty()) sysDrive = L"C:";
    return sysDrive;
}
