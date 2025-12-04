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
	"github.com/AFWareLLC/rsp/RSP"
	"github.com/urfave/cli/v2"
	"io"
	"log"
)

func Echo(filename string) {
	log.Printf("Echoing from file %s", filename)

	stream, err := NewScopeInfoStream(filename)

	if err != nil {
		log.Fatal(err)
	}

	defer stream.Close()
	i := 0
	for {
		scope, err := stream.Next()
		if err != nil {
			if err == io.EOF {
				break
			}
			log.Fatal(err)
		}

		log.Printf("-------")
		log.Printf("  Tag: %s", string(scope.Tag()))
		log.Printf("  Ticks: %d - %d", scope.TicksStart(), scope.TicksEnd())
		log.Printf("  Machine Freq: %d", scope.MachineNominalFreqHz())
		log.Printf("  MaxOffset: %d", scope.MaxOffset())

		for j := 0; j < scope.MetadataLength(); j++ {
			m := new(RSP.MetadataEntry)
			if scope.Metadata(m, j) {
				log.Printf("    Metadata #%d: %s Type=%d Value=%d",
					j, string(m.Tag()), m.Type(), m.Value())
			}
		}
		i++
	}
}

var EchoCommand = &cli.Command{
	Name:      "echo",
	Usage:     "Dump out the profiling data to stdout. Not very useful, but sometimes handy for debugging or quick inspection.",
	ArgsUsage: "<filename>",
	Action: func(c *cli.Context) error {
		if c.Args().Len() < 1 {
			return fmt.Errorf("missing filename\nUsage: rsp echo <filename>")
		}

		filename := c.Args().Get(0)

		// Call your example func here
		Echo(filename)

		return nil
	},
}
