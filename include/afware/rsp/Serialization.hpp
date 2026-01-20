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

#include "Machine.hpp"
#include "Metadata.hpp"
#include "Scope.hpp"
#include "Slots.hpp"

#include "scope_info_generated.h"

#include <array>

#include <flatbuffers/flatbuffers.h>

namespace rsp {

inline flatbuffers::DetachedBuffer SerializeScopeInfo(const ScopeInfo *scope_info, Machine *machine) {
  flatbuffers::FlatBufferBuilder builder;

  auto tag_offset = builder.CreateString(scope_info->tag.c_str());

  std::vector<flatbuffers::Offset<RSP::MetadataEntry>> metadata_offsets;
  uint8_t max_offset = scope_info->metadata_ptr->metadata_idx;
  for (uint8_t i = 0; i < max_offset; ++i) {
    const auto &m = scope_info->metadata_ptr->metadata[i];

    uint64_t value = 0;
    std::memcpy(&value, m.data.data(), sizeof(uint64_t));

    auto m_tag   = builder.CreateString(m.tag.c_str());
    auto m_entry = RSP::CreateMetadataEntry(builder, m_tag, static_cast<RSP::MetadataType>(m.type), value);
    metadata_offsets.push_back(m_entry);
  }

  auto metadata_vector = builder.CreateVector(metadata_offsets);

  auto scope_fb = RSP::CreateScopeInfo(builder,
                                       tag_offset,
                                       scope_info->ticks_start,
                                       scope_info->ticks_end,
                                       machine->GetNominalFreq(),
                                       max_offset,
                                       max_offset,
                                       metadata_vector);

  builder.Finish(scope_fb);
  return builder.Release();
}

inline std::ostream &operator<<(std::ostream &os, const RSP::MetadataEntry &m) {
  os << "{tag=" << (m.tag() ? m.tag()->c_str() : "<null>") << ", type=" << static_cast<int>(m.type())
     << ", value=" << m.value() << "}";
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const RSP::ScopeInfo *scope) {
  if (!scope) return os;

  os << "Scope[" << (scope->tag() ? scope->tag()->c_str() : "<null>") << "] " << "ticks_start=" << scope->ticks_start()
     << " ticks_end=" << scope->ticks_end() << " machine_nominal_freq_hz=" << scope->machine_nominal_freq_hz()
     << " metadata={";

  bool first        = true;
  auto metadata_vec = scope->metadata();
  if (metadata_vec) {
    for (uint32_t i = 0; i < metadata_vec->size(); ++i) {
      const RSP::MetadataEntry *m_ptr = metadata_vec->Get(i);
      if (!m_ptr) continue;  // <--- check for nullptr
      if (!first) os << ", ";
      os << *m_ptr;  // safe dereference
      first = false;
    }
  }

  os << "}";
  return os;
}

}  // namespace rsp
