#include <gtest/gtest.h>

#include <afware/rsp/ConstexprString.hpp>

#include <cstring>

// --- Basic construction ---

TEST(ConstexprString, BasicConstruction) {
  rsp::ConstexprString<32> s("hello");
  EXPECT_STREQ(s.c_str(), "hello");
  EXPECT_EQ(s.size, 5u);
}

TEST(ConstexprString, EmptyString) {
  rsp::ConstexprString<32> s("");
  EXPECT_STREQ(s.c_str(), "");
  EXPECT_EQ(s.size, 0u);
}

TEST(ConstexprString, SingleCharacter) {
  rsp::ConstexprString<32> s("X");
  EXPECT_STREQ(s.c_str(), "X");
  EXPECT_EQ(s.size, 1u);
}

// --- Truncation behavior ---

TEST(ConstexprString, TruncationAtMaxLength) {
  // MaxLength=4 means room for 3 chars + null terminator
  rsp::ConstexprString<4> s("abcdefgh");
  EXPECT_STREQ(s.c_str(), "abc");
  EXPECT_EQ(s.size, 3u);
}

TEST(ConstexprString, ExactFit) {
  // MaxLength=6 fits "hello" (5 chars + null)
  rsp::ConstexprString<6> s("hello");
  EXPECT_STREQ(s.c_str(), "hello");
  EXPECT_EQ(s.size, 5u);
}

TEST(ConstexprString, OneOverMaxLength) {
  // MaxLength=5 truncates "hello" to "hell"
  rsp::ConstexprString<5> s("hello");
  EXPECT_STREQ(s.c_str(), "hell");
  EXPECT_EQ(s.size, 4u);
}

TEST(ConstexprString, MinimalMaxLength) {
  // MaxLength=1 only fits null terminator
  rsp::ConstexprString<1> s("anything");
  EXPECT_STREQ(s.c_str(), "");
  EXPECT_EQ(s.size, 0u);
}

TEST(ConstexprString, MaxLengthTwo) {
  rsp::ConstexprString<2> s("XY");
  EXPECT_STREQ(s.c_str(), "X");
  EXPECT_EQ(s.size, 1u);
}

// --- Null termination ---

TEST(ConstexprString, AlwaysNullTerminated) {
  rsp::ConstexprString<4> s("abcdefghijklmnop");
  EXPECT_EQ(s.data[3], '\0');
}

TEST(ConstexprString, DataArrayZeroInitialized) {
  rsp::ConstexprString<16> s("ab");
  // Characters beyond the string should be zero
  for (size_t i = s.size + 1; i < 16; ++i) {
    EXPECT_EQ(s.data[i], '\0') << "Index " << i;
  }
}

// --- Constexpr usage ---

TEST(ConstexprString, ConstexprConstruction) {
  constexpr rsp::ConstexprString<16> s("constexpr");
  static_assert(s.size == 9);
  EXPECT_STREQ(s.c_str(), "constexpr");
}

// --- Special characters ---

TEST(ConstexprString, SpecialCharacters) {
  rsp::ConstexprString<32> s("hello world!");
  EXPECT_STREQ(s.c_str(), "hello world!");
  EXPECT_EQ(s.size, 12u);
}

TEST(ConstexprString, NumericString) {
  rsp::ConstexprString<32> s("12345");
  EXPECT_STREQ(s.c_str(), "12345");
  EXPECT_EQ(s.size, 5u);
}

// --- Different template sizes ---

TEST(ConstexprString, LargeMaxLength) {
  rsp::ConstexprString<256> s("short");
  EXPECT_STREQ(s.c_str(), "short");
  EXPECT_EQ(s.size, 5u);
}

TEST(ConstexprString, DefaultScopeTagSize) {
  // RSP_SCOPE_TAG_SIZE defaults to 32
  rsp::ConstexprString<32> s("MyScope");
  EXPECT_STREQ(s.c_str(), "MyScope");
}

TEST(ConstexprString, ScopeTagTruncation) {
  // A 32-byte ConstexprString fits 31 chars
  rsp::ConstexprString<32> s("abcdefghijklmnopqrstuvwxyz12345678");
  EXPECT_EQ(s.size, 31u);
  EXPECT_EQ(s.data[31], '\0');
}
