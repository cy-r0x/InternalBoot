//go:build windows

package bcd

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"
)

const bcdStore = "C:\\Boot\\BCD"

// Backup creates a timestamped backup of the BCD store
func Backup() (string, error) {
	backupDir := filepath.Join(os.Getenv("SystemDrive")+"\\", "InternalBoot", "Backups")
	if err := os.MkdirAll(backupDir, 0755); err != nil {
		return "", fmt.Errorf("failed to create backup dir: %w", err)
	}

	timestamp := time.Now().Format("20060102_150405")
	backupPath := filepath.Join(backupDir, fmt.Sprintf("BCD_backup_%s", timestamp))

	cmd := exec.Command("bcdedit", "/export", backupPath)
	if out, err := cmd.CombinedOutput(); err != nil {
		return "", fmt.Errorf("bcdedit backup failed: %w\n%s", err, string(out))
	}

	return backupPath, nil
}

// Restore restores the BCD from a backup file
func Restore(backupPath string) error {
	cmd := exec.Command("bcdedit", "/import", backupPath, "/clean")
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("bcdedit restore failed: %w\n%s", err, string(out))
	}
	return nil
}

// Entry represents a BCD boot entry
type Entry struct {
	Identifier string `json:"identifier"`
	Description string `json:"description"`
	Device     string `json:"device"`
	Path       string `json:"path"`
}

// ListEntries returns all BCD entries
func ListEntries() ([]Entry, error) {
	cmd := exec.Command("bcdedit", "/enum", "firmware")
	out, err := cmd.Output()
	if err != nil {
		return nil, fmt.Errorf("bcdedit enum failed: %w", err)
	}

	var entries []Entry
	lines := strings.Split(string(out), "\n")
	var current Entry
	for _, line := range lines {
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, "identifier") {
			if current.Identifier != "" {
				entries = append(entries, current)
			}
			current = Entry{Identifier: strings.TrimSpace(strings.TrimPrefix(line, "identifier"))}
		} else if strings.HasPrefix(line, "description") {
			current.Description = strings.TrimSpace(strings.TrimPrefix(line, "description"))
		} else if strings.HasPrefix(line, "device") {
			current.Device = strings.TrimSpace(strings.TrimPrefix(line, "device"))
		} else if strings.HasPrefix(line, "path") {
			current.Path = strings.TrimSpace(strings.TrimPrefix(line, "path"))
		}
	}
	if current.Identifier != "" {
		entries = append(entries, current)
	}

	return entries, nil
}

// RemoveEntry removes a BCD entry by identifier
func RemoveEntry(identifier string) error {
	cmd := exec.Command("bcdedit", "/delete", identifier)
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("bcdedit delete failed: %w\n%s", err, string(out))
	}
	return nil
}

// FindEntryByDescription searches for an entry matching the description
func FindEntryByDescription(description string) (*Entry, error) {
	entries, err := ListEntries()
	if err != nil {
		return nil, err
	}

	for _, e := range entries {
		if strings.EqualFold(e.Description, description) {
			return &e, nil
		}
	}
	return nil, nil
}
