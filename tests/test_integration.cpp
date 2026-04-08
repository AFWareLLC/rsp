#include <gtest/gtest.h>

#include <afware/rsp/API.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

std::filesystem::path TempFile(const char *suffix) {
  return std::filesystem::temp_directory_path() / ("rsp_integ_" + std::string(suffix) + ".bin");
}

// Directly test the serialization pipeline (ScopeInfo -> BinaryDiskSink -> file -> deserialize)
// without relying on the profiler's background thread.
struct ScopedTestInfo {
  rsp::MetadataSlot slot;
  rsp::ScopeInfo info;

  ScopedTestInfo(const char *tag, uint64_t start, uint64_t end)
      : info(rsp::ScopeTag{tag}) {
    info.metadata_ptr = &slot;
    info.ticks_start  = start;
    info.ticks_end    = end;
  }
};

// Helper: read all ScopeInfo entries from a binary file
std::vector<const RSP::ScopeInfo *> ReadEntries(
    const std::filesystem::path &path,
    std::vector<std::vector<uint8_t>> &buffers) {
  std::vector<const RSP::ScopeInfo *> entries;
  std::ifstream f(path, std::ios::binary);
  while (f.peek() != EOF) {
    uint32_t len = 0;
    f.read(reinterpret_cast<char *>(&len), sizeof(len));
    if (!f.good()) break;

    buffers.emplace_back(len);
    f.read(reinterpret_cast<char *>(buffers.back().data()), len);
    if (!f.good()) break;

    entries.push_back(flatbuffers::GetRoot<RSP::ScopeInfo>(buffers.back().data()));
  }
  return entries;
}

}  // namespace

// --- End-to-end pipeline ---

TEST(Integration, SingleScopeEndToEnd) {
  auto path = TempFile("single");
  rsp::Machine machine;

  {
    rsp::BinaryDiskSink sink(path, &machine);
    ScopedTestInfo t("EndToEnd", 5000, 10000);
    t.info.AddMetadata<int32_t>(rsp::MetadataTag{"items"}, 99);
    t.info.AddMetadata<double>(rsp::MetadataTag{"ratio"}, 0.75);
    sink.Sink(t.info);
  }

  std::vector<std::vector<uint8_t>> bufs;
  auto entries = ReadEntries(path, bufs);

  ASSERT_EQ(entries.size(), 1u);
  auto *e = entries[0];

  EXPECT_STREQ(e->tag()->c_str(), "EndToEnd");
  EXPECT_EQ(e->ticks_start(), 5000u);
  EXPECT_EQ(e->ticks_end(), 10000u);
  EXPECT_EQ(e->machine_nominal_freq_hz(), machine.GetNominalFreq());

  ASSERT_EQ(e->metadata()->size(), 2u);
  EXPECT_STREQ(e->metadata()->Get(0)->tag()->c_str(), "items");
  EXPECT_EQ(e->metadata()->Get(0)->type(),
            static_cast<RSP::MetadataType>(rsp::MetadataType::INT32));
  EXPECT_STREQ(e->metadata()->Get(1)->tag()->c_str(), "ratio");
  EXPECT_EQ(e->metadata()->Get(1)->type(),
            static_cast<RSP::MetadataType>(rsp::MetadataType::DOUBLE));

  // Verify metadata values
  int32_t items_val;
  uint64_t raw0 = e->metadata()->Get(0)->value();
  std::memcpy(&items_val, &raw0, sizeof(int32_t));
  EXPECT_EQ(items_val, 99);

  double ratio_val;
  uint64_t raw1 = e->metadata()->Get(1)->value();
  std::memcpy(&ratio_val, &raw1, sizeof(double));
  EXPECT_DOUBLE_EQ(ratio_val, 0.75);

  std::filesystem::remove(path);
}

