//go:build !windows

package iso

import "fmt"

type Info struct {
	Path       string `json:"path"`
	SizeBytes  uint64 `json:"sizeBytes"`
	SizeGB     string `json:"sizeGB"`
	IsValid    bool   `json:"isValid"`
	Version    string `json:"version"`
	MountPoint string `json:"mountPoint"`
}

func Validate(path string) (*Info, error) {
	return nil, fmt.Errorf("ISO operations only supported on Windows")
}

func Mount(path string) (string, error) {
	return "", fmt.Errorf("ISO operations only supported on Windows")
}

func Unmount(path string) error {
	return fmt.Errorf("ISO operations only supported on Windows")
}

func Extract(mountPoint string, dest string, progressCallback func(percent int)) error {
	return fmt.Errorf("ISO operations only supported on Windows")
}
