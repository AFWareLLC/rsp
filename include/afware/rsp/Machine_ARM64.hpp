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

#include <chrono>
#include <cstdint>
#include <fstream>
#include <thread>

//
// This is ARM64-specific machine support code.
// Assumptions made:
// - Compiler supports inline
// - CNTVCT_EL0 is available
// - We treat CNTVCT as the equivalent of the invariant TSC.
//

namespace rsp {

inline uint64_t Now() {
  uint64_t val;
  asm volatile("isb; mrs %0, cntvct_el0" : "=r"(val));
  return val;
}

inline uint64_t GetNominalFreq() {
  uint64_t freq;
  asm volatile("isb; mrs %0, cntfrq_el0" : "=r"(freq));
  return freq;
}

//
// This is the final abstraction of the machine - which gets instantiated by the
// profiler on startup. The bits and pieces if provides access to are needed to
// compute accurate timings from the TSC.
//
// This more or less glues together all the machine specific nonsense above
// to give a nominal TSC frequency estimate and provide an indication
// that we have the correct hardware to profile successfully.
//

class Machine {
public:
  Machine() {
    nominal_freq_ = GetNominalFreq();
  }

  bool OK() const {
    (nominal_freq_ != 0);
  }

  uint64_t GetNominalFreq() const {
    return nominal_freq_;
  }

private:
  uint64_t nominal_freq_ = 0;
};

}  // namespace rsp
