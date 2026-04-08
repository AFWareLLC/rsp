// Compiled WITHOUT -DRSP_ENABLE to test the disabled API surface.

#include <gtest/gtest.h>

#include <afware/rsp/API.hpp>

TEST(APIDisabled, AvailableReturnsFalse) {
  EXPECT_FALSE(rsp::Available());
}

TEST(APIDisabled, StartReturnsFalse) {
  EXPECT_FALSE(rsp::Start());
}

TEST(APIDisabled, StopDoesNotCrash) {
  rsp::Stop();
}

TEST(APIDisabled, ScopeMacroIsNoOp) {
  RSP_SCOPE("disabled_scope");
  // Should compile and not crash
}

TEST(APIDisabled, ScopeMetadataMacroIsNoOp) {
  RSP_SCOPE_METADATA("key", 42);
  // Should compile and not crash
}

TEST(APIDisabled, FunctionScopeMacroIsNoOp) {
  RSP_FUNCTION_SCOPE;
  // Should compile and not crash
}

TEST(APIDisabled, AllMacrosTogetherNoOp) {
  RSP_SCOPE("test");
  RSP_SCOPE_METADATA("a", 1);
  RSP_SCOPE_METADATA("b", 2.0);
  RSP_FUNCTION_SCOPE;
  // All should be no-ops
}
