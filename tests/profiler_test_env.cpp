#include <gtest/gtest.h>

#include <afware/rsp/API.hpp>

#include <chrono>
#include <thread>

namespace {

class ProfilerTestEnvironment : public ::testing::Environment {
public:
  void SetUp() override {
    rsp::Instance().SetSinkToSilent();
    ASSERT_TRUE(rsp::Instance().Start());
  }

  void TearDown() override {
    rsp::Instance().Stop();
    // Give the sink thread time to drain remaining items.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
};

[[maybe_unused]] ::testing::Environment *const profiler_env =
    ::testing::AddGlobalTestEnvironment(new ProfilerTestEnvironment);

}  // namespace
