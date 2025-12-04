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
	"log"
)

func TimingsForScope(filename string, scope string, savePath string, bindAddr string) {
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

	timesPlot := CreateTimesPlot(timesMs, scope, fmt.Sprintf("Analysis for scope: %s", scope))
	AddPercentilesToTimePlot(timesPlot, len(timesMs), p50, p95, p99)

	if savePath == "" {
		ServeChartsPage(bindAddr, timesPlot)
	} else {
		SaveChartsPageHTML(savePath, timesPlot)
	}
}

var TimingsCommand = &cli.Command{
	Name:      "timings",
	Usage:     "Plot elapsed times in milliseconds and visualize p50, p90 and p99.",
	ArgsUsage: "<filename> <scope>",
	Flags: []cli.Flag{
		&cli.StringFlag{
			Name:    "output",
			Aliases: []string{"o"},
			Usage:   "Save results to the specified file",
		},
		&cli.StringFlag{
			Name:    "bind",
			Aliases: []string{"b"},
			Usage:   "Address and port to bind to.",
			Value:   "localhost:8080",
		},
	},
	Action: func(c *cli.Context) error {
		if c.Args().Len() < 2 {
			return fmt.Errorf("missing filename\nUsage: rsp timings [-o | -b] <filename> <scope>")
		}

		filename := c.Args().Get(0)
		scope := c.Args().Get(1)

		savePath := c.String("output")
		bindAddr := c.String("bind")

		if savePath != "" && c.IsSet("bind") {
			return fmt.Errorf("--output/-o and --bind/-b are mutually exclusive")
		}

		TimingsForScope(filename, scope, savePath, bindAddr)

		return nil
	},
}
