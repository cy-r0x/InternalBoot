//go:build !windows

package disk

import "fmt"

type Partition struct {
	Index       int    `json:"index"`
	DriveLetter string `json:"driveLetter"`
	Label       string `json:"label"`
	FileSystem  string `json:"fileSystem"`
	SizeBytes   uint64 `json:"sizeBytes"`
	SizeGB      string `json:"sizeGB"`
	IsBootable  bool   `json:"isBootable"`
	Type        string `json:"type"`
}

type DiskInfo struct {
	Index      int         `json:"index"`
	Model      string      `json:"model"`
	SizeBytes  uint64      `json:"sizeBytes"`
	SizeGB     string      `json:"sizeGB"`
	Type       string      `json:"type"`
	Partitions []Partition `json:"partitions"`
}

func GetDisks() ([]DiskInfo, error) {
	return nil, fmt.Errorf("disk operations only supported on Windows")
}

func GetPartitions() ([]Partition, error) {
	return nil, fmt.Errorf("disk operations only supported on Windows")
}

func ValidateSpace(driveLetter string, requiredBytes uint64) error {
	return fmt.Errorf("disk operations only supported on Windows")
}

func FormatSize(bytes uint64) string {
	return "N/A"
}
