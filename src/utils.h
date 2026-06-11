#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <windows.h>

using LogCallback = std::function<void(const std::wstring&)>;

std::wstring Utf8ToWide(const std::string& utf8);
std::string WideToUtf8(const std::wstring& wide);

std::wstring GetEnvVarW(const wchar_t* name);
std::string GetEnvVarA(const char* name);

// Run a command and capture stdout. Returns true if process started.
// exitCode is set to the process exit code (0 usually means success).
bool RunCommand(const std::wstring& cmd, std::string* output, int* exitCode);
bool RunCommand(const std::wstring& cmd, std::string* output, int* exitCode, LogCallback log);
bool RunCommand(const std::wstring& cmd);

std::string FormatSize(uint64_t bytes);
std::wstring GetExecutableDirW();
std::wstring PathJoin(const std::wstring& a, const std::wstring& b);
std::wstring PathJoin(const std::wstring& a, const std::wstring& b, const std::wstring& c);
std::wstring ToLowerW(const std::wstring& s);
std::string Trim(const std::string& s);
std::wstring TrimW(const std::wstring& s);
bool FileExists(const std::wstring& path);
uint64_t FileSize(const std::wstring& path);
bool CreateDirectoryRecursive(const std::wstring& path);
bool DeleteDirectoryRecursive(const std::wstring& path);
std::wstring GetSystemDriveW();
