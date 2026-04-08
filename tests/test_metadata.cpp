#include <gtest/gtest.h>

#include <afware/rsp/Metadata.hpp>
#include <afware/rsp/Scope.hpp>

#include <cstring>
#include <limits>
#include <sstream>

// --- MetadataEntry default construction ---

TEST(MetadataEntry, DefaultConstruction) {
  rsp::MetadataEntry entry;
  EXPECT_EQ(entry.type, rsp::MetadataType::UNSET);
  EXPECT_STREQ(entry.tag.c_str(), "NOT SET");
}

TEST(MetadataEntry, ExplicitConstruction) {
  rsp::MetadataEntry entry{rsp::MetadataTag{"mykey"}, rsp::MetadataType::INT32};
  EXPECT_STREQ(entry.tag.c_str(), "mykey");
  EXPECT_EQ(entry.type, rsp::MetadataType::INT32);
}

TEST(MetadataEntry, MaxDataSize) {
  EXPECT_EQ(rsp::MetadataEntry::MAX_METADATA_DATA_SIZE_BYTES, 8u);
}

// --- Helper to read back stored data ---

template <typename T>
T ReadBack(const rsp::MetadataEntry &entry) {
  T val;
  std::memcpy(&val, entry.data.data(), sizeof(T));
  return val;
}

// --- MakeScopeMetadata for each type ---

TEST(MakeScopeMetadata, Uint8) {
  auto entry = rsp::MakeScopeMetadata<uint8_t>(rsp::MetadataTag{"u8"}, 42);
  EXPECT_EQ(entry.type, rsp::MetadataType::UINT8);
  EXPECT_STREQ(entry.tag.c_str(), "u8");
  EXPECT_EQ(ReadBack<uint8_t>(entry), 42);
}

TEST(MakeScopeMetadata, Int8) {
  auto entry = rsp::MakeScopeMetadata<int8_t>(rsp::MetadataTag{"i8"}, -10);
  EXPECT_EQ(entry.type, rsp::MetadataType::INT8);
  EXPECT_EQ(ReadBack<int8_t>(entry), -10);
}

TEST(MakeScopeMetadata, Uint16) {
  auto entry = rsp::MakeScopeMetadata<uint16_t>(rsp::MetadataTag{"u16"}, 1000);
  EXPECT_EQ(entry.type, rsp::MetadataType::UINT16);
  EXPECT_EQ(ReadBack<uint16_t>(entry), 1000);
}

TEST(MakeScopeMetadata, Int16) {
  auto entry = rsp::MakeScopeMetadata<int16_t>(rsp::MetadataTag{"i16"}, -500);
  EXPECT_EQ(entry.type, rsp::MetadataType::INT16);
  EXPECT_EQ(ReadBack<int16_t>(entry), -500);
}

TEST(MakeScopeMetadata, Uint32) {
  auto entry = rsp::MakeScopeMetadata<uint32_t>(rsp::MetadataTag{"u32"}, 100000u);
  EXPECT_EQ(entry.type, rsp::MetadataType::UINT32);
  EXPECT_EQ(ReadBack<uint32_t>(entry), 100000u);
}

TEST(MakeScopeMetadata, Int32) {
  auto entry = rsp::MakeScopeMetadata<int32_t>(rsp::MetadataTag{"i32"}, -100000);
  EXPECT_EQ(entry.type, rsp::MetadataType::INT32);
  EXPECT_EQ(ReadBack<int32_t>(entry), -100000);
}

TEST(MakeScopeMetadata, Uint64) {
  uint64_t big = 0xDEADBEEFCAFEBABEull;
  auto entry   = rsp::MakeScopeMetadata<uint64_t>(rsp::MetadataTag{"u64"}, big);
  EXPECT_EQ(entry.type, rsp::MetadataType::UINT64);
  EXPECT_EQ(ReadBack<uint64_t>(entry), big);
}

TEST(MakeScopeMetadata, Int64) {
  int64_t val = -9'000'000'000'000LL;
  auto entry  = rsp::MakeScopeMetadata<int64_t>(rsp::MetadataTag{"i64"}, val);
  EXPECT_EQ(entry.type, rsp::MetadataType::INT64);
  EXPECT_EQ(ReadBack<int64_t>(entry), val);
}

TEST(MakeScopeMetadata, Float) {
  auto entry = rsp::MakeScopeMetadata<float>(rsp::MetadataTag{"f32"}, 3.14f);
  EXPECT_EQ(entry.type, rsp::MetadataType::FLOAT);
  EXPECT_FLOAT_EQ(ReadBack<float>(entry), 3.14f);
}

TEST(MakeScopeMetadata, Double) {
  auto entry = rsp::MakeScopeMetadata<double>(rsp::MetadataTag{"f64"}, 2.71828);
  EXPECT_EQ(entry.type, rsp::MetadataType::DOUBLE);
  EXPECT_DOUBLE_EQ(ReadBack<double>(entry), 2.71828);
}

// --- Boundary values ---

TEST(MakeScopeMetadata, Uint8Max) {
  auto entry = rsp::MakeScopeMetadata<uint8_t>(rsp::MetadataTag{"max"}, std::numeric_limits<uint8_t>::max());
  EXPECT_EQ(ReadBack<uint8_t>(entry), 255);
}

TEST(MakeScopeMetadata, Uint8Zero) {
  auto entry = rsp::MakeScopeMetadata<uint8_t>(rsp::MetadataTag{"zero"}, 0);
  EXPECT_EQ(ReadBack<uint8_t>(entry), 0);
}

