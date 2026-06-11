//go:build !windows

package bcd

import "fmt"

type Entry struct {
	Identifier  string `json:"identifier"`
	Description string `json:"description"`
	Device      string `json:"device"`
	Path        string `json:"path"`
}

func Backup() (string, error) {
	return "", fmt.Errorf("BCD operations only supported on Windows")
}

func Restore(backupPath string) error {
	return fmt.Errorf("BCD operations only supported on Windows")
}

func ListEntries() ([]Entry, error) {
	return nil, fmt.Errorf("BCD operations only supported on Windows")
}

func RemoveEntry(identifier string) error {
	return fmt.Errorf("BCD operations only supported on Windows")
}

func FindEntryByDescription(description string) (*Entry, error) {
	return nil, fmt.Errorf("BCD operations only supported on Windows")
}
