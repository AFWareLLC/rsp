package main

import (
	"testing"
)

func TestSelectScopesMatchingTags(t *testing.T) {
	entries := [][]byte{
		buildScopeInfoFB("alpha", 0, 1000, 1000000, 0, 0, nil),
		buildScopeInfoFB("beta", 0, 2000, 1000000, 0, 0, nil),
		buildScopeInfoFB("alpha", 0, 3000, 1000000, 0, 0, nil),
		buildScopeInfoFB("gamma", 0, 4000, 1000000, 0, 0, nil),
	}
	path := writeCaptureFile(t, entries)

	result, err := SelectScopes(path, []string{"alpha", "gamma"})
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if len(result["alpha"]) != 2 {
		t.Errorf("expected 2 alpha entries, got %d", len(result["alpha"]))
	}
	if len(result["gamma"]) != 1 {
		t.Errorf("expected 1 gamma entry, got %d", len(result["gamma"]))
	}
	if _, ok := result["beta"]; ok {
		t.Error("beta should not be in results")
	}
}

func TestSelectScopesNoMatches(t *testing.T) {
	entries := [][]byte{
		buildScopeInfoFB("alpha", 0, 1000, 1000000, 0, 0, nil),
	}
	path := writeCaptureFile(t, entries)

	result, err := SelectScopes(path, []string{"nonexistent"})
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(result) != 0 {
		t.Errorf("expected empty result, got %d entries", len(result))
	}
}

func TestSelectScopesEmptyFile(t *testing.T) {
	path := writeCaptureFile(t, nil)

	result, err := SelectScopes(path, []string{"anything"})
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(result) != 0 {
		t.Errorf("expected empty result, got %d entries", len(result))
	}
}

func TestSelectScopesNonexistentFile(t *testing.T) {
	_, err := SelectScopes("/tmp/nonexistent_rsp_test_file_12345.rsp", []string{"a"})
	if err == nil {
		t.Error("expected error for nonexistent file")
	}
}

func TestSelectScopesElapsedSeconds(t *testing.T) {
	entries := [][]byte{
		buildScopeInfoFB("scope", 0, 1000, 1000000, 0, 0, nil),
	}
	path := writeCaptureFile(t, entries)

	result, err := SelectScopes(path, []string{"scope"})
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if len(result["scope"]) != 1 {
		t.Fatalf("expected 1 entry, got %d", len(result["scope"]))
	}
	// 1000 ticks / 1000000 Hz = 0.001 seconds
	if result["scope"][0].ElapsedSeconds != 0.001 {
		t.Errorf("expected 0.001 ElapsedSeconds, got %f", result["scope"][0].ElapsedSeconds)
	}
}

func TestCountByScopeBasic(t *testing.T) {
	entries := [][]byte{
		buildScopeInfoFB("alpha", 0, 100, 1000000, 0, 0, nil),
		buildScopeInfoFB("beta", 0, 200, 1000000, 0, 0, nil),
		buildScopeInfoFB("alpha", 0, 300, 1000000, 0, 0, nil),
		buildScopeInfoFB("alpha", 0, 400, 1000000, 0, 0, nil),
		buildScopeInfoFB("beta", 0, 500, 1000000, 0, 0, nil),
	}
	path := writeCaptureFile(t, entries)

	counts, err := CountByScope(path)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if counts["alpha"] != 3 {
		t.Errorf("expected alpha=3, got %d", counts["alpha"])
	}
	if counts["beta"] != 2 {
		t.Errorf("expected beta=2, got %d", counts["beta"])
	}
}

func TestCountByScopeEmpty(t *testing.T) {
	path := writeCaptureFile(t, nil)

	counts, err := CountByScope(path)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(counts) != 0 {
		t.Errorf("expected empty counts, got %d entries", len(counts))
	}
}

func TestCountByScopeNonexistentFile(t *testing.T) {
	_, err := CountByScope("/tmp/nonexistent_rsp_test_file_12345.rsp")
	if err == nil {
		t.Error("expected error for nonexistent file")
	}
}
