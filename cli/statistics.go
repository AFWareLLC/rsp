// Copyright © 2025, AFWare LLC <ajf@afware.io>
//
// Permission to use, copy, modify, and/or distribute this software
// for any purpose with or without fee is hereby granted, provided
// that the above copyright notice and this permission notice appear
// in all copies.
//
// THE SOFTWARE IS PROVIDED “AS IS” AND ISC DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
// DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
// ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.

package main

import (
	"gonum.org/v1/gonum/stat"
	"sort"
)

func ExtractTimes(scopes []ScopeInfo) []float64 {
	times := make([]float64, len(scopes))
	for i, s := range scopes {
		times[i] = s.ElapsedSeconds
	}
	return times
}

func ExtractTimesAsMilliseconds(scopes []ScopeInfo) []float64 {
	times := make([]float64, len(scopes))
	for i, s := range scopes {
		times[i] = s.ElapsedSeconds * 1000
	}
	return times
}

func ExtractTimesAsNanoseconds(scopes []ScopeInfo) []float64 {
	times := make([]float64, len(scopes))
	for i, s := range scopes {
		times[i] = s.ElapsedSeconds * 1e9
	}
	return times
}

func ComputePercentiles(values []float64) (p50, p95, p99 float64) {
	if len(values) == 0 {
		return 0, 0, 0
	}

	sorted := make([]float64, len(values))
	copy(sorted, values)
	sort.Float64s(sorted)

	p50 = stat.Quantile(0.50, stat.Empirical, sorted, nil)
	p95 = stat.Quantile(0.95, stat.Empirical, sorted, nil)
	p99 = stat.Quantile(0.99, stat.Empirical, sorted, nil)

	return p50, p95, p99
}
