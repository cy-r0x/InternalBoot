//go:build windows

package disk

import (
	"encoding/json"
	"fmt"
	"os/exec"
	"strconv"
	"strings"
)

// Partition represents a disk partition
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

// DiskInfo represents a physical disk
type DiskInfo struct {
	Index      int         `json:"index"`
	Model      string      `json:"model"`
	SizeBytes  uint64      `json:"sizeBytes"`
	SizeGB     string      `json:"sizeGB"`
	Type       string      `json:"type"`
	Partitions []Partition `json:"partitions"`
}

// GetDisks retrieves disk and partition information using PowerShell
func GetDisks() ([]DiskInfo, error) {
	cmd := exec.Command("powershell", "-Command",
		"Get-Disk | Select-Object Number,Model,Size,PartitionStyle | ConvertTo-Json -Compress")
	out, err := cmd.Output()
	if err != nil {
		return nil, fmt.Errorf("failed to get disks: %w", err)
	}

	raw := strings.TrimSpace(string(out))
	if raw == "" {
		return []DiskInfo{}, nil
	}

	items, err := unmarshalPSJSON(raw)
	if err != nil {
		return nil, fmt.Errorf("failed to parse disk JSON: %w", err)
	}

	var disks []DiskInfo
	for i, item := range items {
		d := DiskInfo{Index: i}
		if v, ok := item["Number"].(json.Number); ok {
			if n, err := v.Int64(); err == nil {
				d.Index = int(n)
			}
		}
		if v, ok := item["Model"].(string); ok {
			d.Model = v
		}
		if v, ok := item["PartitionStyle"].(string); ok {
			d.Type = v
		}
		if v, ok := item["Size"].(json.Number); ok {
			if n, err := v.Int64(); err == nil {
				d.SizeBytes = uint64(n)
				d.SizeGB = FormatSize(d.SizeBytes)
			}
		}
		disks = append(disks, d)
	}

	return disks, nil
}

// GetPartitions retrieves all partitions with drive letters
func GetPartitions() ([]Partition, error) {
	cmd := exec.Command("powershell", "-Command",
		"Get-Volume | Where-Object {$_.DriveLetter} | Select-Object DriveLetter,FileSystemLabel,FileSystem,Size,SizeRemaining | ConvertTo-Json -Compress")
	out, err := cmd.Output()
	if err != nil {
		return nil, fmt.Errorf("failed to get partitions: %w", err)
	}

	raw := strings.TrimSpace(string(out))
	if raw == "" {
		return []Partition{}, nil
	}

	items, err := unmarshalPSJSON(raw)
	if err != nil {
		return nil, fmt.Errorf("failed to parse partition JSON: %w", err)
	}

	var partitions []Partition
	for i, item := range items {
		p := Partition{Index: i}
		if v, ok := item["DriveLetter"].(string); ok {
			p.DriveLetter = v
		}
		if v, ok := item["FileSystemLabel"].(string); ok {
			p.Label = v
		}
		if v, ok := item["FileSystem"].(string); ok {
			p.FileSystem = v
		}
		if v, ok := item["Size"].(json.Number); ok {
			if n, err := v.Int64(); err == nil {
				p.SizeBytes = uint64(n)
				p.SizeGB = FormatSize(p.SizeBytes)
			}
		}
		partitions = append(partitions, p)
	}

	return partitions, nil
}

// unmarshalPSJSON handles PowerShell's ConvertTo-Json output which may be a single object or an array
func unmarshalPSJSON(raw string) ([]map[string]interface{}, error) {
	decoder := json.NewDecoder(strings.NewReader(raw))
	decoder.UseNumber()

	// Try array first
	var arr []map[string]interface{}
	if err := decoder.Decode(&arr); err == nil {
		return arr, nil
	}

	// Try single object
	decoder = json.NewDecoder(strings.NewReader(raw))
	decoder.UseNumber()
	var obj map[string]interface{}
	if err := decoder.Decode(&obj); err != nil {
		return nil, err
	}
	return []map[string]interface{}{obj}, nil
}

// ValidateSpace checks if a partition has at least requiredBytes free
func ValidateSpace(driveLetter string, requiredBytes uint64) error {
	cmd := exec.Command("powershell", "-Command",
		fmt.Sprintf("(Get-Volume -DriveLetter %s).SizeRemaining", driveLetter))
	out, err := cmd.Output()
	if err != nil {
		return fmt.Errorf("failed to get free space: %w", err)
	}

	remainingStr := strings.TrimSpace(string(out))
	remaining, err := strconv.ParseUint(remainingStr, 10, 64)
	if err != nil {
		return fmt.Errorf("failed to parse free space: %w", err)
	}

	if remaining < requiredBytes {
		return fmt.Errorf("insufficient space: %d bytes required, %d available", requiredBytes, remaining)
	}

	return nil
}

// FormatSize formats bytes to human-readable GB
func FormatSize(bytes uint64) string {
	gb := float64(bytes) / (1024 * 1024 * 1024)
	return fmt.Sprintf("%.2f GB", gb)
}
