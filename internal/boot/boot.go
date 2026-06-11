//go:build windows

package boot

import (
	"fmt"
	"os/exec"
	"strings"

	"github.com/cy-r0x/internal-boot/internal/bcd"
)

// SetupOptions contains parameters for creating a boot entry
type SetupOptions struct {
	Description string `json:"description"`
	DriveLetter string `json:"driveLetter"`
	BootPath    string `json:"bootPath"` // e.g., \\EFI\\Microsoft\\Boot\\bootmgfw.efi
}

// AddInstallerEntry adds a new BCD entry pointing to the Windows installer
func AddInstallerEntry(opts SetupOptions) (string, error) {
	// Ensure the path starts with \\ for EFI paths
	bootPath := opts.BootPath
	if !strings.HasPrefix(bootPath, "\\") {
		bootPath = "\\" + bootPath
	}

	// Create a new boot entry
	cmd := exec.Command("bcdedit", "/copy", "{current}", "/d", opts.Description)
	out, err := cmd.Output()
	if err != nil {
		return "", fmt.Errorf("bcdedit copy failed: %w", err)
	}

	// Parse the new GUID from output: "The entry was successfully copied to {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}."
	output := string(out)
	start := strings.Index(output, "{")
	end := strings.Index(output, "}")
	if start == -1 || end == -1 {
		return "", fmt.Errorf("failed to parse new BCD entry GUID")
	}
	guid := output[start : end+1]

	// Set device and path for the new entry
	devicePath := fmt.Sprintf("partition=%s:", opts.DriveLetter)

	cmd = exec.Command("bcdedit", "/set", guid, "device", devicePath)
	if out, err := cmd.CombinedOutput(); err != nil {
		return "", fmt.Errorf("bcdedit set device failed: %w\n%s", err, string(out))
	}

	cmd = exec.Command("bcdedit", "/set", guid, "path", bootPath)
	if out, err := cmd.CombinedOutput(); err != nil {
		return "", fmt.Errorf("bcdedit set path failed: %w\n%s", err, string(out))
	}

	// Set osdevice as well (required for Windows boot entries)
	cmd = exec.Command("bcdedit", "/set", guid, "osdevice", devicePath)
	if out, err := cmd.CombinedOutput(); err != nil {
		return "", fmt.Errorf("bcdedit set osdevice failed: %w\n%s", err, string(out))
	}

	// Set boot sequence to include this entry (optional: set as default)
	cmd = exec.Command("bcdedit", "/displayorder", guid, "/addlast")
	if out, err := cmd.CombinedOutput(); err != nil {
		return "", fmt.Errorf("bcdedit displayorder failed: %w\n%s", err, string(out))
	}

	return guid, nil
}

// SetDefault sets the given entry as the default boot option
func SetDefault(identifier string) error {
	cmd := exec.Command("bcdedit", "/default", identifier)
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("bcdedit default failed: %w\n%s", err, string(out))
	}
	return nil
}

// RemoveInstallerEntry removes the InternalBoot installer entry
func RemoveInstallerEntry() error {
	entry, err := bcd.FindEntryByDescription("InternalBoot Installer")
	if err != nil {
		return err
	}
	if entry == nil {
		return nil // Already removed
	}
	return bcd.RemoveEntry(entry.Identifier)
}

// Reboot triggers a system reboot
func Reboot() error {
	cmd := exec.Command("shutdown", "/r", "/t", "0")
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("reboot command failed: %w", err)
	}
	return nil
}
