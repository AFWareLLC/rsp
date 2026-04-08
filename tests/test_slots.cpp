#include <gtest/gtest.h>

#include <afware/rsp/Slots.hpp>

#include <set>
#include <thread>
#include <vector>

// --- MetadataSlot ---

TEST(MetadataSlot, DefaultState) {
  rsp::MetadataSlot slot;
  EXPECT_EQ(slot.metadata_idx, 0);
}

TEST(MetadataSlot, AddMetadataIncrementsIndex) {
  rsp::MetadataSlot slot;
  slot.AddMetadata<int32_t>(rsp::MetadataTag{"a"}, 1);
  EXPECT_EQ(slot.metadata_idx, 1);
  slot.AddMetadata<int32_t>(rsp::MetadataTag{"b"}, 2);
  EXPECT_EQ(slot.metadata_idx, 2);
}

TEST(MetadataSlot, AddMetadataStoresCorrectValue) {
  rsp::MetadataSlot slot;
  slot.AddMetadata<uint32_t>(rsp::MetadataTag{"count"}, 42u);

  EXPECT_STREQ(slot.metadata[0].tag.c_str(), "count");
  EXPECT_EQ(slot.metadata[0].type, rsp::MetadataType::UINT32);

  uint32_t val;
  std::memcpy(&val, slot.metadata[0].data.data(), sizeof(uint32_t));
  EXPECT_EQ(val, 42u);
}

TEST(MetadataSlot, AddMultipleMetadata) {
  rsp::MetadataSlot slot;
  slot.AddMetadata<int32_t>(rsp::MetadataTag{"a"}, 10);
  slot.AddMetadata<double>(rsp::MetadataTag{"b"}, 3.14);
  slot.AddMetadata<uint8_t>(rsp::MetadataTag{"c"}, 255);

  EXPECT_EQ(slot.metadata_idx, 3);
  EXPECT_EQ(slot.metadata[0].type, rsp::MetadataType::INT32);
  EXPECT_EQ(slot.metadata[1].type, rsp::MetadataType::DOUBLE);
  EXPECT_EQ(slot.metadata[2].type, rsp::MetadataType::UINT8);
}

TEST(MetadataSlot, MakePristineResetsIndex) {
  rsp::MetadataSlot slot;
  slot.AddMetadata<int32_t>(rsp::MetadataTag{"x"}, 1);
  slot.AddMetadata<int32_t>(rsp::MetadataTag{"y"}, 2);
  EXPECT_EQ(slot.metadata_idx, 2);

  slot.MakePristine();
  EXPECT_EQ(slot.metadata_idx, 0);
}

TEST(MetadataSlot, MakePristineResetsEntries) {
  rsp::MetadataSlot slot;
  slot.AddMetadata<int32_t>(rsp::MetadataTag{"val"}, 999);
  slot.MakePristine();

  // After pristine, entries should be UNSET
  EXPECT_EQ(slot.metadata[0].type, rsp::MetadataType::UNSET);
}

TEST(MetadataSlot, FillToCapacity) {
  rsp::MetadataSlot slot;
  for (uint8_t i = 0; i < RSP_MAX_METADATA_ENTRIES; ++i) {
    slot.AddMetadata<uint8_t>(rsp::MetadataTag{"m"}, i);
  }
  EXPECT_EQ(slot.metadata_idx, RSP_MAX_METADATA_ENTRIES);
}

TEST(MetadataSlot, OverflowThrows) {
  rsp::MetadataSlot slot;
  for (uint8_t i = 0; i < RSP_MAX_METADATA_ENTRIES; ++i) {
    slot.AddMetadata<uint8_t>(rsp::MetadataTag{"m"}, i);
  }
  // One more should throw via std::array::at() bounds check
  EXPECT_THROW(slot.AddMetadata<uint8_t>(rsp::MetadataTag{"overflow"}, 0), std::out_of_range);
}

// --- MetadataSlotStorage ---

TEST(MetadataSlotStorage, AcquireReturnsNonNull) {
  rsp::MetadataSlotStorage<16> storage;
  auto *slot = storage.Acquire();
  EXPECT_NE(slot, nullptr);
  storage.Release(slot);
}

TEST(MetadataSlotStorage, AcquireReturnsUniquePointers) {
  rsp::MetadataSlotStorage<16> storage;
  std::set<rsp::MetadataSlot *> seen;

  for (int i = 0; i < 16; ++i) {
    auto *slot = storage.Acquire();
    EXPECT_TRUE(seen.insert(slot).second) << "Duplicate slot pointer at iteration " << i;
  }

  // Release all
  for (auto *s : seen) {
    storage.Release(s);
  }
}

TEST(MetadataSlotStorage, ReleaseAndReacquire) {
  rsp::MetadataSlotStorage<4> storage;
  auto *slot = storage.Acquire();
  slot->AddMetadata<int32_t>(rsp::MetadataTag{"x"}, 42);
  EXPECT_EQ(slot->metadata_idx, 1);

  storage.Release(slot);
  // After release, slot should be pristine when re-acquired
  // (Release calls MakePristine)
  auto *slot2 = storage.Acquire();
  EXPECT_EQ(slot2->metadata_idx, 0);
  storage.Release(slot2);
}

TEST(MetadataSlotStorage, ExpansionWhenExhausted) {
  rsp::MetadataSlotStorage<4> storage;
  std::vector<rsp::MetadataSlot *> slots;

  // Acquire all 4 initial slots
  for (int i = 0; i < 4; ++i) {
    slots.push_back(storage.Acquire());
  }

  // Acquire one more - should trigger expansion
  auto *extra = storage.Acquire();
  EXPECT_NE(extra, nullptr);
  slots.push_back(extra);

  for (auto *s : slots) {
    storage.Release(s);
  }
}

TEST(MetadataSlotStorage, LargeExpansion) {
  rsp::MetadataSlotStorage<2> storage;
  std::vector<rsp::MetadataSlot *> slots;

  // Acquire well beyond initial capacity
  for (int i = 0; i < 20; ++i) {
    auto *s = storage.Acquire();
    EXPECT_NE(s, nullptr);
    slots.push_back(s);
  }

  for (auto *s : slots) {
    storage.Release(s);
  }
}

TEST(MetadataSlotStorage, ConcurrentAcquireRelease) {
  rsp::MetadataSlotStorage<64> storage;
  constexpr int threads = 8;
  constexpr int ops_per_thread = 100;

  auto worker = [&]() {
    for (int i = 0; i < ops_per_thread; ++i) {
      auto *slot = storage.Acquire();
      EXPECT_NE(slot, nullptr);
      slot->AddMetadata<int32_t>(rsp::MetadataTag{"t"}, i);
      storage.Release(slot);
    }
  };

  std::vector<std::thread> ts;
  for (int i = 0; i < threads; ++i) {
    ts.emplace_back(worker);
  }
  for (auto &t : ts) {
    t.join();
  }
}
