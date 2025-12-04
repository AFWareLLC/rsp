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
	"log"
	"net/http"
	"os"

	"github.com/go-echarts/go-echarts/v2/components"
)

func ServeChartsPage(addr string, chartsList ...components.Charter) {
	page := components.NewPage()
	page.SetLayout(components.PageFlexLayout)
	page.AddCharts(chartsList...)

	http.HandleFunc("/", func(w http.ResponseWriter, _ *http.Request) {
		if err := page.Render(w); err != nil {
			log.Println("Error rendering page:", err)
		}
	})

	log.Printf("Serving charts page at http://%s/", addr)
	log.Fatal(http.ListenAndServe(addr, nil))
}

func SaveChartsPageHTML(filename string, chartsList ...components.Charter) {
	page := components.NewPage()
	page.SetLayout(components.PageFlexLayout)
	page.AddCharts(chartsList...)

	f, err := os.Create(filename)
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()

	if err := page.Render(f); err != nil {
		log.Fatal(err)
	}

	log.Printf("Saved charts page to %s", filename)
}
