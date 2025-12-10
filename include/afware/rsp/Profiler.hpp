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
#include "Machine.hpp"
#include "Macros.hpp"
#include "Scope.hpp"
#include "Slots.hpp"
#include "Sinks.hpp"
#include "Queue.hpp"

#include <array>
#include <atomic>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace rsp {

//
// This describes the number of active slots that you would
// expect to be active at any one time.
//
// This is needed for storage preallocation and is not
// runtime configurable.
//

#if !defined(RSP_PROFILER_DEFAULT_STORAGE_SLOTS)
#define RSP_PROFILER_DEFAULT_STORAGE_SLOTS 1024
#endif

//
// This controls how long we wait/block for on item de-queue.
//
#if !defined(RSP_PROFILER_DEQUEUE_WAIT_MS)
#define RSP_PROFILER_DEQUEUE_WAIT_MS 10
#endif

using SlotStorage = MetadataSlotStorage<RSP_PROFILER_DEFAULT_STORAGE_SLOTS>;

//
// The profiler is a Singleton that is really just a resource
// manager and aggregator. All of the scope-specific information
// is aggregated here and will get "sunk" to whatever output
// format the user specifics.
//
// The profiler runs its own aggregation thread to handle
// serialization/output of profiler statistics. The entire pipeline is
// entirely lock-free, but thread safe (moodycamel::ConcurrentQueue is
// heavily used)
//
// There are a number of configurable sinks through which the
// data can be aggregated.
//

class Profiler;
Profiler &RSPInstance();

class Profiler {
  using SinkFunc      = std::function<void(const ScopeInfo &)>;
  using ProfilerQueue = moodycamel::BlockingConcurrentQueue<ScopeInfo>;

public:
  Profiler(const Profiler &)            = delete;
  Profiler &operator=(const Profiler &) = delete;
  Profiler(Profiler &&)                 = delete;
  Profiler &operator=(Profiler &&)      = delete;

  bool Ready() const {
    return machine_.OK();
  }

  bool Start() {
    stop_ = false;
    if (!Ready()) {
      return false;
    }

    this->StartSinkThread();
    return true;
  }

  void Stop() {
    stop_ = true;
  }

  void Add(ScopeInfo scope_info) {
    queue_.enqueue(std::move(scope_info));
  }

  //
  // We deal with sinks this way - all explicit and gross-like - to avoid
  // inheritience/virtual function overhead.
  //

  void SetSinkToSilent() {
    sink_ = [&](const ScopeInfo &info) { (void)info; };

    sink_type_ = SinkType::SILENT;
  }

  void SetSinkToCout() {
    sink_ = [&](const ScopeInfo &info) { std::cout << info << "\n"; };

    sink_type_ = SinkType::COUT;
  }

  void SetSinkToBinaryDisk(std::shared_ptr<BinaryDiskSink> sink_ptr) {
    if (!sink_ptr || !sink_ptr->OK()) {
      throw std::runtime_error("Could not set up BinaryDiskSink.");  // TODO(ajf): exception type?
    }

    sink_ = [sink_ptr](const ScopeInfo &info) { sink_ptr->Sink(info); };

    sink_type_ = SinkType::BINARY_DISK;
  }

  SinkType GetSinkType() const {
    return sink_type_;
  }

  static std::shared_ptr<BinaryDiskSink> CreateBinaryDiskSink(const std::filesystem::path &path) {
    return std::make_shared<BinaryDiskSink>(path, RSPInstance().GetMachine());
  }

  SlotStorage *GetSlotStorage() {
    return &slot_storage_;
  }

  Machine *GetMachine() {
    return &machine_;
  }

private:
  Profiler() : machine_(Machine()), slot_storage_{} {
    SetSinkToSilent();
  }

  ~Profiler() {
    StopSinkThread();
  }

