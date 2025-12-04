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
	"fmt"
	"io"
)

func SelectScopes(filename string, scopeTags []string) (map[string][]ScopeInfo, error) {
	wanted := make(map[string]struct{}, len(scopeTags))
	for _, t := range scopeTags {
		wanted[t] = struct{}{}
	}

	result := make(map[string][]ScopeInfo)

	stream, err := NewScopeInfoStream(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to open scope stream: %w", err)
	}
	defer stream.Close()

	for {
		fbScope, err := stream.Next()
		if err != nil {
			if err == io.EOF {
				break
			}
			return nil, fmt.Errorf("failed reading scope: %w", err)
		}

		s := ConvertScopeInfo(fbScope)

		// Only select matching tags
		if _, ok := wanted[s.Tag]; ok {
			result[s.Tag] = append(result[s.Tag], s)
		}
	}

	return result, nil
}

func CountByScope(filename string) (map[string]int, error) {
	counts := make(map[string]int)

	stream, err := NewScopeInfoStream(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to open scope stream: %w", err)
	}

	defer stream.Close()

	for {
		fbScope, err := stream.Next()
		if err != nil {
			if err == io.EOF {
				break
			}
			return nil, fmt.Errorf("failed reading scope entry: %w", err)
		}

		s := ConvertScopeInfo(fbScope)
		counts[s.Tag]++
	}

	return counts, nil
}
