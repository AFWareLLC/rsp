#include <gtest/gtest.h>

#include <afware/rsp/Machine.hpp>

TEST(Machine, OK) {
  rsp::Machine machine;
  EXPECT_TRUE(machine.OK());
}

TEST(Machine, NominalFreqNonZero) {
  rsp::Machine machine;
  EXPECT_GT(machine.GetNominalFreq(), 0u);
}

TEST(Machine, NominalFreqReasonableRange) {
  rsp::Machine machine;
  uint64_t freq = machine.GetNominalFreq();
  // Should be somewhere between 100 MHz and 10 GHz
  EXPECT_GT(freq, 100'000'000ull);
  EXPECT_LT(freq, 10'000'000'000ull);
}

// --- Now() ---

TEST(Now, ReturnsNonZero) {
  EXPECT_GT(rsp::Now(), 0u);
}

TEST(Now, Monotonic) {
  uint64_t a = rsp::Now();
  uint64_t b = rsp::Now();
  EXPECT_GE(b, a);
}

TEST(Now, MonotonicAcrossMultipleCalls) {
  uint64_t prev = rsp::Now();
  for (int i = 0; i < 1000; ++i) {
    uint64_t curr = rsp::Now();
    EXPECT_GE(curr, prev) << "Monotonicity violated at iteration " << i;
    prev = curr;
  }
}

TEST(Now, AdvancesOverTime) {
  uint64_t a = rsp::Now();
  // Do some busy work
  volatile int x = 0;
  for (int i = 0; i < 10000; ++i) {
    x += i;
  }
  (void)x;
  uint64_t b = rsp::Now();
  EXPECT_GT(b, a);
}

// --- Platform-specific detection ---

#if defined(__x86_64__) || defined(_M_X64)

TEST(AMD64, HasInvariantTSC) {
  EXPECT_TRUE(rsp::AMD64_HasInvariantTSC());
}

TEST(AMD64, GetNominalTSCHzNonZero) {
  EXPECT_GT(rsp::GetNominalTSCHz(), 0u);
}

#elif defined(__aarch64__) || defined(_M_ARM64)

TEST(ARM64, ReadCntfrqHzNonZero) {
  EXPECT_GT(rsp::ARM64_ReadCntfrqHz(), 0u);
}

TEST(ARM64, CounterLooksSane) {
  uint64_t freq = rsp::ARM64_ReadCntfrqHz();
  EXPECT_TRUE(rsp::ARM64_CounterLooksSane(freq));
}

#endif
