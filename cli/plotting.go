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
	"github.com/go-echarts/go-echarts/v2/charts"
	"github.com/go-echarts/go-echarts/v2/opts"
	"strconv"
)

func CreateTimesPlot(times []float64, seriesName string, plotTitle string) *charts.Line {
	// X-axis labels (entry index)
	xAxis := make([]string, len(times))
	for i := range times {
		xAxis[i] = strconv.Itoa(i + 1)
	}

	// Create line chart
	line := charts.NewLine()
	line.SetGlobalOptions(
		charts.WithInitializationOpts(opts.Initialization{
			Width:  "100vh", // fill horizontal
			Height: "100vh", // fill most vertical space
		}),
		charts.WithGridOpts(opts.Grid{
			Left:   "10%",
			Right:  "10%",
			Top:    "10%",
			Bottom: "15%",
		}),
		charts.WithTitleOpts(opts.Title{Title: plotTitle}),
		charts.WithYAxisOpts(opts.YAxis{Name: "Time (ms)",
			Min: 0,
			Max: "dataMax + 10",
		}),
		charts.WithDataZoomOpts(
			opts.DataZoom{
				Type:       "inside", // mouse wheel / touch zoom
				XAxisIndex: []int{0},
			},
			opts.DataZoom{
				Type:       "slider", // slider at bottom
				XAxisIndex: []int{0},
			},
		),
	)

	data := make([]opts.LineData, len(times))
	for i, t := range times {
		data[i] = opts.LineData{Value: t}
	}

	showLegend := true

	line.SetXAxis(xAxis).AddSeries(seriesName, data)
	line.SetGlobalOptions(charts.WithLegendOpts(opts.Legend{Show: opts.Bool(showLegend)}))

	return line
}

func AddPercentilesToTimePlot(line *charts.Line, dataLen int, p50, p95, p99 float64) {
	repeat := func(val float64) []opts.LineData {
		out := make([]opts.LineData, dataLen)
		for i := range out {
			out[i] = opts.LineData{Value: val}
		}
		return out
	}

	showPercentiles := true

	line.AddSeries("P50", repeat(p50),
		charts.WithLineChartOpts(opts.LineChart{Smooth: opts.Bool(showPercentiles)}),
	)
	line.AddSeries("P95", repeat(p95),
		charts.WithLineChartOpts(opts.LineChart{Smooth: opts.Bool(showPercentiles)}),
	)
	line.AddSeries("P99", repeat(p99),
		charts.WithLineChartOpts(opts.LineChart{Smooth: opts.Bool(showPercentiles)}),
	)

	// Legend toggle

	showLegend := true
	line.SetGlobalOptions(
		charts.WithLegendOpts(opts.Legend{Show: opts.Bool(showLegend)}),
	)
}
