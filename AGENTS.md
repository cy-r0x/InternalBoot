# AGENTS.md — InternalBoot

> This file contains project-specific context for AI coding agents. If you are reading this, you are expected to know nothing about the project beyond what is written here.

---

## Project Overview

**InternalBoot** is a desktop application that enables USB-free Windows installation. It extracts a Windows ISO file to an internal drive partition and creates a Windows Boot Manager (BCD) entry so the machine can boot directly into the installer without a USB drive.

- **Name**: InternalBoot
- **Author**: Md Whahidul Islam Payel (`whahidulislampayel@gmail.com`)
- **Repository**: `github.com/cy-r0x/internal-boot`
- **Target Platform**: Windows (the application is fundamentally Windows-centric)

---

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Language | C++17 |
| UI Framework | Win32 API (native windows, owner-drawn controls) |
| Graphics | GDI / GDI+ |
| Build System | CMake 3.20+ |

---

## Project Structure

```
InternalBoot/
├── CMakeLists.txt          # CMake project definition
├── README.md               # Human-facing README
├── AGENTS.md               # This file
└── src/
    ├── main.cpp            # WinMain entry point
    ├── wizard.cpp/.h       # Main window: Rufus-style layout (settings, progress bar, log pane)
    ├── appstate.cpp/.h     # Thread-safe progress + log buffer
    ├── theme.cpp/.h        # Dark-theme colours, fonts, GDI helpers
    ├── iso.cpp/.h          # ISO validation, PowerShell mount/unmount, robocopy extraction
    ├── disk.cpp/.h         # Partition enumeration & free-space checks via PowerShell
    ├── bcd.cpp/.h          # BCD store backup, restore, list, remove via bcdedit
    ├── boot.cpp/.h         # Boot entry add, set-default, remove, reboot via bcdedit/shutdown
    └── utils.cpp/.h        # UTF-8/UTF-16 conversion, command runner with live log streaming
```

---

## Build and Development Commands

All commands assume you are in the project root.

### Prerequisites
- Visual Studio 2022 with "Desktop development with C++" workload, **or** MinGW-w64
- CMake 3.20+
- **Administrator privileges are required at runtime**

### Build (Visual Studio)
```powershell
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

### Build (MinGW)
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Output
- `build/Release/InternalBoot.exe` (MSVC)
- `build/InternalBoot.exe` (MinGW)

---

## Code Organization and Architecture

### UI (`wizard.cpp/.h`)
- Single main window (640×720) with a **Rufus-style** layout:
  - Top: setting rows (partition dropdown, ISO path + Browse, entry name, set-default toggle)
  - Middle: action buttons (START, Reboot Now, Remove Boot Entry) + progress bar
  - Bottom: large read-only multiline log edit
- All buttons are **owner-drawn** (`BS_OWNERDRAW`) for dark theming.
- Static labels are drawn manually in `WM_PAINT`.
- Dark theme colours match the original web frontend palette.

### Worker Thread
- Installation runs on a background `std::thread` (`RunInstallThread` in `wizard.cpp`).
- The thread uses `AppState::Logger()` to append log lines and `AppState::SetProgress()` to update progress.
- A `WM_TIMER` (200 ms) polls `AppState` on the UI thread and updates the progress bar and log edit.

### Internal Packages (`src/*.cpp`)
Each module handles a single domain:

| Module | Purpose |
|--------|---------|
| `iso` | Validate ISO size/extension, mount/unmount via PowerShell, extract via robocopy |
| `disk` | Enumerate partitions and check free space via PowerShell (`Get-Volume`) |
| `bcd` | Backup, restore, list, remove BCD entries via `bcdedit` |
| `boot` | Add installer boot entries, set default, remove entry, reboot via `bcdedit`/`shutdown` |
| `utils` | UTF-8/UTF-16 conversion, child-process execution with live stdout capture, path helpers |

Every backend function that shells out accepts an optional `LogCallback` so output can be streamed to the log pane in real time.

---

## Code Style Guidelines

### C++
- C++17 standard.
- Use `std::wstring` for all Windows strings (UTF-16).
- Use `Utf8ToWide` / `WideToUtf8` when interfacing with external UTF-8 data (PowerShell output, file sizes).
- Prefer explicit error handling over exceptions in the UI layer.
- Keep Win32 window message handling in `wizard.cpp` only.

### Dark Theme Colour Palette
- Background: `#0f172a`
- Panel/Surface: `#1e293b`
- Border: `#334155`
- Primary accent: `#38bdf8`
- Success: `#22c55e`
- Error: `#ef4444`
- Text primary: `#e2e8f0`, secondary: `#94a3b8`

---

## Testing Instructions

**There are currently no automated tests in this project.**

Manual testing workflow:
1. Build `InternalBoot.exe` on a Windows machine.
2. Run as Administrator.
3. Select a valid Windows ISO (≥3 GB).
4. Choose a target partition with sufficient free space (ISO size + 2 GB buffer).
5. Click START and observe the log pane.
6. Verify BCD backup is created in `%SystemDrive%\InternalBoot\Backups\`.
7. After completion, verify the boot entry appears in `bcdedit /enum firmware`.
8. Test Reboot Now and Remove Boot Entry buttons.

---

## Security Considerations

- **Administrator Privileges Required**: The application shells out to `bcdedit`, `Mount-DiskImage`, `robocopy`, and `shutdown`.
- **BCD Store Mutation**: The app reads, copies, modifies, and deletes Windows Boot Manager entries. It creates a timestamped backup of the BCD store before modification (`%SystemDrive%\InternalBoot\Backups\BCD_backup_<timestamp>`), but there is no automatic restore on failure.
- **System Reboot**: The reboot button triggers an immediate system restart (`shutdown /r /t 0`).
- **ISO Source Trust**: The app validates ISO files by extension and minimum size only; it does **not** verify cryptographic signatures or checksums.
- **Data Destruction Risk**: `robocopy` is used to extract ISO contents into a user-selected directory. Ensure the target path does not overwrite existing data.

---

## Deployment

CMake compiles everything into a single native executable (`InternalBoot.exe`). No separate web server or files are needed at runtime.

Distribute the executable. The end user must run it as Administrator.

---

## Key Files for Agents

| File | Why it matters |
|------|---------------|
| `CMakeLists.txt` | Build configuration |
| `src/wizard.cpp/.h` | Entire UI layer and installation orchestration |
| `src/appstate.cpp/.h` | Thread-safe progress + log buffer |
| `src/iso.cpp/.h` | ISO validation, mount, extract |
| `src/disk.cpp/.h` | Partition enumeration |
| `src/bcd.cpp/.h` | BCD store operations |
| `src/boot.cpp/.h` | Boot entry management |
| `src/utils.cpp/.h` | Cross-cutting utilities |
