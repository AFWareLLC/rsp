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
	"github.com/urfave/cli/v2"
	"github.com/jedib0t/go-pretty/v6/table"
	"log"
	"os"
)

func PercentilesForScope(filename string, scope string) {
	log.Printf("Analyzing scope %s, from %s", scope, filename)

	scopeInfos, err := SelectScopes(filename, []string{scope})

	if err != nil {
		log.Fatal(err)
	}

	infos, ok := scopeInfos[scope]
	if !ok || len(infos) == 0 {
		log.Fatalf("No entries found for scope %s", scope)
		return
	}

	log.Printf("Found %d entries for scope %s", len(infos), scope)

	timesMs := ExtractTimesAsMilliseconds(infos)

	p50, p95, p99 := ComputePercentiles(timesMs)

	t := table.NewWriter()
	t.SetOutputMirror(os.Stdout)
	t.AppendHeader(table.Row{"p50", "p95", "p99"})
	t.AppendRow(table.Row{p50, p95, p99})
	t.Render()
}

var PercentilesOnlyCommand = &cli.Command{
	Name:      "percentiles",
	Usage:     "Print p50, p95 and p99 for a given scope",
	ArgsUsage: "<filename> <scope>",
	Action: func(c *cli.Context) error {
		if c.Args().Len() < 2 {
			return fmt.Errorf("missing filename\nUsage: rsp percentiles <filename> <scope>")
		}

		filename := c.Args().Get(0)
		scope := c.Args().Get(1)

		PercentilesForScope(filename, scope)

		return nil
	},
}
