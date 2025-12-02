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
// This is AMD64 specific machine support code.
// Assumptions made:
// - Compiler suppoorts inline ASM
// - Machine has an invariant TSC (we do not support variant and profiling will not start
//  unless we detect TSC invariance).
//
// Due to the low-levelness of this code, it's pretty groaty. Sorry. Go read the Intel manuals.
//

namespace rsp {

//
// Read current RDTSC. We must serialize first with lfence,
// as the speculation/out-of-order execution pipelines can
// move instructions to before our RDTSC call. lfence
// enforces strict ordering.
//

inline uint64_t Now() {
  unsigned lo, hi;
  __asm__ __volatile__(
      "lfence\n\t"
      "rdtsc"
      : "=a"(lo), "=d"(hi)
      :
      : "memory");
  return ((uint64_t)hi << 32) | lo;
}

//
// Detect an invariant TSC.
//

inline bool AMD64_HasInvariantTSC() {
  uint32_t eax, ebx, ecx, edx;

  //
  // CPUID leaf 0x80000007, EDX[8] = Invariant TSC
  //

  __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000007), "c"(0) : "memory");

  return (edx & (1u << 8)) != 0;
}

//
// This will try to figure out the nominal TSC frequency.
//
// There's a couple of ways to do this:
//
// 1. Check /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq: the issue
//    here is that this may not be populated in WSL or Docker.
// 2. Parse output from lscpu
// 3. A rough calibration routine where we sleep for a known period
//    and check how much the counter changes.
//

inline void GetNominalTSCHz_cpufreq(uint64_t *freq) {
  std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
  if (!f.is_open()) {
    return;
  }

  unsigned long khz = 0;
  f >> khz;
  if (khz == 0) {
    return;
  }

  *freq = static_cast<uint64_t>(khz) * 1000ull;  // kHz -> Hz
}

inline void GetNominalTSCHz_lscpu(uint64_t *freq) {
  FILE *pipe = popen("lscpu", "r");

  if (pipe) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
      std::string line{buffer};
      auto pos = line.find("CPU max MHz:");
      if (pos != std::string::npos) {
        double mhz = std::stod(line.substr(pos + 12));

        if (mhz > 0.0) {
          *freq = static_cast<uint64_t>(mhz * 1e6);
        }
      }
    }

    pclose(pipe);
  }
}

inline void GetNominalTSCHz_cal(uint64_t *freq) {
  auto t0 = Now();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  auto t1 = Now();

  *freq = static_cast<uint64_t>((t1 - t0) * 10.0);
}

inline uint64_t GetNominalTSCHz() {
  uint64_t freq = 0;

  GetNominalTSCHz_cpufreq(&freq);
  if (freq != 0) {
    return freq;
  }

  GetNominalTSCHz_lscpu(&freq);
  if (freq != 0) {
    return freq;
  }

  GetNominalTSCHz_cal(&freq);
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
    tsc_invar_ = AMD64_HasInvariantTSC();
    if (tsc_invar_) {
      nominal_tsc_hz_ = GetNominalTSCHz();
    }
  }

  bool OK() const {
    return tsc_invar_ && (nominal_tsc_hz_ != 0);
  }

  uint64_t GetNominalFreq() const {
    return nominal_tsc_hz_;
  }

private:
  bool tsc_invar_          = false;
  uint64_t nominal_tsc_hz_ = 0;
};

}  // namespace rsp
