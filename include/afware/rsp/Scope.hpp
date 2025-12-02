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
#include "Metadata.hpp"
#include "Slots.hpp"

#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace rsp {

//
// Each scope will generate a ScopeInfo.
//
// In this form, the information is *not* serializable as it not self contained.
// For efficiency, we use a quasi-arena-bump-allocator to hold storage slots for
// a set number of scopes' metadata - this is to avoid allocation of the metadata storage specific
// specifically. The metadata itself is sized bounded, and to make serialization/management easier
// we are using a std::array, so they are of fixed size.
//
// The storage (and it's allocation to a particular scope) is managed in other parts of the pipeline.
//

#if !defined(RSP_SCOPE_TAG_SIZE)
#define RSP_SCOPE_TAG_SIZE 32
#endif

using ScopeTag = ConstexprString<RSP_SCOPE_TAG_SIZE>;

struct ScopeInfo {
  ScopeTag tag;

  uint64_t ticks_start;
  uint64_t ticks_end;

  MetadataSlot* metadata_ptr = nullptr;

  constexpr ScopeInfo(ScopeTag t) : tag(t) {
  }

  template <typename T>
  void AddMetadata(MetadataTag tag, T val) {
    if (!metadata_ptr) {
      throw std::runtime_error("No metadata slot allotted.");
    }

    metadata_ptr->template AddMetadata<T>(tag, val);
  }

  static ScopeInfo Blank() {
    return ScopeInfo{ScopeTag{"DEFAULT"}};
  }

  ScopeInfo(const ScopeInfo&)            = default;
  ScopeInfo& operator=(const ScopeInfo&) = default;
};

//
// Streaming operators/helpers.
//

inline const char* MetadataTypeToString(MetadataType type) {
  switch (type) {
    case MetadataType::UNSET:
      return "UNSET";
    case MetadataType::INT8:
      return "INT8";
    case MetadataType::UINT8:
      return "UINT8";
    case MetadataType::INT16:
      return "INT16";
    case MetadataType::UINT16:
      return "UINT16";
    case MetadataType::INT32:
      return "INT32";
    case MetadataType::UINT32:
      return "UINT32";
    case MetadataType::INT64:
      return "INT64";
    case MetadataType::UINT64:
      return "UINT64";
    case MetadataType::DOUBLE:
      return "DOUBLE";
    case MetadataType::FLOAT:
      return "FLOAT";
    default:
      return "UNKNOWN";
  }
}

inline std::ostream& operator<<(std::ostream& os, const MetadataEntry& m) {
  os << m.tag.c_str() << "=" << MetadataTypeToString(m.type) << ": ";
  switch (m.type) {
    case MetadataType::INT8:
      os << +(*reinterpret_cast<const int8_t*>(m.data.data()));
      break;
    case MetadataType::UINT8:
      os << +(*reinterpret_cast<const uint8_t*>(m.data.data()));
      break;
    case MetadataType::INT16:
      os << *reinterpret_cast<const int16_t*>(m.data.data());
      break;
    case MetadataType::UINT16:
      os << *reinterpret_cast<const uint16_t*>(m.data.data());
      break;
    case MetadataType::INT32:
      os << *reinterpret_cast<const int32_t*>(m.data.data());
      break;
    case MetadataType::UINT32:
      os << *reinterpret_cast<const uint32_t*>(m.data.data());
      break;
    case MetadataType::INT64:
      os << *reinterpret_cast<const int64_t*>(m.data.data());
      break;
    case MetadataType::UINT64:
      os << *reinterpret_cast<const uint64_t*>(m.data.data());
      break;
    case MetadataType::FLOAT:
      os << *reinterpret_cast<const float*>(m.data.data());
      break;
    case MetadataType::DOUBLE:
      os << *reinterpret_cast<const double*>(m.data.data());
      break;
    default:
      os << "(unset)";
      break;
  }
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const ScopeInfo& s) {
  os << "Scope[" << s.tag.c_str() << "] "
     << "ticks_start=" << s.ticks_start << " ticks_end=" << s.ticks_end << " metadata={";
  bool first = true;
  for (const auto& m : s.metadata_ptr->metadata) {
    if (m.type != MetadataType::UNSET) {
      if (!first) os << ", ";
      os << m;
      first = false;
    }
  }
  os << "}";
  return os;
}

}  // namespace rsp
