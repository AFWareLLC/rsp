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

//
// Read the ARM architectural counter.
//

inline uint64_t Now() {
  uint64_t val;
  asm volatile("mrs %0, cntvct_el0" : "=r"(val));
  return val;
}

//
// ARM64 has no CPUID, but CNTVCT and CNTFRQ are *architectural*.
// We define "invariant" as:
//  - CNTFRQ_EL0 exists and is nonzero
//  - CNTVCT_EL0 is monotonic over repeated reads
//

inline bool ARM64_HasInvariantCounter() {
  uint64_t freq = 0;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));

  if (freq == 0) {
    return false;
  }

  uint64_t a = Now();
  uint64_t b = Now();
  uint64_t c = Now();

  return (a < b) && (b < c);
}

//
// Try several ways to determine the nominal counter frequency.
// The architectural register CNTFRQ_EL0 is the first choice.
//

inline void GetNominalFreq_cntfrq(uint64_t* out) {
  uint64_t freq = 0;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));

  if (freq != 0) {
    *out = freq;
  }
}

//
// Sometimes SBCs expose CPU max frequency rather than CNTFRQ.
// Usually they match, but only use it as fallback.
//

inline void GetNominalFreq_cpufreq(uint64_t* out) {
  std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
  if (!f.is_open()) {
    return;
  }

  uint64_t khz = 0;
  f >> khz;
  if (khz != 0) {
    *out = khz * 1000ull;
  }
}

//
// Final fallback: calibrate with sleep.
//

inline void GetNominalFreq_cal(uint64_t* out) {
  auto t0 = Now();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  auto t1 = Now();
  *out    = static_cast<uint64_t>((t1 - t0) * 10.0);
}

inline uint64_t GetNominalFreq() {
  uint64_t freq = 0;

  GetNominalFreq_cntfrq(&freq);
  if (freq != 0) {
    return freq;
  }

  GetNominalFreq_cpufreq(&freq);
  if (freq != 0) {
    return freq;
  }

  GetNominalFreq_cal(&freq);
  if (freq != 0) {
    return freq;
  }

  return 0;
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
    invariant_ = ARM64_HasInvariantCounter();
    if (invariant_) {
      nominal_freq_ = GetNominalFreq();
    }
  }

  bool OK() const {
    return invariant_ && (nominal_freq_ != 0);
  }

  uint64_t GetNominalFreq() const {
    return nominal_freq_;
  }

private:
  bool invariant_        = false;
  uint64_t nominal_freq_ = 0;
};

}  // namespace rsp
