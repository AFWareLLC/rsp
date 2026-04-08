package main

import (
	"math"
	"testing"
)

func TestExtractTimes(t *testing.T) {
	scopes := []ScopeInfo{
		{ElapsedSeconds: 0.001},
		{ElapsedSeconds: 0.002},
		{ElapsedSeconds: 0.003},
	}

	times := ExtractTimes(scopes)

	if len(times) != 3 {
		t.Fatalf("expected 3 times, got %d", len(times))
	}
	if times[0] != 0.001 || times[1] != 0.002 || times[2] != 0.003 {
		t.Errorf("unexpected times: %v", times)
	}
}

func TestExtractTimesEmpty(t *testing.T) {
	times := ExtractTimes(nil)
	if len(times) != 0 {
		t.Fatalf("expected 0 times, got %d", len(times))
	}
}

func TestExtractTimesAsMilliseconds(t *testing.T) {
	scopes := []ScopeInfo{
		{ElapsedSeconds: 0.001},
		{ElapsedSeconds: 0.5},
		{ElapsedSeconds: 1.0},
	}

	times := ExtractTimesAsMilliseconds(scopes)

	if len(times) != 3 {
		t.Fatalf("expected 3 times, got %d", len(times))
	}
	if times[0] != 1.0 || times[1] != 500.0 || times[2] != 1000.0 {
		t.Errorf("unexpected times: %v", times)
	}
}

func TestExtractTimesAsNanoseconds(t *testing.T) {
	scopes := []ScopeInfo{
		{ElapsedSeconds: 0.000000001},
		{ElapsedSeconds: 0.000001},
		{ElapsedSeconds: 1.0},
	}

	times := ExtractTimesAsNanoseconds(scopes)

	if len(times) != 3 {
		t.Fatalf("expected 3 times, got %d", len(times))
	}
	if times[0] != 1.0 {
		t.Errorf("expected 1.0 ns, got %f", times[0])
	}
	if times[1] != 1000.0 {
		t.Errorf("expected 1000.0 ns, got %f", times[1])
	}
	if times[2] != 1e9 {
		t.Errorf("expected 1e9 ns, got %f", times[2])
	}
}

func TestComputePercentilesEmpty(t *testing.T) {
	p50, p95, p99 := ComputePercentiles(nil)
	if p50 != 0 || p95 != 0 || p99 != 0 {
		t.Errorf("expected all zeros for empty input, got p50=%f p95=%f p99=%f", p50, p95, p99)
	}
}

func TestComputePercentilesSingleValue(t *testing.T) {
	p50, p95, p99 := ComputePercentiles([]float64{42.0})
	if p50 != 42.0 || p95 != 42.0 || p99 != 42.0 {
		t.Errorf("expected all 42.0 for single value, got p50=%f p95=%f p99=%f", p50, p95, p99)
	}
}

func TestComputePercentilesOrdering(t *testing.T) {
	// Generate 100 values: 1.0, 2.0, ..., 100.0
	values := make([]float64, 100)
	for i := range values {
		values[i] = float64(i + 1)
	}

	p50, p95, p99 := ComputePercentiles(values)

	if p50 != 50.0 {
		t.Errorf("expected p50=50.0, got %f", p50)
	}
	if p95 != 95.0 {
		t.Errorf("expected p95=95.0, got %f", p95)
	}
	if p99 != 99.0 {
		t.Errorf("expected p99=99.0, got %f", p99)
	}
}

func TestComputePercentilesUnsortedInput(t *testing.T) {
	// Pass values in reverse order to verify sorting
	values := []float64{100, 99, 98, 97, 96, 95, 94, 93, 92, 91,
		90, 89, 88, 87, 86, 85, 84, 83, 82, 81,
		80, 79, 78, 77, 76, 75, 74, 73, 72, 71,
		70, 69, 68, 67, 66, 65, 64, 63, 62, 61,
		60, 59, 58, 57, 56, 55, 54, 53, 52, 51,
		50, 49, 48, 47, 46, 45, 44, 43, 42, 41,
		40, 39, 38, 37, 36, 35, 34, 33, 32, 31,
		30, 29, 28, 27, 26, 25, 24, 23, 22, 21,
		20, 19, 18, 17, 16, 15, 14, 13, 12, 11,
		10, 9, 8, 7, 6, 5, 4, 3, 2, 1}

	p50, p95, p99 := ComputePercentiles(values)

	if p50 != 50.0 {
		t.Errorf("expected p50=50.0, got %f", p50)
	}
	if p95 != 95.0 {
		t.Errorf("expected p95=95.0, got %f", p95)
	}
	if p99 != 99.0 {
		t.Errorf("expected p99=99.0, got %f", p99)
	}
}

func TestComputePercentilesDoesNotMutateInput(t *testing.T) {
	values := []float64{3.0, 1.0, 2.0}
	original := make([]float64, len(values))
	copy(original, values)

	ComputePercentiles(values)

	for i := range values {
		if values[i] != original[i] {
			t.Errorf("input was mutated at index %d: expected %f, got %f", i, original[i], values[i])
		}
	}
}

func TestComputePercentilesMonotonic(t *testing.T) {
	values := make([]float64, 1000)
	for i := range values {
		values[i] = float64(i) * 0.1
	}

	p50, p95, p99 := ComputePercentiles(values)

	if p50 > p95 {
		t.Errorf("p50 (%f) should be <= p95 (%f)", p50, p95)
	}
	if p95 > p99 {
		t.Errorf("p95 (%f) should be <= p99 (%f)", p95, p99)
	}
	if math.IsNaN(p50) || math.IsNaN(p95) || math.IsNaN(p99) {
		t.Error("percentiles should not be NaN")
	}
}
