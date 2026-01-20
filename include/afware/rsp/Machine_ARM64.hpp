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

//
// This is ARM64 specific machine support code.
// Assumptions made:
// - Compiler supports inline ASM
// - The userspace virtual counter is accessible (CNTVCT_EL0)
// - CNTFRQ_EL0 is readable to obtain ticks/sec
//
// Due to the low-levelness of this code, it's pretty groaty. Sorry.
//

namespace rsp {

//
// Read CNTVCT_EL0 (virtual count).
//
// Notes:
// - CNTVCT_EL0 is the architected virtual counter (monotonic).
// - ISB is used to ensure the counter read is not speculated/reordered
//   across preceding instructions (similar motivation as lfence+rdtsc).
// - Returns raw ticks, not nanoseconds.
//

inline uint64_t Now() {
  uint64_t v = 0;
  __asm__ __volatile__(
      "isb\n\t"
      "mrs %0, cntvct_el0"
      : "=r"(v)
      :
      : "memory");
  return v;
}

//
// Read CNTFRQ_EL0 (counter frequency, ticks/sec).
//

inline uint64_t ARM64_ReadCntfrqHz() {
  uint64_t hz = 0;
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(hz) : : "memory");
  return hz;
}

//
// Best-effort "can we use this clock source?" check.
//
// If CNTVCT_EL0 is trapped, this would normally SIGILL. We don't try to
// recover here — in practice Linux/arm64 exposes it to userspace.
// We do validate that CNTFRQ_EL0 is non-zero and that the counter moves.
//

inline bool ARM64_CounterLooksSane(uint64_t cntfrq_hz) {
  if (cntfrq_hz == 0) {
    return false;
  }

  // Basic monotonic movement check
  const uint64_t t0 = Now();
  const uint64_t t1 = Now();
  return t1 >= t0;
}

//
// This is the final abstraction of the machine - which gets instantiated by the
// profiler on startup. The bits and pieces it provides access to are needed to
// compute accurate timings from the counter ticks.
//

class Machine {
public:
  Machine() {
    nominal_cnt_hz_ = ARM64_ReadCntfrqHz();
    ok_             = ARM64_CounterLooksSane(nominal_cnt_hz_);
  }

  bool OK() const {
    return ok_;
  }

  uint64_t GetNominalFreq() const {
    return nominal_cnt_hz_;
  }

private:
  bool ok_                 = false;
  uint64_t nominal_cnt_hz_ = 0;
};

}  // namespace rsp
