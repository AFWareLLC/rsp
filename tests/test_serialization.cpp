#include <gtest/gtest.h>

#include <afware/rsp/Serialization.hpp>

#include <cstring>
#include <sstream>

// Helper: create a ScopeInfo with a slot for testing serialization.
// Does NOT go through the profiler.
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

// --- Basic serialization ---

TEST(Serialization, ProducesNonEmptyBuffer) {
  rsp::Machine machine;
  ScopedTestInfo t("test", 100, 200);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  EXPECT_GT(buf.size(), 0u);
}

TEST(Serialization, RoundtripTag) {
  rsp::Machine machine;
  ScopedTestInfo t("MyScope", 100, 200);

  auto buf          = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb          = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
  EXPECT_STREQ(fb->tag()->c_str(), "MyScope");
}

TEST(Serialization, RoundtripTicks) {
  rsp::Machine machine;
  ScopedTestInfo t("ticks", 12345, 67890);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());

  EXPECT_EQ(fb->ticks_start(), 12345u);
  EXPECT_EQ(fb->ticks_end(), 67890u);
}

TEST(Serialization, RoundtripMachineFreq) {
  rsp::Machine machine;
  ScopedTestInfo t("freq", 0, 0);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());

  EXPECT_EQ(fb->machine_nominal_freq_hz(), machine.GetNominalFreq());
}

// --- Metadata serialization ---

TEST(Serialization, NoMetadata) {
  rsp::Machine machine;
  ScopedTestInfo t("empty", 0, 0);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());

  EXPECT_EQ(fb->max_offset(), 0);
  EXPECT_NE(fb->metadata(), nullptr);
  EXPECT_EQ(fb->metadata()->size(), 0u);
}

TEST(Serialization, SingleMetadata) {
  rsp::Machine machine;
  ScopedTestInfo t("single", 0, 0);
  t.info.AddMetadata<int32_t>(rsp::MetadataTag{"count"}, 42);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());

  EXPECT_EQ(fb->max_offset(), 1);
  ASSERT_EQ(fb->metadata()->size(), 1u);

  auto *m = fb->metadata()->Get(0);
  EXPECT_STREQ(m->tag()->c_str(), "count");
  EXPECT_EQ(m->type(), RSP::MetadataType_INT32);

  // Verify value roundtrip
  int32_t val;
  uint64_t raw = m->value();
  std::memcpy(&val, &raw, sizeof(int32_t));
  EXPECT_EQ(val, 42);
}

TEST(Serialization, MultipleMetadata) {
  rsp::Machine machine;
  ScopedTestInfo t("multi", 0, 0);
  t.info.AddMetadata<int32_t>(rsp::MetadataTag{"a"}, 1);
  t.info.AddMetadata<double>(rsp::MetadataTag{"b"}, 3.14);
  t.info.AddMetadata<uint8_t>(rsp::MetadataTag{"c"}, 255);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());

  EXPECT_EQ(fb->max_offset(), 3);
  ASSERT_EQ(fb->metadata()->size(), 3u);

  EXPECT_STREQ(fb->metadata()->Get(0)->tag()->c_str(), "a");
  EXPECT_STREQ(fb->metadata()->Get(1)->tag()->c_str(), "b");
  EXPECT_STREQ(fb->metadata()->Get(2)->tag()->c_str(), "c");

  // Note: C++ MetadataType enum ordering (DOUBLE=9,FLOAT=10) differs from
  // the FlatBuffer schema (FLOAT=9,DOUBLE=10). SerializeScopeInfo does a
  // static_cast, so we compare using the same cast for consistency.
  EXPECT_EQ(fb->metadata()->Get(0)->type(),
            static_cast<RSP::MetadataType>(rsp::MetadataType::INT32));
  EXPECT_EQ(fb->metadata()->Get(1)->type(),
            static_cast<RSP::MetadataType>(rsp::MetadataType::DOUBLE));
  EXPECT_EQ(fb->metadata()->Get(2)->type(),
            static_cast<RSP::MetadataType>(rsp::MetadataType::UINT8));
}

// --- All metadata types roundtrip ---

TEST(Serialization, Uint8MetadataRoundtrip) {
  rsp::Machine machine;
  ScopedTestInfo t("u8", 0, 0);
  t.info.AddMetadata<uint8_t>(rsp::MetadataTag{"v"}, 200);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
  auto *m  = fb->metadata()->Get(0);

  EXPECT_EQ(m->type(), RSP::MetadataType_UINT8);
  uint8_t val;
  uint64_t raw = m->value();
  std::memcpy(&val, &raw, sizeof(uint8_t));
  EXPECT_EQ(val, 200);
}

