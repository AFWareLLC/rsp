# Really S(imple|tupid) Profiler for C++

RSP (pronounced *rasp*) is designed to be an easy and practical scoped
profiler for C++. It is not the most feature-rich profiler available,
but it is efficient, predictable, and extremely easy to understand,
integrate, and extend.

Most profilers make it difficult to extract the specific timing
information algorithm developers actually need. They often generate
enormous event streams with high collection overhead and require
heavyweight GUI tools just to make sense of the results.

Our thesis is that while visualization is valuable, in practice
algorithm developers gain far more insight from flexible analytics and
aggregations over well-structured raw measurements.

Rather than building yet another profiler GUI, we designed the RSP
pipeline around a compact message format that makes it straightforward
to build custom, task-specific analysis tools tailored to your
workflow. The provided FlatBuffer schema keeps output efficient to
serialize, memory-friendly, and easily consumable from C++, Go,
Python, Rust, or any other language with FlatBuffers support.

RSP has been used internally in production for quite some time. It
originated as part of a larger codebase and has been lightly adapted
for public release. The profiler is intentionally opinionated: it
focuses on the needs of algorithm developers—people who require
precise, targeted measurements—rather than broad, system-wide
tracing. It excels at focused, low-overhead profiling, though it
performs adequately for more general workloads as well.

Included in the cli directory, is a standalone binary that can be
built with some  common analysis functions. See `cli/README.md`.

## Features

- Minimal overhead
- Scoped profiling WITH metadata
- Support for nested scoping
- Support for multithreading
- Serialized output (binary) in Flatbuffer format
- Profiling directives are able to be left in the code and "compiled out"
- Lightweight (header only), with only a single dependency that is not included - Flatbuffers.
- Configurable "sinks" - currently, streaming to `cout` or a file on-disk is supported. 
- Permissively licensed (ISC)

## Requirements

- AMD64 CPU with invariant TSC. We do not support "varying" TSCs - this should not be
  an issue for most modern chips.
- ARM64 chip. We use the CLOCK_MONOTONIC_RAW as the time source, as the hardware
  counters are typically not readable from userspace.
- Recent compiler supporting modern C++ standards (clang 18 or newer is recommended, as is `-std=c++23`, but `-std=c++17` works).
- (For now) Linux only
- `#include <flatbuffers/flatbuffers.h>` is needed, but `flatc` is only needed if you change the format definitions. You might
   need to recompile the flatbuffer schema to match your version fo flatbuffers: see `build_flatbuffers.sh`.

## A brief introduction

To use the profiler in your codebase, two steps are required.

1. In your sources, add `#include <afware/rsp/API.hpp>`
2. In your build options, or in a header that gets included *BEFORE* our header, `#define RSP_ENABLE`

The `#define RSP_ENABLE` is crucical, as otherwise no profiling data will be collected. Similarly, removing/omitting
this definition is how you can "compile out" the profiler. You can leave the profiling directives in place, however,
as long as you keep our headers included - we provide macros that are "no-ops" when profiling is not present in the binary.
Our library is entirely header-only, so there is nothing to link with.

In your `main()` somewhere, you should then add something like this:

```
//
// This will check if profiling is available: i.e. it's compiled in, and we were able to figure out
// that the CPU met the appropriate requirements.
//

if (rsp::ProfilingAvailable()) {
    auto sink_ptr = rsp::Profiler::CreateBinaryDiskSink("/path/to/profiling/output/on/disk");
    rsp::RSPInstance().SetSinkToBinaryDisk(sink_ptr);
	

	//
	// A call to StartProfiling() is necessary to ensure that:
	// - All internal resources for the profiler are allocated and available
	// - The configuration is correct
	// - The aggregation/sinking thread has started
	//

	if (!rsp::StartProfiling()) {
		throw std::runtime_error("Could not start profiling!");
	}
}

```

Your first profiling operation might look like:

```
void SomeFunction() {
	RSP_SCOPE("SomeFunction");
	
	// Do work...
	
}
```

That's it. You will see scoping information tagged as `SomeFunction`. It's recommended you name the scopes as something
fairly easy to parse so you can filter them later.

Let's say you wanted to add some metadata:

