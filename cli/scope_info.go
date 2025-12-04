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
	"github.com/AFWareLLC/rsp/RSP"
)

type MetadataType byte

type MetadataEntry struct {
	Tag   string
	Type  MetadataType
	Value uint64
}

type ScopeInfo struct {
	Tag                string
	TicksStart         uint64
	TicksEnd           uint64
	MachineNominalFreq uint64
	MaxBufferSize      uint64
	MaxOffset          byte
	Metadata           []MetadataEntry

	ElapsedSeconds float64
}

func ConvertScopeInfo(fb *RSP.ScopeInfo) ScopeInfo {
	s := ScopeInfo{
		Tag:                string(fb.Tag()),
		TicksStart:         fb.TicksStart(),
		TicksEnd:           fb.TicksEnd(),
		MachineNominalFreq: fb.MachineNominalFreqHz(),
		MaxBufferSize:      fb.MaxBufferSize(),
		MaxOffset:          fb.MaxOffset(),
	}

	if s.MachineNominalFreq > 0 {
		s.ElapsedSeconds = float64(s.TicksEnd-s.TicksStart) / float64(s.MachineNominalFreq)
	}

	metadata := make([]MetadataEntry, fb.MetadataLength())
	for i := 0; i < fb.MetadataLength(); i++ {
		m := new(RSP.MetadataEntry)
		if fb.Metadata(m, i) {
			metadata[i] = MetadataEntry{
				Tag:   string(m.Tag()),
				Type:  MetadataType(m.Type()),
				Value: m.Value(),
			}
		}
	}

	s.Metadata = metadata
	return s
}
