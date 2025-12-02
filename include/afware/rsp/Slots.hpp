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

#include "Metadata.hpp"
#include "Queue.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace rsp {

#if !defined(RSP_MAX_METADATA_ENTRIES)
#define RSP_MAX_METADATA_ENTRIES 8
#endif

struct MetadataSlot {
  uint8_t metadata_idx                                         = 0;
  std::array<MetadataEntry, RSP_MAX_METADATA_ENTRIES> metadata = {};

  void MakePristine() {
    for (uint8_t i = 0; i < metadata_idx; ++i) {
      metadata[i] = MetadataEntry();
    }
    metadata_idx = 0;
  }

  template <typename T>
  void AddMetadata(MetadataTag tag, T val) {
    metadata.at(metadata_idx++) = MakeScopeMetadata<T>(tag, val);
  }
};

template <size_t NumSlots>
class MetadataSlotStorage {
public:
  using Slot     = MetadataSlot;
  using FreeList = moodycamel::ConcurrentQueue<Slot *>;

  MetadataSlotStorage() {
    slots_.reserve(NumSlots);

    for (size_t i = 0; i < NumSlots; ++i) {
      slots_.emplace_back(std::make_unique<Slot>());
      free_list_.enqueue(slots_.back().get());
    }
  }

  Slot *Acquire() {
    //
    // We prefer to not allocate at runtime, so
    // we first check the free-list.
    //
    // If the free-list is empty, we then allocate
    // to expand the number of slots.
    //

    Slot *ret;

    if (free_list_.try_dequeue(ret)) {
      return ret;
    }

    //
    // If we got here, one of two things might be happening:
    // - Someone else is already calling Expand()
    // - We need to call Expand()
    //
    // However, just like among the immortals, there can be only one.
    // We want to grab the lock, try to dequeue. If that works - great.
    //
    // Otherwise, it's up to us to call Expand() and try again.
    //

    const std::scoped_lock lock{expansion_mutex_};
    if (free_list_.try_dequeue(ret)) {
      return ret;
    }

    this->Expand();

    //
    // Something is probably super duper wrong if we get here.
    //

    if (!free_list_.try_dequeue(ret)) {
      throw std::runtime_error("Could not get a free slot!");
    }

    return ret;
  }

  void Release(Slot *slot) {
    slot->MakePristine();
    free_list_.enqueue(slot);
  }

private:
  std::vector<std::unique_ptr<Slot>> slots_;
  FreeList free_list_;
  std::mutex expansion_mutex_;

  //
  // I couldn't think of a better strategy here.
  // If we run out of slots, we just add NumSlots
  // more. This should be very rare, ideally.
  //

  void Expand() {
    const size_t new_cap = slots_.capacity() + NumSlots;
    slots_.reserve(new_cap);

    for (size_t i = 0; i < NumSlots; ++i) {
      slots_.emplace_back(std::make_unique<Slot>());
      free_list_.enqueue(slots_.back().get());
    }
  }
};

}  // namespace rsp