TEST(Integration, MultipleScopesEndToEnd) {
  auto path = TempFile("multi");
  rsp::Machine machine;

  {
    rsp::BinaryDiskSink sink(path, &machine);
    for (int i = 0; i < 50; ++i) {
      ScopedTestInfo t("Batch", static_cast<uint64_t>(i * 1000),
                       static_cast<uint64_t>(i * 1000 + 500));
      t.info.AddMetadata<uint32_t>(rsp::MetadataTag{"idx"}, static_cast<uint32_t>(i));
      sink.Sink(t.info);
    }
  }

  std::vector<std::vector<uint8_t>> bufs;
  auto entries = ReadEntries(path, bufs);

  ASSERT_EQ(entries.size(), 50u);
  for (int i = 0; i < 50; ++i) {
    EXPECT_STREQ(entries[i]->tag()->c_str(), "Batch");
    EXPECT_EQ(entries[i]->ticks_start(), static_cast<uint64_t>(i * 1000));
    EXPECT_EQ(entries[i]->ticks_end(), static_cast<uint64_t>(i * 1000 + 500));

    uint32_t idx;
    uint64_t raw = entries[i]->metadata()->Get(0)->value();
    std::memcpy(&idx, &raw, sizeof(uint32_t));
    EXPECT_EQ(idx, static_cast<uint32_t>(i));
  }

  std::filesystem::remove(path);
}

TEST(Integration, NestedScopesWithMetadata) {
  auto path = TempFile("nested");
  rsp::Machine machine;

  {
    rsp::BinaryDiskSink sink(path, &machine);

    // Simulate nested scopes written to sink
    ScopedTestInfo outer("Outer", 100, 1000);
    outer.info.AddMetadata<int32_t>(rsp::MetadataTag{"depth"}, 0);
    sink.Sink(outer.info);

    ScopedTestInfo inner("Inner", 200, 800);
    inner.info.AddMetadata<int32_t>(rsp::MetadataTag{"depth"}, 1);
    inner.info.AddMetadata<uint64_t>(rsp::MetadataTag{"items"}, 42);
    sink.Sink(inner.info);
  }

  std::vector<std::vector<uint8_t>> bufs;
  auto entries = ReadEntries(path, bufs);

  ASSERT_EQ(entries.size(), 2u);
  EXPECT_STREQ(entries[0]->tag()->c_str(), "Outer");
  EXPECT_STREQ(entries[1]->tag()->c_str(), "Inner");
  EXPECT_EQ(entries[0]->metadata()->size(), 1u);
  EXPECT_EQ(entries[1]->metadata()->size(), 2u);

  std::filesystem::remove(path);
}

TEST(Integration, AllMetadataTypesEndToEnd) {
  auto path = TempFile("all_types");
  rsp::Machine machine;

  {
    rsp::BinaryDiskSink sink(path, &machine);

    ScopedTestInfo t("AllTypes", 0, 0);
    t.info.AddMetadata<int8_t>(rsp::MetadataTag{"i8"}, -1);
    t.info.AddMetadata<uint8_t>(rsp::MetadataTag{"u8"}, 255);
    t.info.AddMetadata<int16_t>(rsp::MetadataTag{"i16"}, -1000);
    t.info.AddMetadata<uint16_t>(rsp::MetadataTag{"u16"}, 60000);
    t.info.AddMetadata<int32_t>(rsp::MetadataTag{"i32"}, -100000);
    t.info.AddMetadata<uint32_t>(rsp::MetadataTag{"u32"}, 100000);
    t.info.AddMetadata<float>(rsp::MetadataTag{"f32"}, 3.14f);
    t.info.AddMetadata<double>(rsp::MetadataTag{"f64"}, 2.718281828);
    sink.Sink(t.info);
  }

  std::vector<std::vector<uint8_t>> bufs;
  auto entries = ReadEntries(path, bufs);

  ASSERT_EQ(entries.size(), 1u);
  auto *e = entries[0];
  ASSERT_EQ(e->metadata()->size(), 8u);

  // Verify types (use cast because C++ and FlatBuffer enum orderings differ for FLOAT/DOUBLE)
  EXPECT_EQ(e->metadata()->Get(0)->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::INT8));
  EXPECT_EQ(e->metadata()->Get(1)->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::UINT8));
  EXPECT_EQ(e->metadata()->Get(2)->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::INT16));
  EXPECT_EQ(e->metadata()->Get(3)->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::UINT16));
  EXPECT_EQ(e->metadata()->Get(4)->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::INT32));
  EXPECT_EQ(e->metadata()->Get(5)->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::UINT32));
  EXPECT_EQ(e->metadata()->Get(6)->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::FLOAT));
  EXPECT_EQ(e->metadata()->Get(7)->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::DOUBLE));

  // Spot-check values
  int8_t i8_val;
  uint64_t raw = e->metadata()->Get(0)->value();
  std::memcpy(&i8_val, &raw, sizeof(int8_t));
  EXPECT_EQ(i8_val, -1);

  double f64_val;
  raw = e->metadata()->Get(7)->value();
  std::memcpy(&f64_val, &raw, sizeof(double));
  EXPECT_DOUBLE_EQ(f64_val, 2.718281828);

  std::filesystem::remove(path);
}

