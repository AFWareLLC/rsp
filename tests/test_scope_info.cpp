#include <gtest/gtest.h>

#include <afware/rsp/Scope.hpp>

#include <cstring>
#include <sstream>

// --- ScopeInfo construction ---

TEST(ScopeInfo, ConstructionWithTag) {
  rsp::ScopeInfo info{rsp::ScopeTag{"TestScope"}};
  EXPECT_STREQ(info.tag.c_str(), "TestScope");
  EXPECT_EQ(info.metadata_ptr, nullptr);
}

TEST(ScopeInfo, BlankFactory) {
  auto info = rsp::ScopeInfo::Blank();
  EXPECT_STREQ(info.tag.c_str(), "DEFAULT");
  EXPECT_EQ(info.metadata_ptr, nullptr);
}

TEST(ScopeInfo, CopyConstruction) {
  rsp::ScopeInfo a{rsp::ScopeTag{"Original"}};
  a.ticks_start = 100;
  a.ticks_end   = 200;

  rsp::ScopeInfo b = a;
  EXPECT_STREQ(b.tag.c_str(), "Original");
  EXPECT_EQ(b.ticks_start, 100u);
  EXPECT_EQ(b.ticks_end, 200u);
}

TEST(ScopeInfo, CopyAssignment) {
  rsp::ScopeInfo a{rsp::ScopeTag{"A"}};
  rsp::ScopeInfo b{rsp::ScopeTag{"B"}};
  a.ticks_start = 10;
  a.ticks_end   = 20;

  b = a;
  EXPECT_STREQ(b.tag.c_str(), "A");
  EXPECT_EQ(b.ticks_start, 10u);
}

// --- AddMetadata ---

TEST(ScopeInfo, AddMetadataWithSlot) {
  rsp::MetadataSlot slot;
  rsp::ScopeInfo info{rsp::ScopeTag{"S"}};
  info.metadata_ptr = &slot;

  info.AddMetadata<int32_t>(rsp::MetadataTag{"count"}, 42);
  EXPECT_EQ(slot.metadata_idx, 1);
  EXPECT_EQ(slot.metadata[0].type, rsp::MetadataType::INT32);
}

TEST(ScopeInfo, AddMetadataMultiple) {
  rsp::MetadataSlot slot;
  rsp::ScopeInfo info{rsp::ScopeTag{"S"}};
  info.metadata_ptr = &slot;

  info.AddMetadata<int32_t>(rsp::MetadataTag{"a"}, 1);
  info.AddMetadata<double>(rsp::MetadataTag{"b"}, 2.0);
  info.AddMetadata<uint8_t>(rsp::MetadataTag{"c"}, 3);

  EXPECT_EQ(slot.metadata_idx, 3);
}

TEST(ScopeInfo, AddMetadataWithoutSlotThrows) {
  rsp::ScopeInfo info{rsp::ScopeTag{"S"}};
  // metadata_ptr is null
  EXPECT_THROW(info.AddMetadata<int32_t>(rsp::MetadataTag{"x"}, 1), std::runtime_error);
}

// --- Streaming operator ---

TEST(ScopeInfo, StreamOutput) {
  rsp::MetadataSlot slot;
  rsp::ScopeInfo info{rsp::ScopeTag{"MyScope"}};
  info.metadata_ptr = &slot;
  info.ticks_start  = 1000;
  info.ticks_end    = 2000;
  info.AddMetadata<int32_t>(rsp::MetadataTag{"items"}, 5);

  std::ostringstream oss;
  oss << info;
  std::string out = oss.str();

  EXPECT_NE(out.find("MyScope"), std::string::npos);
  EXPECT_NE(out.find("1000"), std::string::npos);
  EXPECT_NE(out.find("2000"), std::string::npos);
  EXPECT_NE(out.find("items"), std::string::npos);
}

// --- ScopeTag ---

TEST(ScopeTag, FitsDefaultSize) {
  rsp::ScopeTag tag{"ShortName"};
  EXPECT_STREQ(tag.c_str(), "ShortName");
}

TEST(ScopeTag, Truncation) {
  // Default RSP_SCOPE_TAG_SIZE is 32, so max 31 chars
  rsp::ScopeTag tag{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghij"};
  EXPECT_EQ(tag.size, 31u);
}