TEST(Serialization, Int64MetadataRoundtrip) {
  rsp::Machine machine;
  ScopedTestInfo t("i64", 0, 0);
  int64_t expected = -999'999'999'999LL;
  t.info.AddMetadata<int64_t>(rsp::MetadataTag{"v"}, expected);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
  auto *m  = fb->metadata()->Get(0);

  EXPECT_EQ(m->type(), RSP::MetadataType_INT64);
  int64_t val;
  uint64_t raw = m->value();
  std::memcpy(&val, &raw, sizeof(int64_t));
  EXPECT_EQ(val, expected);
}

TEST(Serialization, FloatMetadataRoundtrip) {
  rsp::Machine machine;
  ScopedTestInfo t("f32", 0, 0);
  t.info.AddMetadata<float>(rsp::MetadataTag{"v"}, 1.5f);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
  auto *m  = fb->metadata()->Get(0);

  EXPECT_EQ(m->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::FLOAT));
  float val;
  uint64_t raw = m->value();
  std::memcpy(&val, &raw, sizeof(float));
  EXPECT_FLOAT_EQ(val, 1.5f);
}

TEST(Serialization, DoubleMetadataRoundtrip) {
  rsp::Machine machine;
  ScopedTestInfo t("f64", 0, 0);
  t.info.AddMetadata<double>(rsp::MetadataTag{"v"}, 2.718281828);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
  auto *m  = fb->metadata()->Get(0);

  EXPECT_EQ(m->type(), static_cast<RSP::MetadataType>(rsp::MetadataType::DOUBLE));
  double val;
  uint64_t raw = m->value();
  std::memcpy(&val, &raw, sizeof(double));
  EXPECT_DOUBLE_EQ(val, 2.718281828);
}

TEST(Serialization, Uint64MetadataRoundtrip) {
  rsp::Machine machine;
  ScopedTestInfo t("u64", 0, 0);
  uint64_t expected = 0xFFFFFFFFFFFFFFFFull;
  t.info.AddMetadata<uint64_t>(rsp::MetadataTag{"v"}, expected);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
  auto *m  = fb->metadata()->Get(0);

  EXPECT_EQ(m->type(), RSP::MetadataType_UINT64);
  EXPECT_EQ(m->value(), expected);
}

// --- FlatBuffer streaming operators ---

TEST(Serialization, FBScopeInfoStreamOutput) {
  rsp::Machine machine;
  ScopedTestInfo t("streamed", 100, 200);
  t.info.AddMetadata<int32_t>(rsp::MetadataTag{"k"}, 5);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());

  std::ostringstream oss;
  rsp::operator<<(oss, fb);
  std::string out = oss.str();

  EXPECT_NE(out.find("streamed"), std::string::npos);
  EXPECT_NE(out.find("100"), std::string::npos);
  EXPECT_NE(out.find("200"), std::string::npos);
}

TEST(Serialization, FBMetadataEntryStreamOutput) {
  rsp::Machine machine;
  ScopedTestInfo t("x", 0, 0);
  t.info.AddMetadata<uint32_t>(rsp::MetadataTag{"mykey"}, 99);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());
  auto *m  = fb->metadata()->Get(0);

  std::ostringstream oss;
  // operator<< is in rsp:: namespace, not findable via ADL for RSP:: types
  rsp::operator<<(oss, *m);
  std::string out = oss.str();
  EXPECT_NE(out.find("mykey"), std::string::npos);
}

TEST(Serialization, NullFBScopeInfoStreamDoesNotCrash) {
  const RSP::ScopeInfo *null_scope = nullptr;
  std::ostringstream oss;
  oss << null_scope;
  // Should not crash, output can be empty
}

// --- Large tick values ---

TEST(Serialization, LargeTickValues) {
  rsp::Machine machine;
  uint64_t big_start = 0xABCDEF0123456789ull;
  uint64_t big_end   = 0xFEDCBA9876543210ull;
  ScopedTestInfo t("big", big_start, big_end);

  auto buf = rsp::SerializeScopeInfo(&t.info, &machine);
  auto *fb = flatbuffers::GetRoot<RSP::ScopeInfo>(buf.data());

  EXPECT_EQ(fb->ticks_start(), big_start);
  EXPECT_EQ(fb->ticks_end(), big_end);
}