// --- Live ActiveScope through profiler to binary sink ---

TEST(Integration, ActiveScopeThroughProfiler) {
  auto path = TempFile("live_scope");
  std::filesystem::remove(path);

  auto sink_ptr = std::make_shared<rsp::BinaryDiskSink>(path, rsp::Instance().GetMachine());
  ASSERT_TRUE(sink_ptr->OK());
  rsp::Instance().SetSinkToBinaryDisk(sink_ptr);

  // Brief pause to let the sink thread pick up the new sink
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Create scopes - the profiler's sink thread will process them
  for (int i = 0; i < 5; ++i) {
    RSP_SCOPE("LiveScope");
    RSP_SCOPE_METADATA("iter", static_cast<int32_t>(i));
    // Small busy loop to ensure measurable time
    volatile int x = 0;
    for (int j = 0; j < 100; ++j) x += j;
    (void)x;
  }

  // Give sink thread generous time to drain all items
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Restore to silent and release our reference to the sink.
  // Dropping the last shared_ptr destroys the BinaryDiskSink,
  // which closes and flushes the ofstream.
  rsp::Instance().SetSinkToSilent();
  sink_ptr.reset();

  // Verify file was written
  ASSERT_TRUE(std::filesystem::exists(path));
  ASSERT_GT(std::filesystem::file_size(path), 0u);

  // Read and verify entries
  std::vector<std::vector<uint8_t>> bufs;
  auto entries = ReadEntries(path, bufs);

  EXPECT_EQ(entries.size(), 5u);
  for (size_t i = 0; i < entries.size(); ++i) {
    EXPECT_STREQ(entries[i]->tag()->c_str(), "LiveScope");
    EXPECT_GT(entries[i]->ticks_end(), entries[i]->ticks_start());
    EXPECT_GT(entries[i]->machine_nominal_freq_hz(), 0u);
    ASSERT_GE(entries[i]->metadata()->size(), 1u);
    EXPECT_STREQ(entries[i]->metadata()->Get(0)->tag()->c_str(), "iter");
  }

  std::filesystem::remove(path);
}

// --- Timing sanity ---

TEST(Integration, TimingConversionSanity) {
  rsp::Machine machine;
  uint64_t freq = machine.GetNominalFreq();

  uint64_t start = rsp::Now();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  uint64_t end = rsp::Now();

  double seconds = static_cast<double>(end - start) / static_cast<double>(freq);

  // Should be roughly 10ms, allow wide margin (5ms to 200ms)
  EXPECT_GT(seconds, 0.005);
  EXPECT_LT(seconds, 0.200);
}

// --- No metadata scopes ---

TEST(Integration, ScopeWithNoMetadata) {
  auto path = TempFile("no_meta");
  rsp::Machine machine;

  {
    rsp::BinaryDiskSink sink(path, &machine);
    ScopedTestInfo t("NoMeta", 100, 200);
    sink.Sink(t.info);
  }

  std::vector<std::vector<uint8_t>> bufs;
  auto entries = ReadEntries(path, bufs);

  ASSERT_EQ(entries.size(), 1u);
  EXPECT_EQ(entries[0]->metadata()->size(), 0u);
  EXPECT_EQ(entries[0]->max_offset(), 0);

  std::filesystem::remove(path);
}