TEST(MakeScopeMetadata, Int8Min) {
  auto entry = rsp::MakeScopeMetadata<int8_t>(rsp::MetadataTag{"min"}, std::numeric_limits<int8_t>::min());
  EXPECT_EQ(ReadBack<int8_t>(entry), -128);
}

TEST(MakeScopeMetadata, Int8Max) {
  auto entry = rsp::MakeScopeMetadata<int8_t>(rsp::MetadataTag{"max"}, std::numeric_limits<int8_t>::max());
  EXPECT_EQ(ReadBack<int8_t>(entry), 127);
}

TEST(MakeScopeMetadata, Uint64Max) {
  auto entry =
      rsp::MakeScopeMetadata<uint64_t>(rsp::MetadataTag{"max"}, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(ReadBack<uint64_t>(entry), std::numeric_limits<uint64_t>::max());
}

TEST(MakeScopeMetadata, Int64Min) {
  auto entry = rsp::MakeScopeMetadata<int64_t>(rsp::MetadataTag{"min"}, std::numeric_limits<int64_t>::min());
  EXPECT_EQ(ReadBack<int64_t>(entry), std::numeric_limits<int64_t>::min());
}

TEST(MakeScopeMetadata, FloatNegative) {
  auto entry = rsp::MakeScopeMetadata<float>(rsp::MetadataTag{"neg"}, -1.5f);
  EXPECT_FLOAT_EQ(ReadBack<float>(entry), -1.5f);
}

TEST(MakeScopeMetadata, DoubleVerySmall) {
  double tiny = 1e-300;
  auto entry  = rsp::MakeScopeMetadata<double>(rsp::MetadataTag{"tiny"}, tiny);
  EXPECT_DOUBLE_EQ(ReadBack<double>(entry), tiny);
}

TEST(MakeScopeMetadata, FloatInfinity) {
  auto entry =
      rsp::MakeScopeMetadata<float>(rsp::MetadataTag{"inf"}, std::numeric_limits<float>::infinity());
  EXPECT_EQ(ReadBack<float>(entry), std::numeric_limits<float>::infinity());
}

// --- Bool conversion ---

TEST(MakeScopeMetadata, BoolTrue) {
  auto entry = rsp::MakeScopeMetadata(rsp::MetadataTag{"flag"}, true);
  EXPECT_EQ(entry.type, rsp::MetadataType::UINT8);
  EXPECT_EQ(ReadBack<uint8_t>(entry), 1);
}

TEST(MakeScopeMetadata, BoolFalse) {
  auto entry = rsp::MakeScopeMetadata(rsp::MetadataTag{"flag"}, false);
  EXPECT_EQ(entry.type, rsp::MetadataType::UINT8);
  EXPECT_EQ(ReadBack<uint8_t>(entry), 0);
}

// --- Enum conversion ---

enum class Color : uint8_t { RED = 0, GREEN = 1, BLUE = 2 };

TEST(MakeScopeMetadata, EnumUint8Underlying) {
  auto entry = rsp::MakeScopeMetadata(rsp::MetadataTag{"color"}, Color::BLUE);
  EXPECT_EQ(entry.type, rsp::MetadataType::UINT8);
  EXPECT_EQ(ReadBack<uint8_t>(entry), 2);
}

enum class Priority : int32_t { LOW = -1, NORMAL = 0, HIGH = 1 };

TEST(MakeScopeMetadata, EnumInt32Underlying) {
  auto entry = rsp::MakeScopeMetadata(rsp::MetadataTag{"prio"}, Priority::LOW);
  EXPECT_EQ(entry.type, rsp::MetadataType::INT32);
  EXPECT_EQ(ReadBack<int32_t>(entry), -1);
}

// --- size_t (platform integral) ---

TEST(MakeScopeMetadata, SizeT) {
  size_t val = 42;
  auto entry = rsp::MakeScopeMetadata(rsp::MetadataTag{"sz"}, val);
  // size_t is an unsigned 8-byte type on 64-bit
  EXPECT_EQ(entry.type, rsp::MetadataType::UINT64);
  EXPECT_EQ(ReadBack<uint64_t>(entry), 42u);
}

// --- MetadataTypeToString ---

TEST(MetadataTypeToString, AllTypes) {
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::UNSET), "UNSET");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::INT8), "INT8");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::UINT8), "UINT8");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::INT16), "INT16");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::UINT16), "UINT16");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::INT32), "INT32");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::UINT32), "UINT32");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::INT64), "INT64");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::UINT64), "UINT64");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::DOUBLE), "DOUBLE");
  EXPECT_STREQ(rsp::MetadataTypeToString(rsp::MetadataType::FLOAT), "FLOAT");
}

// --- Streaming operator ---

TEST(MetadataEntryStream, Int32Value) {
  auto entry = rsp::MakeScopeMetadata<int32_t>(rsp::MetadataTag{"count"}, 99);
  std::ostringstream oss;
  oss << entry;
  std::string out = oss.str();
  EXPECT_NE(out.find("count"), std::string::npos);
  EXPECT_NE(out.find("INT32"), std::string::npos);
  EXPECT_NE(out.find("99"), std::string::npos);
}

TEST(MetadataEntryStream, UnsetValue) {
  rsp::MetadataEntry entry;
  std::ostringstream oss;
  oss << entry;
  std::string out = oss.str();
  EXPECT_NE(out.find("(unset)"), std::string::npos);
}

TEST(MetadataEntryStream, DoubleValue) {
  auto entry = rsp::MakeScopeMetadata<double>(rsp::MetadataTag{"rate"}, 1.5);
  std::ostringstream oss;
  oss << entry;
  std::string out = oss.str();
  EXPECT_NE(out.find("rate"), std::string::npos);
  EXPECT_NE(out.find("DOUBLE"), std::string::npos);
}
