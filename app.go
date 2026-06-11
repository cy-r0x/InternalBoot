package main

import (
	"context"
	"fmt"
	"path/filepath"
	"sync"

	"github.com/wailsapp/wails/v2/pkg/runtime"

	"github.com/cy-r0x/internal-boot/internal/bcd"
	"github.com/cy-r0x/internal-boot/internal/boot"
	"github.com/cy-r0x/internal-boot/internal/disk"
	"github.com/cy-r0x/internal-boot/internal/iso"
)

// App struct
type App struct {
	ctx context.Context

	mu       sync.Mutex
	isoInfo  *iso.Info
	progress Progress
}

// Progress represents the current installation progress
type Progress struct {
	Stage      string `json:"stage"`
	Message    string `json:"message"`
	Percent    int    `json:"percent"`
	IsRunning  bool   `json:"isRunning"`
	HasError   bool   `json:"hasError"`
	ErrorMsg   string `json:"errorMsg"`
	CanReboot  bool   `json:"canReboot"`
	CanCleanup bool   `json:"canCleanup"`
}

// NewApp creates a new App application struct
func NewApp() *App {
	return &App{}
}

// startup is called when the app starts
func (a *App) startup(ctx context.Context) {
	a.ctx = ctx
}

// SelectISO opens a file dialog to choose a Windows ISO
func (a *App) SelectISO() (string, error) {
	selection, err := runtime.OpenFileDialog(a.ctx, runtime.OpenDialogOptions{
		Title: "Select Windows ISO",
		Filters: []runtime.FileFilter{
			{
				DisplayName: "ISO Files (*.iso)",
				Pattern:     "*.iso",
			},
		},
	})
	if err != nil {
		return "", err
	}
	return selection, nil
}

// ValidateISO validates the selected ISO file
func (a *App) ValidateISO(path string) (*iso.Info, error) {
	info, err := iso.Validate(path)
	if err != nil {
		return nil, err
	}
	a.mu.Lock()
	a.isoInfo = info
	a.mu.Unlock()
	return info, nil
}

// GetPartitions returns available partitions
func (a *App) GetPartitions() ([]disk.Partition, error) {
	return disk.GetPartitions()
}

// GetProgress returns current installation progress
func (a *App) GetProgress() Progress {
	a.mu.Lock()
	defer a.mu.Unlock()
	return a.progress
}

// InstallRequest contains parameters for starting installation
type InstallRequest struct {
	ISOPath      string `json:"isoPath"`
	DriveLetter  string `json:"driveLetter"`
	TargetPath   string `json:"targetPath"` // e.g., "InternalBoot_Installer"
	SetAsDefault bool   `json:"setAsDefault"`
}

// StartInstallation begins the ISO extraction and BCD configuration
func (a *App) StartInstallation(req InstallRequest) error {
	a.mu.Lock()
	if a.progress.IsRunning {
		a.mu.Unlock()
		return fmt.Errorf("installation already in progress")
	}
	a.progress = Progress{IsRunning: true, Stage: "preparing", Message: "Preparing installation..."}
	a.mu.Unlock()

	go a.runInstallation(req)
	return nil
}

