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
	"encoding/binary"
	"io"
	"os"

	"github.com/AFWareLLC/rsp/RSP"
)

func BatchReadCapture(filename string) ([]*RSP.ScopeInfo, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	var infos []*RSP.ScopeInfo

	for {
		// Read the 4-byte length
		var length uint32
		if err := binary.Read(f, binary.LittleEndian, &length); err != nil {
			if err == io.EOF {
				break
			}
			return nil, err
		}

		// Read the FlatBuffer data
		buf := make([]byte, length)
		_, err := io.ReadFull(f, buf)
		if err != nil {
			return nil, err
		}

		// Get a ScopeInfo from the buffer
		scope := RSP.GetRootAsScopeInfo(buf, 0)
		infos = append(infos, scope)
	}

	return infos, nil
}

// ScopeInfoStream provides a streaming iterator over ScopeInfo entries in a file
type ScopeInfoStream struct {
	f *os.File
}

// NewScopeInfoStream opens the file and prepares the stream
func NewScopeInfoStream(filename string) (*ScopeInfoStream, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	return &ScopeInfoStream{f: f}, nil
}

// Close closes the underlying file
func (s *ScopeInfoStream) Close() error {
	return s.f.Close()
}

// Next reads the next ScopeInfo from the stream. Returns io.EOF when done.
func (s *ScopeInfoStream) Next() (*RSP.ScopeInfo, error) {
	var length uint32
	if err := binary.Read(s.f, binary.LittleEndian, &length); err != nil {
		return nil, err
	}

	buf := make([]byte, length)
	_, err := io.ReadFull(s.f, buf)
	if err != nil {
		return nil, err
	}

	scope := RSP.GetRootAsScopeInfo(buf, 0)
	return scope, nil
}
