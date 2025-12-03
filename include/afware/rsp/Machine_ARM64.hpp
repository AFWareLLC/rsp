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

#pragma once

#include <cstdint>
#include <ctime>

namespace rsp {
//
// ARM64 time source: CLOCK_MONOTONIC_RAW
//
// This is the best way to approach this for ARM, since the counters
// are typically privledged and aren't userspace accessible.
//
// This is a stable, non-adjusted, high-resolution clock exposed by
// the Linux VDSO. It is much faster than a syscall and does *not*
// slew or jump due to NTP.
//

inline uint64_t Now() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return uint64_t(ts.tv_sec) * 1'000'000'000ull + uint64_t(ts.tv_nsec);
}

//
// ARM64 Machine abstraction
//

class Machine {
public:
  Machine() {
  }

  bool OK() const {
    return true;
  }

  uint64_t GetNominalFreq() const {
    return 1'000'000'000ull;  // nanosecond resolution
  }
};

}  // namespace rsp
