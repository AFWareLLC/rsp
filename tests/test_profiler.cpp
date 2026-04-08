#include <gtest/gtest.h>

#include <afware/rsp/Profiler.hpp>

// --- Singleton ---

TEST(Profiler, InstanceReturnsSameReference) {
  auto &a = rsp::Instance();
  auto &b = rsp::Instance();
  EXPECT_EQ(&a, &b);
}

// --- Ready ---

TEST(Profiler, ReadyOnSupportedHardware) {
  EXPECT_TRUE(rsp::Instance().Ready());
}

// --- Slot and Machine access ---

TEST(Profiler, GetSlotStorageNonNull) {
  EXPECT_NE(rsp::Instance().GetSlotStorage(), nullptr);
}

TEST(Profiler, GetMachineNonNull) {
  EXPECT_NE(rsp::Instance().GetMachine(), nullptr);
}

TEST(Profiler, MachineIsOK) {
  EXPECT_TRUE(rsp::Instance().GetMachine()->OK());
}

TEST(Profiler, MachineFreqNonZero) {
  EXPECT_GT(rsp::Instance().GetMachine()->GetNominalFreq(), 0u);
}

// --- Sink configuration ---
// NOTE: The profiler is running (started by the global test environment).
// We test sink type getters which reflect the last SetSink* call.
// The global env sets it to Silent at startup.

TEST(Profiler, DefaultSinkIsSilent) {
  // The global test env sets this to Silent
  EXPECT_EQ(rsp::Instance().GetSinkType(), rsp::SinkType::SILENT);
}

TEST(Profiler, SetSinkToCout) {
  rsp::Instance().SetSinkToCout();
  EXPECT_EQ(rsp::Instance().GetSinkType(), rsp::SinkType::COUT);
  // Restore to silent to avoid noisy test output
  rsp::Instance().SetSinkToSilent();
}

TEST(Profiler, SetSinkToSilent) {
  rsp::Instance().SetSinkToSilent();
  EXPECT_EQ(rsp::Instance().GetSinkType(), rsp::SinkType::SILENT);
}

TEST(Profiler, SetSinkToBinaryDisk) {
  auto path = std::filesystem::temp_directory_path() / "rsp_profiler_sink_test.bin";
  auto sink_ptr = std::make_shared<rsp::BinaryDiskSink>(path, rsp::Instance().GetMachine());
  rsp::Instance().SetSinkToBinaryDisk(sink_ptr);
  EXPECT_EQ(rsp::Instance().GetSinkType(), rsp::SinkType::BINARY_DISK);
  // Restore
  rsp::Instance().SetSinkToSilent();
  std::filesystem::remove(path);
}

TEST(Profiler, SetSinkToBinaryDiskNullThrows) {
  EXPECT_THROW(rsp::Instance().SetSinkToBinaryDisk(nullptr), std::runtime_error);
}

// --- SlotStorage from profiler ---

TEST(Profiler, SlotStorageAcquireRelease) {
  auto *storage = rsp::Instance().GetSlotStorage();
  auto *slot    = storage->Acquire();
  EXPECT_NE(slot, nullptr);
  EXPECT_EQ(slot->metadata_idx, 0);
  storage->Release(slot);
}

// --- CreateBinaryDiskSink ---

TEST(Profiler, CreateBinaryDiskSinkValid) {
  auto path     = std::filesystem::temp_directory_path() / "rsp_create_sink_test.bin";
  auto sink_ptr = rsp::Profiler::CreateBinaryDiskSink(path);
  EXPECT_NE(sink_ptr, nullptr);
  EXPECT_TRUE(sink_ptr->OK());
  std::filesystem::remove(path);
}