```
void SomeFunction() {
	RSP_SCOPE("SomeFunction");
	
	const auto some_list_of_items = GetWorkFromSomewhere();
	
	RSP_SCOPE_METADATA("ItemsInList", some_list_of_items.size());
	
	DoMoreWork(some_list_of_items);
}
```

Now, each scope will also include metadata about the number of items in the list.

Nesting is also supported:

```
void SomeFunction() {
	RSP_SCOPE("SomeFunction");
	
    const auto some_list_of_items = GetWorkFromSomewhere();
	
	RSP_SCOPE_METADATA("ItemsInList", some_list_of_items.size());
	
	DoMoreWork(some_list_of_items);
	
	const auto more_items = GetWorkFromSomewhere();
	
	RSP_SCOPE("Secondary");
	RSP_SCOPE_METADATA("ItemsInList", more_items.size());

}
```

In this case, the first scope (`SomeFunction`) will contain the timing related to the entire execution of the function, and will have
the `ItemsInList` metadata from the first `RSP_SCOPE_METADATA` directive associated with it. The second scope is the "child" scope,
and will contain timing information only for the portion of the execution after it was instantiated. Similarly, the second `RSP_SCOPE_METADATA` is associated with the `Secondary` scope.

Each scope contains the following elements:

- Start tick (upon construction)
- Stop tick (upon destruction)
- Metadata associated with the scope

Post-aggregation, these tick-deltas can be combined with the detected nominal clock rate and converted into timings.

The metadata is tagged with a key, and the value can be of a number of types: 8, 16, 32 or 64 bit ints (signed and unsigned), float or double.

For illustrative examples it is recommended that the user reviews the following examples:

- `examples/simple.cpp`: This is a simple example that prints out the output to `stdout`, making it easy to see the association between
   scopes and their metadata

- `examples/threaded.cpp`: An example showing how data from all threads get aggregated into a single output.
- `examples/disk_consumer.cpp` and `examples/disk_producer.cpp`: Example demonstrating serialization to disk.

These examples can be built by running `build_examples.sh` (assuming you have clang installed).


## Performance Footprint

We do our best to ensure that we aren't allocating in the hot parts of
the code, and that we really only do the bare minimum in the critical
path. In particular, we use a bump-style memory manager that
preallocates a block of memory up-front for storing our
metadata. These memory slots are managed by the Profiler and get
re-used. Should more slots be needed, only then do we
reallocate. Ideally, such reallocations will be rare as we set a
fairly conservative highwater mark: the relevant setting can be found
by grepping for `RSP_PROFILER_DEFAULT_STORAGE_SLOTS` - this is
changable at compile-time without modifying the code should it be
needed.

As a basic measure of performance, we defined a test program that performs a large number
of trials of two different algorithms for computing digits of pi.

See: `examples/speedtest.cpp`

Our test environment was openSUSE Tumbleweed running in WSL2 on a machine with a AMD Ryzen Threadripper PRO 7965WX. We
use `-O3 -march=native -mtune=native` compiling with clang 18.1.8.

The `build_examples.sh` script will produce two versions of the
speedtest binary - one with profiling enabled, the other without. In
each run, we perform 100000 trials of each of the two algorithms and
report the *total* time taken.

Our results are:

```
(opensuse) [HYDRA] -> ./bin/speedtest
Profiling enabled.
Profiling started.
Time doing actual work: 0.188837 seconds
(opensuse) [HYDRA] -> ./bin/speedtest_no_profiler
Profiling unavailable.
Time doing actual work: 0.165545 seconds
```

We declared this to be "good enough". Your mileage may vary, however
we have found in actual workloads (rather than highly contrived tests),
the footprint is barely noticable.

## Questions?

Please email `ajf <at> afware <dot> io`. Paid commerical support is available.

# License
```
Copyright (c) 2025, AFWare LLC. All Rights Reserved.

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted, provided
that the above copyright notice and this permission notice appear
in all copies.

THE SOFTWARE IS PROVIDED “AS IS” AND ISC DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.
```

## License for moodycamel::ConcurrentQueue
We depend upon (and include a copy of) the excellent
`moodycamel::ConcurrentQueue`, which can be found here:
https://github.com/cameron314/concurrentqueue

It is licensed as follows:
```
Copyright (c) 2013-2016, Cameron Desrochers. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
