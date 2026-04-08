package main

import (
	"encoding/binary"
	"io"
	"os"
	"path/filepath"
	"testing"
)

// writeCaptureFile creates a temporary file in the given format:
// [4-byte LE length][flatbuffer bytes] repeated for each entry.
func writeCaptureFile(t *testing.T, entries [][]byte) string {
	t.Helper()
	path := filepath.Join(t.TempDir(), "capture.rsp")
	f, err := os.Create(path)
	if err != nil {
		t.Fatal(err)
	}
	defer f.Close()

	for _, entry := range entries {
		length := uint32(len(entry))
		if err := binary.Write(f, binary.LittleEndian, length); err != nil {
			t.Fatal(err)
		}
		if _, err := f.Write(entry); err != nil {
			t.Fatal(err)
		}
	}

	return path
}

func TestBatchReadCaptureEmpty(t *testing.T) {
	path := writeCaptureFile(t, nil)

	infos, err := BatchReadCapture(path)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(infos) != 0 {
		t.Errorf("expected 0 entries, got %d", len(infos))
	}
}

func TestBatchReadCaptureSingle(t *testing.T) {
	buf := buildScopeInfoFB("batch_single", 100, 200, 1000000, 0, 0, nil)
	path := writeCaptureFile(t, [][]byte{buf})

	infos, err := BatchReadCapture(path)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(infos) != 1 {
		t.Fatalf("expected 1 entry, got %d", len(infos))
	}
	if string(infos[0].Tag()) != "batch_single" {
		t.Errorf("expected tag 'batch_single', got '%s'", string(infos[0].Tag()))
	}
}

func TestBatchReadCaptureMultiple(t *testing.T) {
	entries := [][]byte{
		buildScopeInfoFB("scope_a", 0, 100, 1000000, 0, 0, nil),
		buildScopeInfoFB("scope_b", 0, 200, 1000000, 0, 0, nil),
		buildScopeInfoFB("scope_c", 0, 300, 1000000, 0, 0, nil),
	}
	path := writeCaptureFile(t, entries)

	infos, err := BatchReadCapture(path)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(infos) != 3 {
		t.Fatalf("expected 3 entries, got %d", len(infos))
	}

	expected := []string{"scope_a", "scope_b", "scope_c"}
	for i, tag := range expected {
		if string(infos[i].Tag()) != tag {
			t.Errorf("entry %d: expected tag '%s', got '%s'", i, tag, string(infos[i].Tag()))
		}
	}
}

func TestBatchReadCaptureNonexistentFile(t *testing.T) {
	_, err := BatchReadCapture("/tmp/nonexistent_rsp_test_file_12345.rsp")
	if err == nil {
		t.Error("expected error for nonexistent file")
	}
}

func TestScopeInfoStreamEmpty(t *testing.T) {
	path := writeCaptureFile(t, nil)

	stream, err := NewScopeInfoStream(path)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	defer stream.Close()

	_, err = stream.Next()
	if err != io.EOF {
		t.Errorf("expected io.EOF, got %v", err)
	}
}

func TestScopeInfoStreamIteration(t *testing.T) {
	entries := [][]byte{
		buildScopeInfoFB("stream_1", 0, 100, 1000000, 0, 0, nil),
		buildScopeInfoFB("stream_2", 0, 200, 1000000, 0, 0, nil),
	}
	path := writeCaptureFile(t, entries)

	stream, err := NewScopeInfoStream(path)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	defer stream.Close()

	scope1, err := stream.Next()
	if err != nil {
		t.Fatalf("unexpected error on first Next: %v", err)
	}
	if string(scope1.Tag()) != "stream_1" {
		t.Errorf("expected 'stream_1', got '%s'", string(scope1.Tag()))
	}

	scope2, err := stream.Next()
	if err != nil {
		t.Fatalf("unexpected error on second Next: %v", err)
	}
	if string(scope2.Tag()) != "stream_2" {
		t.Errorf("expected 'stream_2', got '%s'", string(scope2.Tag()))
	}

	_, err = stream.Next()
	if err != io.EOF {
		t.Errorf("expected io.EOF after last entry, got %v", err)
	}
}

func TestScopeInfoStreamNonexistentFile(t *testing.T) {
	_, err := NewScopeInfoStream("/tmp/nonexistent_rsp_test_file_12345.rsp")
	if err == nil {
		t.Error("expected error for nonexistent file")
	}
}

func TestBatchReadCaptureFieldValues(t *testing.T) {
	buf := buildScopeInfoFB("fields", 500, 1500, 2000000, 128, 7, nil)
	path := writeCaptureFile(t, [][]byte{buf})

	infos, err := BatchReadCapture(path)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	scope := infos[0]
	if scope.TicksStart() != 500 {
		t.Errorf("expected TicksStart 500, got %d", scope.TicksStart())
	}
	if scope.TicksEnd() != 1500 {
		t.Errorf("expected TicksEnd 1500, got %d", scope.TicksEnd())
	}
	if scope.MachineNominalFreqHz() != 2000000 {
		t.Errorf("expected freq 2000000, got %d", scope.MachineNominalFreqHz())
	}
	if scope.MaxBufferSize() != 128 {
		t.Errorf("expected MaxBufferSize 128, got %d", scope.MaxBufferSize())
	}
	if scope.MaxOffset() != 7 {
		t.Errorf("expected MaxOffset 7, got %d", scope.MaxOffset())
	}
}
