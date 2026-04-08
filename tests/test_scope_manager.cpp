#include <gtest/gtest.h>

#include <afware/rsp/Profiler.hpp>

#include <atomic>
#include <thread>

// --- ScopeManager basic operations ---

TEST(ScopeManager, CurrentReturnsNullWhenEmpty) {
  rsp::ScopeManager mgr;
  EXPECT_EQ(mgr.Current(), nullptr);
}

TEST(ScopeManager, PushAndCurrent) {
  rsp::ScopeManager mgr;
  // We don't dereference the pointer in Push/Pop/Current,
  // so we can use a dummy value for unit testing the stack.
  auto *dummy = reinterpret_cast<rsp::ActiveScope *>(0x1000);
  mgr.Push(dummy);
  EXPECT_EQ(mgr.Current(), dummy);
  mgr.Pop();
}

TEST(ScopeManager, PushMultiple) {
  rsp::ScopeManager mgr;
  auto *a = reinterpret_cast<rsp::ActiveScope *>(0x1000);
  auto *b = reinterpret_cast<rsp::ActiveScope *>(0x2000);
  auto *c = reinterpret_cast<rsp::ActiveScope *>(0x3000);

  mgr.Push(a);
  EXPECT_EQ(mgr.Current(), a);

  mgr.Push(b);
  EXPECT_EQ(mgr.Current(), b);

  mgr.Push(c);
  EXPECT_EQ(mgr.Current(), c);

  mgr.Pop();
  EXPECT_EQ(mgr.Current(), b);

  mgr.Pop();
  EXPECT_EQ(mgr.Current(), a);

  mgr.Pop();
  EXPECT_EQ(mgr.Current(), nullptr);
}

TEST(ScopeManager, PopAllReturnsNull) {
  rsp::ScopeManager mgr;
  auto *a = reinterpret_cast<rsp::ActiveScope *>(0x1000);
  mgr.Push(a);
  mgr.Pop();
  EXPECT_EQ(mgr.Current(), nullptr);
}

// --- Thread-local GetScopeManager ---

TEST(GetScopeManager, ReturnsNonNull) {
  EXPECT_NE(rsp::GetScopeManager(), nullptr);
}

TEST(GetScopeManager, SamePointerSameThread) {
  auto *a = rsp::GetScopeManager();
  auto *b = rsp::GetScopeManager();
  EXPECT_EQ(a, b);
}

TEST(GetScopeManager, DifferentPointersDifferentThreads) {
  rsp::ScopeManager *main_mgr = rsp::GetScopeManager();
  std::atomic<rsp::ScopeManager *> other_mgr{nullptr};

  std::thread t([&]() { other_mgr.store(rsp::GetScopeManager()); });
  t.join();

  EXPECT_NE(main_mgr, nullptr);
  EXPECT_NE(other_mgr.load(), nullptr);
  EXPECT_NE(main_mgr, other_mgr.load());
}

TEST(GetScopeManager, ThreadLocalIsolation) {
  // Push a scope in one thread; verify another thread's manager is unaffected
  auto *main_mgr = rsp::GetScopeManager();
  auto *dummy    = reinterpret_cast<rsp::ActiveScope *>(0x1000);
  main_mgr->Push(dummy);

  std::atomic<bool> other_empty{false};
  std::thread t([&]() { other_empty.store(rsp::GetScopeManager()->Current() == nullptr); });
  t.join();

  EXPECT_TRUE(other_empty.load());
  main_mgr->Pop();
}
