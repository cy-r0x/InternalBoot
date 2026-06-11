//go:build !windows

package boot

import "fmt"

type SetupOptions struct {
	Description string `json:"description"`
	DriveLetter string `json:"driveLetter"`
	BootPath    string `json:"bootPath"`
}

func AddInstallerEntry(opts SetupOptions) (string, error) {
	return "", fmt.Errorf("boot operations only supported on Windows")
}

func SetDefault(identifier string) error {
	return fmt.Errorf("boot operations only supported on Windows")
}

func RemoveInstallerEntry() error {
	return fmt.Errorf("boot operations only supported on Windows")
}

func Reboot() error {
	return fmt.Errorf("boot operations only supported on Windows")
}
