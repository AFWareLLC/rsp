#include <gtest/gtest.h>

#include <afware/rsp/API.hpp>

#include <atomic>
#include <thread>
#include <vector>

// --- Concurrent ActiveScope creation ---

TEST(Threading, ConcurrentScopeCreation) {
  constexpr int num_threads  = 8;
  constexpr int scopes_each  = 100;
  std::atomic<int> completed = 0;

  auto worker = [&]() {
    for (int i = 0; i < scopes_each; ++i) {
      RSP_SCOPE("ThreadScope");
      RSP_SCOPE_METADATA("iter", static_cast<int32_t>(i));
      // Small work
      volatile int x = 0;
      for (int j = 0; j < 10; ++j) x += j;
      (void)x;
    }
    completed.fetch_add(1);
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }
  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(completed.load(), num_threads);
}

// --- Thread-local scope manager isolation ---

TEST(Threading, ScopeManagerIsolation) {
  // Each thread's scope nesting is independent
  constexpr int num_threads = 4;
  std::atomic<bool> all_ok  = true;

  auto worker = [&]() {
    EXPECT_EQ(rsp::GetScopeManager()->Current(), nullptr);

    {
      RSP_SCOPE("ThreadOuter");
      auto *outer = rsp::GetScopeManager()->Current();
      if (!outer) {
        all_ok.store(false);
        return;
      }

      {
        RSP_SCOPE("ThreadInner");
        auto *inner = rsp::GetScopeManager()->Current();
        if (inner == outer || !inner) {
          all_ok.store(false);
          return;
        }
      }

      if (rsp::GetScopeManager()->Current() != outer) {
        all_ok.store(false);
      }
    }

    if (rsp::GetScopeManager()->Current() != nullptr) {
      all_ok.store(false);
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }
  for (auto &t : threads) {
    t.join();
  }

  EXPECT_TRUE(all_ok.load());
}

// --- Concurrent nested scopes ---

TEST(Threading, ConcurrentNestedScopes) {
  constexpr int num_threads = 4;
  constexpr int depth       = 5;
  std::atomic<int> completed = 0;

  auto worker = [&]() {
    // Create nested scopes recursively via iteration
    std::vector<std::unique_ptr<rsp::ActiveScope>> scopes;
    for (int d = 0; d < depth; ++d) {
      scopes.push_back(std::make_unique<rsp::ActiveScope>("Nested"));
    }
    // Destroy in reverse
    while (!scopes.empty()) {
      scopes.pop_back();
    }
    completed.fetch_add(1);
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }
  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(completed.load(), num_threads);
}

// --- High volume stress test ---

TEST(Threading, StressTest) {
  constexpr int num_threads  = 4;
  constexpr int scopes_each  = 1000;
  std::atomic<int> total     = 0;

  auto worker = [&]() {
    for (int i = 0; i < scopes_each; ++i) {
      rsp::ActiveScope scope("Stress");
      scope.info.AddMetadata<uint32_t>(rsp::MetadataTag{"i"}, static_cast<uint32_t>(i));
      total.fetch_add(1);
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }
  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(total.load(), num_threads * scopes_each);
}

// --- Concurrent slot storage usage ---

TEST(Threading, ConcurrentSlotStorageFromProfiler) {
  auto *storage = rsp::Instance().GetSlotStorage();
  constexpr int num_threads = 8;
  constexpr int ops         = 50;

  auto worker = [&]() {
    for (int i = 0; i < ops; ++i) {
      auto *slot = storage->Acquire();
      EXPECT_NE(slot, nullptr);
      slot->AddMetadata<int32_t>(rsp::MetadataTag{"w"}, i);
      storage->Release(slot);
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }
  for (auto &t : threads) {
    t.join();
  }
}

// --- Metadata correctness under concurrency ---

TEST(Threading, MetadataCorrectness) {
  // Verify each thread's metadata is isolated and correct
  constexpr int num_threads = 4;
  std::atomic<bool> all_ok  = true;

  auto worker = [&](int thread_id) {
    for (int i = 0; i < 50; ++i) {
      rsp::ActiveScope scope("MetaCheck");
      int32_t expected = thread_id * 1000 + i;
      scope.info.AddMetadata<int32_t>(rsp::MetadataTag{"id"}, expected);

      // Verify the metadata was stored correctly
      int32_t stored;
      std::memcpy(&stored, scope.info.metadata_ptr->metadata[0].data.data(), sizeof(int32_t));
      if (stored != expected) {
        all_ok.store(false);
      }
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker, i);
  }
  for (auto &t : threads) {
    t.join();
  }

  EXPECT_TRUE(all_ok.load());
}

// --- End-to-end concurrent writes to disk ---

TEST(Threading, ConcurrentDiskWrites) {
  auto path = std::filesystem::temp_directory_path() / "rsp_thread_disk_test.bin";
  std::filesystem::remove(path);
  rsp::Machine machine;

  constexpr int num_threads  = 4;
  constexpr int writes_each  = 25;

  {
    // Shared sink (BinaryDiskSink uses ofstream which is NOT thread-safe,
    // so we serialize writes through a mutex)
    std::mutex sink_mutex;
    rsp::BinaryDiskSink sink(path, &machine);

    auto writer = [&](int tid) {
      for (int i = 0; i < writes_each; ++i) {
        rsp::MetadataSlot slot;
        rsp::ScopeInfo info{rsp::ScopeTag{"ThreadWrite"}};
        info.metadata_ptr = &slot;
        info.ticks_start  = static_cast<uint64_t>(tid * 10000 + i);
        info.ticks_end    = static_cast<uint64_t>(tid * 10000 + i + 100);
        info.AddMetadata<int32_t>(rsp::MetadataTag{"tid"}, tid);

        std::lock_guard<std::mutex> lock(sink_mutex);
        sink.Sink(info);
      }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back(writer, i);
    }
    for (auto &t : threads) {
      t.join();
    }
  }

  // Read back and count entries
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
    EXPECT_STREQ(fb->tag()->c_str(), "ThreadWrite");
    ++count;
  }

  EXPECT_EQ(count, num_threads * writes_each);
  std::filesystem::remove(path);
}
