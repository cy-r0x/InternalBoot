# InternalBoot

USB-free Windows installation. Extracts a Windows ISO to an internal drive partition and creates a Windows Boot Manager (BCD) entry so the machine can boot directly into the installer without a USB drive.

---

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Language | C++17 |
| UI Framework | Win32 API |
| Graphics | GDI / GDI+ |
| Build System | CMake (3.20+) |

---

## Build Requirements

- **Windows** (the app is fundamentally Windows-centric)
- **Visual Studio 2022** with "Desktop development with C++" workload, **or**
- **MinGW-w64** with POSIX threads
- **CMake** 3.20 or newer
- **Administrator privileges** at runtime (the app calls `bcdedit`, `Mount-DiskImage`, and `shutdown`)

---

## Build Instructions

### With Visual Studio (recommended)

```powershell
# Open a "Developer PowerShell for VS 2022"
cd <project-root>
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

The executable will be at `build/Release/InternalBoot.exe`.

### With MinGW-w64

```bash
cd <project-root>
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

The executable will be at `build/InternalBoot.exe`.

---

## Usage

1. Run `InternalBoot.exe` **as Administrator**.
2. Select a **target partition** from the dropdown.
3. Click **Browse** and choose a Windows ISO (≥ 3 GB).
4. Optionally change the **boot entry name**.
5. Optionally check **"Set as default boot entry"**.
6. Click **START**.
7. Watch the log pane for real-time progress.
8. When complete, click **Reboot Now** or close the app and reboot manually later.
9. To remove the boot entry, click **Remove Boot Entry**.

---

## Architecture

```
src/
  main.cpp       — Entry point (WinMain)
  wizard.cpp     — Main window, Rufus-style UI layout, controls all screens
  appstate.cpp   — Thread-safe progress + log buffer shared with worker thread
  theme.cpp      — Dark-theme colours, fonts, GDI helpers
  iso.cpp        — ISO validation, PowerShell mount/unmount, robocopy extraction
  disk.cpp       — Partition enumeration & free-space checks via PowerShell
  bcd.cpp        — BCD store backup, restore, list, remove via bcdedit
  boot.cpp       — Boot entry add, set-default, remove, reboot via bcdedit/shutdown
  utils.cpp      — UTF-8/UTF-16 conversion, command runner with live log streaming
```

- The UI runs on the main thread. Installation runs on a background `std::thread`.
- The worker thread posts progress updates and log lines to a mutex-protected `AppState`.
- A `WM_TIMER` (200 ms) polls `AppState` and updates the progress bar and log edit.
- All heavy operations (ISO mount, robocopy, bcdedit) are executed as child processes with live stdout capture so every line appears in the log pane in real time.

---

## Security & Risks

- **Administrator required** — The application shells out to `bcdedit`, `Mount-DiskImage`, `robocopy`, and `shutdown`. It will fail without elevated permissions.
- **BCD mutation** — The app reads, copies, modifies, and deletes Windows Boot Manager entries. It creates a timestamped backup of the BCD store before modification (`%SystemDrive%\InternalBoot\Backups\BCD_backup_<timestamp>`), but there is no automatic restore on failure.
- **System reboot** — The **Reboot Now** button triggers an immediate restart (`shutdown /r /t 0`).
- **ISO trust** — The app validates ISO files by extension and minimum size only; it does **not** verify cryptographic signatures.
- **Data risk** — `robocopy` extracts ISO contents into the selected partition. Ensure the target does not overwrite existing data.

---

## License

MIT — see repository for details.

---

> **Author:** Md Whahidul Islam Payel  
> **Repository:** `github.com/cy-r0x/internal-boot`
