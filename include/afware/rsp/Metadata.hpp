// Copyright © 2025, AFWare LLC <ajf@afware.io>
//
// Permission to use, copy, modify, and/or distribute this software
// for any purpose with or without fee is hereby granted, provided
// that the above copyright notice and this permission notice appear
// in all copies.
//
// THE SOFTWARE IS PROVIDED “AS IS” AND ISC DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
// DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
// ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.

#pragma once

#include "ConstexprString.hpp"

#include <array>
#include <cstring>

namespace rsp {

//
// Scope metadata.
//
// Each scope can be bundled with metadata (for example: how many items are in the vector
// we are processing?
//
// The following assumptions are true:
//
// - Each metadata has a tag of no more than 32 bytes (using a constexpr string to avoid allocation)
// - Each metadata value can be one of the common types below - restricting to 8 bytes maximum.
//

#if !defined(RSP_SCOPE_METADATA_TAG_SIZE)
#define RSP_SCOPE_METADATA_TAG_SIZE 32
#endif

using MetadataTag = ConstexprString<RSP_SCOPE_METADATA_TAG_SIZE>;

enum class MetadataType : uint8_t {
  UNSET,
  INT8,
  UINT8,
  INT16,
  UINT16,
  INT32,
  UINT32,
  INT64,
  UINT64,  // NOTE: this is the same as size_t
  DOUBLE,
  FLOAT,
};

struct MetadataEntry {
  MetadataTag tag;
  MetadataType type = MetadataType::UNSET;

  //
  // The maximum size for the payload here will be 8 bytes -> double / 64 bit int types
  //
  static constexpr std::size_t MAX_METADATA_DATA_SIZE_BYTES = 8;

  std::array<std::byte, MAX_METADATA_DATA_SIZE_BYTES> data = {};

  constexpr MetadataEntry(MetadataTag tag, MetadataType type) : tag(tag), type(type) {
  }

  constexpr MetadataEntry() : tag(MetadataTag{"NOT SET"}), type(MetadataType::UNSET) {
  }
};

template <typename T>
inline MetadataEntry MakeScopeMetadata(MetadataTag tag, T val);

template <>
inline MetadataEntry MakeScopeMetadata(MetadataTag tag, uint8_t val) {
  MetadataEntry entry{tag, MetadataType::UINT8};
  std::memcpy(entry.data.data(), &val, sizeof(uint8_t));
  return entry;
}

template <>
inline MetadataEntry MakeScopeMetadata<int8_t>(MetadataTag tag, int8_t val) {
  MetadataEntry entry{tag, MetadataType::INT8};
  std::memcpy(entry.data.data(), &val, sizeof(int8_t));
  return entry;
}

template <>
inline MetadataEntry MakeScopeMetadata<uint16_t>(MetadataTag tag, uint16_t val) {
  MetadataEntry entry{tag, MetadataType::UINT16};
  std::memcpy(entry.data.data(), &val, sizeof(uint16_t));
  return entry;
}

template <>
inline MetadataEntry MakeScopeMetadata<int16_t>(MetadataTag tag, int16_t val) {
  MetadataEntry entry{tag, MetadataType::INT16};
  std::memcpy(entry.data.data(), &val, sizeof(int16_t));
  return entry;
}

template <>
inline MetadataEntry MakeScopeMetadata<uint32_t>(MetadataTag tag, uint32_t val) {
  MetadataEntry entry{tag, MetadataType::UINT32};
  std::memcpy(entry.data.data(), &val, sizeof(uint32_t));
  return entry;
}

template <>
inline MetadataEntry MakeScopeMetadata<int32_t>(MetadataTag tag, int32_t val) {
  MetadataEntry entry{tag, MetadataType::INT32};
  std::memcpy(entry.data.data(), &val, sizeof(int32_t));
  return entry;
}



template <>
inline MetadataEntry MakeScopeMetadata<uint64_t>(MetadataTag tag, uint64_t val) {
  MetadataEntry entry{tag, MetadataType::UINT64};
  std::memcpy(entry.data.data(), &val, sizeof(uint64_t));
  return entry;
}

template <>
inline MetadataEntry MakeScopeMetadata<unsigned long>(MetadataTag tag, unsigned long val) {
  static_assert(sizeof(unsigned long) == sizeof(uint64_t));
  return MakeScopeMetadata<uint64_t>(tag, static_cast<uint64_t>(val));
}

template <>
inline MetadataEntry MakeScopeMetadata<int64_t>(MetadataTag tag, int64_t val) {
  MetadataEntry entry{tag, MetadataType::INT64};
  std::memcpy(entry.data.data(), &val, sizeof(int64_t));
  return entry;
}

template <>
inline MetadataEntry MakeScopeMetadata<float>(MetadataTag tag, float val) {
  MetadataEntry entry{tag, MetadataType::FLOAT};
  std::memcpy(entry.data.data(), &val, sizeof(float));
  return entry;
}

template <>
inline MetadataEntry MakeScopeMetadata<double>(MetadataTag tag, double val) {
  MetadataEntry entry{tag, MetadataType::DOUBLE};
  std::memcpy(entry.data.data(), &val, sizeof(double));
  return entry;
}

}  // namespace rsp
