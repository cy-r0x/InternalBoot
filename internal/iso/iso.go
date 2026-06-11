//go:build windows

package iso

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

// Info holds metadata about a Windows ISO
type Info struct {
	Path       string `json:"path"`
	SizeBytes  uint64 `json:"sizeBytes"`
	SizeGB     string `json:"sizeGB"`
	IsValid    bool   `json:"isValid"`
	Version    string `json:"version"`
	MountPoint string `json:"mountPoint"`
}

// Validate checks if the given path is a valid Windows ISO
func Validate(path string) (*Info, error) {
	stat, err := os.Stat(path)
	if err != nil {
		return nil, fmt.Errorf("cannot access ISO: %w", err)
	}

	if strings.ToLower(filepath.Ext(path)) != ".iso" {
		return nil, fmt.Errorf("file is not an ISO")
	}

	// Minimum Windows ISO size check (~3GB)
	if stat.Size() < 3*1024*1024*1024 {
		return nil, fmt.Errorf("ISO too small to be a valid Windows installer")
	}

	info := &Info{
		Path:      path,
		SizeBytes: uint64(stat.Size()),
		SizeGB:    fmt.Sprintf("%.2f GB", float64(stat.Size())/(1024*1024*1024)),
		IsValid:   true,
	}

	return info, nil
}

// Mount mounts the ISO using Windows Explorer (native ISO mount)
func Mount(path string) (string, error) {
	// Windows 8+ has native ISO mounting via PowerShell
	cmd := exec.Command("powershell", "-Command",
		fmt.Sprintf("(Mount-DiskImage -ImagePath '%s' -PassThru | Get-Volume).DriveLetter", path))
	out, err := cmd.Output()
	if err != nil {
		return "", fmt.Errorf("failed to mount ISO: %w", err)
	}

	driveLetter := strings.TrimSpace(string(out))
	if driveLetter == "" {
		return "", fmt.Errorf("ISO mounted but drive letter not found")
	}

	return driveLetter + ":\\", nil
}

// Unmount unmounts the ISO
func Unmount(path string) error {
	cmd := exec.Command("powershell", "-Command",
		fmt.Sprintf("Dismount-DiskImage -ImagePath '%s'", path))
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("failed to unmount ISO: %w", err)
	}
	return nil
}

// Extract copies ISO contents to destination using robocopy
func Extract(mountPoint string, dest string, progressCallback func(percent int)) error {
	if err := os.MkdirAll(dest, 0755); err != nil {
		return fmt.Errorf("failed to create destination: %w", err)
	}

	// Robocopy is the most reliable way on Windows for large file copies
	cmd := exec.Command("robocopy", mountPoint, dest, "/E", "/NFL", "/NDL", "/NJH", "/NJS", "/nc", "/ns", "/np")
	
	// For now, run synchronously. In future we can parse output for progress.
	if err := cmd.Run(); err != nil {
		// Robocopy exit codes 0-7 are success
		if exitErr, ok := err.(*exec.ExitError); ok {
			code := exitErr.ExitCode()
			if code >= 0 && code <= 7 {
				return nil
			}
		}
		return fmt.Errorf("robocopy failed: %w", err)
	}

	return nil
}