  void StartSinkThread() {
    stop_ = false;

    sink_thread_ = std::thread([this]() {
      while (!stop_) {
        ScopeInfo info = ScopeInfo::Blank();
        if (queue_.wait_dequeue_timed(info, std::chrono::milliseconds(RSP_PROFILER_DEQUEUE_WAIT_MS))) {
          sink_(info);
          GetSlotStorage()->Release(info.metadata_ptr);
        }
      }

      ScopeInfo info = ScopeInfo::Blank();
      while (queue_.try_dequeue(info)) {
        sink_(info);
        GetSlotStorage()->Release(info.metadata_ptr);
        info = ScopeInfo::Blank();
      }
    });
  }

  void StopSinkThread() {
    stop_ = true;
    if (sink_thread_.joinable()) {
      sink_thread_.join();
    }
  }

  //
  // Machine abstraction.
  //

  Machine machine_;

  //
  //
  //
  SlotStorage slot_storage_;

  SinkFunc sink_;
  SinkType sink_type_;

  //
  // Queue for holding finalized scope info.
  //

  ProfilerQueue queue_;

  //
  // Thread control.
  //

  std::thread sink_thread_;
  std::atomic<bool> stop_ = false;

  friend Profiler &RSPInstance();
};

//
// Meyer-style singleton - the profiler
// will manage scope output from ALL threads.
//

inline Profiler &RSPInstance() {
  static Profiler instance;
  return instance;
}

//
// Scope management.
//
// Scopes are thread local, since we support nested scoping.
// They all eventually get aggregated up to the profiler instance
// (and request resources managed by it).
//

class ActiveScope;

//
// The ScopeManager is basically just a stack of open scopes for the thread.
// This should never reallocate unless you're
// doing crazy things.
//
// The #define below lets you control that expectation. It's not a hard-maximum,
// rather just a high-water mark. If you nest scopes multiple levels deep, you
// certainly could exceed this and incur a reallocation.
//
// The main functionality this provides is the ability for us to associate
// metadata to the appropriate scope.
//

#if !defined(RSP_MAX_ACTIVE_SCOPES_PER_THREAD)
#define RSP_MAX_ACTIVE_SCOPES_PER_THREAD 32
#endif

class ScopeManager {
public:
  ScopeManager() : scopes_({}) {
    scopes_.reserve(RSP_MAX_ACTIVE_SCOPES_PER_THREAD);
  }

  void Push(ActiveScope *scope) {
    scopes_.push_back(scope);
  }

  void Pop() {
    scopes_.pop_back();
  }

  ActiveScope *Current() {
    if (scopes_.empty()) {
      return nullptr;
    } else {
      return scopes_[scopes_.size() - 1];
    }
  }

private:
  std::vector<ActiveScope *> scopes_;
};

//
// Returns the thread-local scope manager.
//

inline ScopeManager *GetScopeManager() {
  thread_local ScopeManager mgr;
  return &mgr;
}

//
// An ActiveScope is what gets instantiated by the profiling macros.
// It's responsible for collecting timing info (on construction and destruction),
// instantiating the scope metadata storage and informing the ScopeManager about itself.
//
// This breaks tonnes of OOP rules, but it's a profiler - they're never pretty.
//

class ActiveScope {
public:
  //
  // Each scope is instantiate with a tag which must be known at compile time.
  // The start time is collected upon construction, but we are careful to measure
  // only after we've set ourselves up to keep our operations out of the timing scope.
  //
  ActiveScope(const char *name) : info(name) {
    info.metadata_ptr = RSPInstance().GetSlotStorage()->Acquire();
    GetScopeManager()->Push(this);

    info.ticks_start = Now();
  }

  //
  // On destruction, we remove ourselves from the scope and then pass our
  // collected stats up to the profiler instance.
  //

  ~ActiveScope() {
    info.ticks_end = Now();
    RSPInstance().Add(info);
    GetScopeManager()->Pop();
  }

  ScopeInfo info;
};

}  // namespace rsp
