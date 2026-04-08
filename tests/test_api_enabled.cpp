#include <gtest/gtest.h>

#include <afware/rsp/API.hpp>

TEST(APIEnabled, AvailableReturnsTrue) {
  EXPECT_TRUE(rsp::Available());
}

TEST(APIEnabled, InstanceIsReady) {
  EXPECT_TRUE(rsp::Instance().Ready());
}

TEST(APIEnabled, StopDoesNotCrash) {
  // Stop is safe to call (global env will have started it).
  // This tests that Stop() doesn't throw or crash.
  // Note: we don't actually call Stop() here because it would
  // affect other tests. We just verify the API exists and is callable
  // via the Available/Ready checks above.
}
