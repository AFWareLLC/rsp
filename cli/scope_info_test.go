package main

import (
	"testing"

	"github.com/AFWareLLC/rsp/RSP"
	flatbuffers "github.com/google/flatbuffers/go"
)

func buildScopeInfoFB(tag string, ticksStart, ticksEnd, freqHz, maxBufSize uint64, maxOffset byte, metadata []struct {
	tag   string
	typ   RSP.MetadataType
	value uint64
}) []byte {
	builder := flatbuffers.NewBuilder(256)

	// Build metadata entries
	var metadataOffsets []flatbuffers.UOffsetT
	for _, m := range metadata {
		mTag := builder.CreateString(m.tag)
		RSP.MetadataEntryStart(builder)
		RSP.MetadataEntryAddTag(builder, mTag)
		RSP.MetadataEntryAddType(builder, m.typ)
		RSP.MetadataEntryAddValue(builder, m.value)
		metadataOffsets = append(metadataOffsets, RSP.MetadataEntryEnd(builder))
	}

	var metadataVector flatbuffers.UOffsetT
	if len(metadataOffsets) > 0 {
		RSP.ScopeInfoStartMetadataVector(builder, len(metadataOffsets))
		for i := len(metadataOffsets) - 1; i >= 0; i-- {
			builder.PrependUOffsetT(metadataOffsets[i])
		}
		metadataVector = builder.EndVector(len(metadataOffsets))
	}

	tagOffset := builder.CreateString(tag)

	RSP.ScopeInfoStart(builder)
	RSP.ScopeInfoAddTag(builder, tagOffset)
	RSP.ScopeInfoAddTicksStart(builder, ticksStart)
	RSP.ScopeInfoAddTicksEnd(builder, ticksEnd)
	RSP.ScopeInfoAddMachineNominalFreqHz(builder, freqHz)
	RSP.ScopeInfoAddMaxBufferSize(builder, maxBufSize)
	RSP.ScopeInfoAddMaxOffset(builder, maxOffset)
	if len(metadataOffsets) > 0 {
		RSP.ScopeInfoAddMetadata(builder, metadataVector)
	}
	RSP.FinishScopeInfoBuffer(builder, RSP.ScopeInfoEnd(builder))

	return builder.FinishedBytes()
}

func TestConvertScopeInfoBasic(t *testing.T) {
	buf := buildScopeInfoFB("test_scope", 1000, 2000, 1000000, 64, 3, nil)
	fb := RSP.GetRootAsScopeInfo(buf, 0)

	s := ConvertScopeInfo(fb)

	if s.Tag != "test_scope" {
		t.Errorf("expected tag 'test_scope', got '%s'", s.Tag)
	}
	if s.TicksStart != 1000 {
		t.Errorf("expected TicksStart 1000, got %d", s.TicksStart)
	}
	if s.TicksEnd != 2000 {
		t.Errorf("expected TicksEnd 2000, got %d", s.TicksEnd)
	}
	if s.MachineNominalFreq != 1000000 {
		t.Errorf("expected freq 1000000, got %d", s.MachineNominalFreq)
	}
	if s.MaxBufferSize != 64 {
		t.Errorf("expected MaxBufferSize 64, got %d", s.MaxBufferSize)
	}
	if s.MaxOffset != 3 {
		t.Errorf("expected MaxOffset 3, got %d", s.MaxOffset)
	}

	// 1000 ticks / 1000000 Hz = 0.001 seconds
	expected := 0.001
	if s.ElapsedSeconds != expected {
		t.Errorf("expected ElapsedSeconds %f, got %f", expected, s.ElapsedSeconds)
	}
}

func TestConvertScopeInfoZeroFreq(t *testing.T) {
	buf := buildScopeInfoFB("zero_freq", 100, 200, 0, 0, 0, nil)
	fb := RSP.GetRootAsScopeInfo(buf, 0)

	s := ConvertScopeInfo(fb)

	if s.ElapsedSeconds != 0 {
		t.Errorf("expected ElapsedSeconds 0 with zero freq, got %f", s.ElapsedSeconds)
	}
}

func TestConvertScopeInfoWithMetadata(t *testing.T) {
	metadata := []struct {
		tag   string
		typ   RSP.MetadataType
		value uint64
	}{
		{"count", RSP.MetadataTypeUINT32, 42},
		{"flags", RSP.MetadataTypeUINT8, 0xFF},
	}

	buf := buildScopeInfoFB("with_meta", 0, 1000, 1000000, 0, 0, metadata)
	fb := RSP.GetRootAsScopeInfo(buf, 0)

	s := ConvertScopeInfo(fb)

	if len(s.Metadata) != 2 {
		t.Fatalf("expected 2 metadata entries, got %d", len(s.Metadata))
	}

	if s.Metadata[0].Tag != "count" {
		t.Errorf("expected metadata[0].Tag 'count', got '%s'", s.Metadata[0].Tag)
	}
	if s.Metadata[0].Type != MetadataType(RSP.MetadataTypeUINT32) {
		t.Errorf("expected metadata[0].Type UINT32, got %d", s.Metadata[0].Type)
	}
	if s.Metadata[0].Value != 42 {
		t.Errorf("expected metadata[0].Value 42, got %d", s.Metadata[0].Value)
	}

	if s.Metadata[1].Tag != "flags" {
		t.Errorf("expected metadata[1].Tag 'flags', got '%s'", s.Metadata[1].Tag)
	}
	if s.Metadata[1].Value != 0xFF {
		t.Errorf("expected metadata[1].Value 0xFF, got %d", s.Metadata[1].Value)
	}
}

func TestConvertScopeInfoNoMetadata(t *testing.T) {
	buf := buildScopeInfoFB("no_meta", 0, 0, 1000000, 0, 0, nil)
	fb := RSP.GetRootAsScopeInfo(buf, 0)

	s := ConvertScopeInfo(fb)

	if len(s.Metadata) != 0 {
		t.Errorf("expected 0 metadata entries, got %d", len(s.Metadata))
	}
}

func TestConvertScopeInfoElapsedSecondsCalculation(t *testing.T) {
	tests := []struct {
		name     string
		start    uint64
		end      uint64
		freq     uint64
		expected float64
	}{
		{"one_second", 0, 3000000000, 3000000000, 1.0},
		{"half_second", 0, 1500000000, 3000000000, 0.5},
		{"microsecond", 1000, 4000, 3000000000, 1e-6},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			buf := buildScopeInfoFB("calc", tc.start, tc.end, tc.freq, 0, 0, nil)
			fb := RSP.GetRootAsScopeInfo(buf, 0)
			s := ConvertScopeInfo(fb)

			diff := s.ElapsedSeconds - tc.expected
			if diff < 0 {
				diff = -diff
			}
			if diff > 1e-12 {
				t.Errorf("expected %e, got %e", tc.expected, s.ElapsedSeconds)
			}
		})
	}
}