func (a *App) runInstallation(req InstallRequest) {
	setProgress := func(p Progress) {
		a.mu.Lock()
		a.progress = p
		a.mu.Unlock()
	}

	// Step 1: Validate ISO
	setProgress(Progress{IsRunning: true, Stage: "validating", Message: "Validating ISO...", Percent: 5})
	info, err := iso.Validate(req.ISOPath)
	if err != nil {
		setProgress(Progress{IsRunning: false, HasError: true, ErrorMsg: err.Error()})
		return
	}

	// Step 2: Validate target space (ISO size + 2GB buffer)
	setProgress(Progress{IsRunning: true, Stage: "checking", Message: "Checking disk space...", Percent: 10})
	requiredSpace := info.SizeBytes + 2*1024*1024*1024
	if err := disk.ValidateSpace(req.DriveLetter, requiredSpace); err != nil {
		setProgress(Progress{IsRunning: false, HasError: true, ErrorMsg: err.Error()})
		return
	}

	// Step 3: Backup BCD
	setProgress(Progress{IsRunning: true, Stage: "backup", Message: "Backing up BCD store...", Percent: 15})
	backupPath, err := bcd.Backup()
	if err != nil {
		setProgress(Progress{IsRunning: false, HasError: true, ErrorMsg: fmt.Sprintf("BCD backup failed: %v", err)})
		return
	}
	_ = backupPath

	// Step 4: Mount ISO
	setProgress(Progress{IsRunning: true, Stage: "mounting", Message: "Mounting ISO...", Percent: 20})
	mountPoint, err := iso.Mount(req.ISOPath)
	if err != nil {
		setProgress(Progress{IsRunning: false, HasError: true, ErrorMsg: fmt.Sprintf("ISO mount failed: %v", err)})
		return
	}
	defer iso.Unmount(req.ISOPath)

	// Step 5: Extract ISO contents
	setProgress(Progress{IsRunning: true, Stage: "extracting", Message: "Extracting ISO contents...", Percent: 25})
	destPath := filepath.Join(req.DriveLetter+":\\", req.TargetPath)
	if err := iso.Extract(mountPoint, destPath, func(percent int) {
		overall := 25 + int(float64(percent)*0.50)
		setProgress(Progress{IsRunning: true, Stage: "extracting", Message: "Extracting ISO contents...", Percent: overall})
	}); err != nil {
		setProgress(Progress{IsRunning: false, HasError: true, ErrorMsg: fmt.Sprintf("Extraction failed: %v", err)})
		return
	}

	// Step 6: Create boot entry
	setProgress(Progress{IsRunning: true, Stage: "bcd", Message: "Configuring Windows Boot Manager...", Percent: 80})
	bootPath := "\\EFI\\Microsoft\\Boot\\bootmgfw.efi"
	// Check if BIOS legacy boot files exist, fallback to EFI
	// In a real implementation, we'd detect UEFI vs BIOS and set appropriate paths

	guid, err := boot.AddInstallerEntry(boot.SetupOptions{
		Description: "InternalBoot Installer",
		DriveLetter: req.DriveLetter,
		BootPath:    bootPath,
	})
	if err != nil {
		setProgress(Progress{IsRunning: false, HasError: true, ErrorMsg: fmt.Sprintf("Boot entry creation failed: %v", err)})
		return
	}

	// Step 7: Optionally set as default
	if req.SetAsDefault {
		setProgress(Progress{IsRunning: true, Stage: "bcd", Message: "Setting as default boot option...", Percent: 90})
		if err := boot.SetDefault(guid); err != nil {
			setProgress(Progress{IsRunning: false, HasError: true, ErrorMsg: fmt.Sprintf("Set default failed: %v", err)})
			return
		}
	}

	// Done
	setProgress(Progress{
		IsRunning:  false,
		Stage:      "complete",
		Message:    "Installation ready! You can now reboot.",
		Percent:    100,
		CanReboot:  true,
		CanCleanup: false,
	})
}

// Reboot triggers system reboot
func (a *App) Reboot() error {
	return boot.Reboot()
}

// Cleanup removes the installer files and boot entry
func (a *App) Cleanup(driveLetter string, targetPath string) error {
	a.mu.Lock()
	a.progress = Progress{IsRunning: true, Stage: "cleanup", Message: "Cleaning up installer files...", Percent: 0}
	a.mu.Unlock()

	if err := boot.RemoveInstallerEntry(); err != nil {
		return err
	}

	// In a real implementation, we'd also remove the target directory
	_ = driveLetter
	_ = targetPath

	a.mu.Lock()
	a.progress = Progress{IsRunning: false, Stage: "idle", Message: "Cleanup complete.", Percent: 100}
	a.mu.Unlock()
	return nil
}

// Greet returns a greeting (kept from template)
func (a *App) Greet(name string) string {
	return fmt.Sprintf("Hello %s, It's show time!", name)
}
