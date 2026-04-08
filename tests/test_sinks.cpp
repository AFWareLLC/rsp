#include <gtest/gtest.h>

#include <afware/rsp/Sinks.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>

namespace {

std::filesystem::path TempFile(const char *suffix) {
  return std::filesystem::temp_directory_path() / ("rsp_test_" + std::string(suffix) + ".bin");
}

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

}  // namespace

// --- SinkType enum ---

TEST(SinkType, Values) {
  EXPECT_EQ(static_cast<uint8_t>(rsp::SinkType::SILENT), 0);
  EXPECT_EQ(static_cast<uint8_t>(rsp::SinkType::COUT), 1);
  EXPECT_EQ(static_cast<uint8_t>(rsp::SinkType::BINARY_DISK), 2);
}

// --- BinaryDiskSink ---

TEST(BinaryDiskSink, OKAfterConstruction) {
  auto path = TempFile("ok_test");
  rsp::Machine machine;
  rsp::BinaryDiskSink sink(path, &machine);
  EXPECT_TRUE(sink.OK());
  std::filesystem::remove(path);
}

TEST(BinaryDiskSink, WritesData) {
  auto path = TempFile("write_test");
  rsp::Machine machine;

  {
    rsp::BinaryDiskSink sink(path, &machine);
    ScopedTestInfo t("WriteTest", 100, 200);
    sink.Sink(t.info);
  }

  EXPECT_TRUE(std::filesystem::exists(path));
  EXPECT_GT(std::filesystem::file_size(path), 0u);
  std::filesystem::remove(path);
}

TEST(BinaryDiskSink, WrittenDataIsDeserializable) {
  auto path = TempFile("deser_test");
  rsp::Machine machine;

  {
    rsp::BinaryDiskSink sink(path, &machine);
    ScopedTestInfo t("DeserScope", 1000, 2000);
    t.info.AddMetadata<int32_t>(rsp::MetadataTag{"items"}, 42);
    sink.Sink(t.info);
  }

  // Read back
  std::ifstream f(path, std::ios::binary);
  ASSERT_TRUE(f.is_open());

  uint32_t len = 0;
  f.read(reinterpret_cast<char *>(&len), sizeof(len));
  EXPECT_GT(len, 0u);

  std::vector<uint8_t> buf(len);
  f.read(reinterpret_cast<char *>(buf.data()), len);

  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
  EXPECT_STREQ(fb->tag()->c_str(), "DeserScope");
  EXPECT_EQ(fb->ticks_start(), 1000u);
  EXPECT_EQ(fb->ticks_end(), 2000u);
  EXPECT_EQ(fb->machine_nominal_freq_hz(), machine.GetNominalFreq());
  ASSERT_EQ(fb->metadata()->size(), 1u);
  EXPECT_STREQ(fb->metadata()->Get(0)->tag()->c_str(), "items");

  std::filesystem::remove(path);
}

TEST(BinaryDiskSink, MultipleWrites) {
  auto path = TempFile("multi_write_test");
  rsp::Machine machine;

  {
    rsp::BinaryDiskSink sink(path, &machine);
    for (int i = 0; i < 10; ++i) {
      ScopedTestInfo t("Scope", static_cast<uint64_t>(i * 100),
                       static_cast<uint64_t>(i * 100 + 50));
      sink.Sink(t.info);
    }
  }

  // Read all entries back
  std::ifstream f(path, std::ios::binary);
  int count = 0;
  while (f.peek() != EOF) {
    uint32_t len = 0;
    f.read(reinterpret_cast<char *>(&len), sizeof(len));
    if (!f.good()) break;

    std::vector<uint8_t> buf(len);
    f.read(reinterpret_cast<char *>(buf.data()), len);
    if (!f.good()) break;

    auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
    EXPECT_STREQ(fb->tag()->c_str(), "Scope");
    ++count;
  }

  EXPECT_EQ(count, 10);
  std::filesystem::remove(path);
}

TEST(BinaryDiskSink, AppendMode) {
  auto path = TempFile("append_test");
  std::filesystem::remove(path);  // Ensure clean start
  rsp::Machine machine;

  // Write first entry
  {
    rsp::BinaryDiskSink sink(path, &machine);
    ScopedTestInfo t("First", 1, 2);
    sink.Sink(t.info);
  }

  // Write second entry (append)
  {
    rsp::BinaryDiskSink sink(path, &machine);
    ScopedTestInfo t("Second", 3, 4);
    sink.Sink(t.info);
  }

  // Should have two entries
  std::ifstream f(path, std::ios::binary);
  int count = 0;
  while (f.peek() != EOF) {
    uint32_t len = 0;
    f.read(reinterpret_cast<char *>(&len), sizeof(len));
    if (!f.good()) break;
    f.seekg(len, std::ios::cur);
    ++count;
  }
  EXPECT_EQ(count, 2);
  std::filesystem::remove(path);
}
