#include <gtest/gtest.h>

#include <afware/rsp/API.hpp>

#include <chrono>
#include <thread>

// --- ActiveScope basic behavior ---

TEST(ActiveScope, ConstructionAndDestruction) {
  // Should not crash or throw
  { rsp::ActiveScope scope("TestScope"); }
}

TEST(ActiveScope, HasMetadataSlot) {
  rsp::ActiveScope scope("TestScope");
  EXPECT_NE(scope.info.metadata_ptr, nullptr);
}

TEST(ActiveScope, TagIsSet) {
  rsp::ActiveScope scope("MyTag");
  EXPECT_STREQ(scope.info.tag.c_str(), "MyTag");
}

TEST(ActiveScope, StartTickIsSet) {
  rsp::ActiveScope scope("Timing");
  EXPECT_GT(scope.info.ticks_start, 0u);
}

TEST(ActiveScope, TimingEndGTStart) {
  uint64_t start, end;
  {
    rsp::ActiveScope scope("Timing");
    start = scope.info.ticks_start;
    // Do a tiny bit of work
    volatile int x = 0;
    for (int i = 0; i < 100; ++i) x += i;
    (void)x;
    end = rsp::Now();
  }
  EXPECT_GT(end, start);
}

TEST(ActiveScope, MetadataCanBeAdded) {
  rsp::ActiveScope scope("WithMeta");
  scope.info.AddMetadata<int32_t>(rsp::MetadataTag{"count"}, 42);
  EXPECT_EQ(scope.info.metadata_ptr->metadata_idx, 1);
}

TEST(ActiveScope, MultipleMetadata) {
  rsp::ActiveScope scope("Multi");
  scope.info.AddMetadata<int32_t>(rsp::MetadataTag{"a"}, 1);
  scope.info.AddMetadata<double>(rsp::MetadataTag{"b"}, 2.0);
  scope.info.AddMetadata<uint64_t>(rsp::MetadataTag{"c"}, 3);
  EXPECT_EQ(scope.info.metadata_ptr->metadata_idx, 3);
}

// --- Scope nesting ---

TEST(ActiveScope, NestingPushesOnScopeManager) {
  {
    rsp::ActiveScope outer("Outer");
    EXPECT_EQ(rsp::GetScopeManager()->Current(), &outer);

    {
      rsp::ActiveScope inner("Inner");
      EXPECT_EQ(rsp::GetScopeManager()->Current(), &inner);
    }

    // Inner destroyed, back to outer
    EXPECT_EQ(rsp::GetScopeManager()->Current(), &outer);
  }

  // Both destroyed
  EXPECT_EQ(rsp::GetScopeManager()->Current(), nullptr);
}

TEST(ActiveScope, DeepNesting) {
  {
    rsp::ActiveScope s1("L1");
    {
      rsp::ActiveScope s2("L2");
      {
        rsp::ActiveScope s3("L3");
        {
          rsp::ActiveScope s4("L4");
          EXPECT_EQ(rsp::GetScopeManager()->Current(), &s4);
        }
        EXPECT_EQ(rsp::GetScopeManager()->Current(), &s3);
      }
      EXPECT_EQ(rsp::GetScopeManager()->Current(), &s2);
    }
    EXPECT_EQ(rsp::GetScopeManager()->Current(), &s1);
  }
  EXPECT_EQ(rsp::GetScopeManager()->Current(), nullptr);
}

// --- RSP_SCOPE macro ---

TEST(Macros, RSP_SCOPE_CreatesScope) {
  {
    RSP_SCOPE("MacroScope");
    // ScopeManager should have a current scope
    EXPECT_NE(rsp::GetScopeManager()->Current(), nullptr);
  }
  EXPECT_EQ(rsp::GetScopeManager()->Current(), nullptr);
}

TEST(Macros, RSP_SCOPE_NestedMacros) {
  {
    RSP_SCOPE("Outer");
    auto *outer = rsp::GetScopeManager()->Current();
    EXPECT_NE(outer, nullptr);

    {
      RSP_SCOPE("Inner");
      auto *inner = rsp::GetScopeManager()->Current();
      EXPECT_NE(inner, nullptr);
      EXPECT_NE(outer, inner);
    }

    EXPECT_EQ(rsp::GetScopeManager()->Current(), outer);
  }
}

// --- RSP_SCOPE_METADATA macro ---

TEST(Macros, RSP_SCOPE_METADATA_AttachesToCurrentScope) {
  {
    RSP_SCOPE("MetaScope");
    RSP_SCOPE_METADATA("count", 42);

    auto *current = rsp::GetScopeManager()->Current();
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(current->info.metadata_ptr->metadata_idx, 1);
  }
}

TEST(Macros, RSP_SCOPE_METADATA_MultipleEntries) {
  {
    RSP_SCOPE("MultiMeta");
    RSP_SCOPE_METADATA("a", static_cast<int32_t>(1));
    RSP_SCOPE_METADATA("b", 2.0);
    RSP_SCOPE_METADATA("c", static_cast<uint64_t>(3));

    auto *current = rsp::GetScopeManager()->Current();
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(current->info.metadata_ptr->metadata_idx, 3);
  }
}

TEST(Macros, RSP_SCOPE_METADATA_AttachesToCorrectNestedScope) {
  {
    RSP_SCOPE("Outer");
    RSP_SCOPE_METADATA("outer_data", static_cast<int32_t>(10));

    auto *outer = rsp::GetScopeManager()->Current();

    {
      RSP_SCOPE("Inner");
      RSP_SCOPE_METADATA("inner_data", static_cast<int32_t>(20));

      auto *inner = rsp::GetScopeManager()->Current();
      EXPECT_EQ(inner->info.metadata_ptr->metadata_idx, 1);
      EXPECT_STREQ(inner->info.metadata_ptr->metadata[0].tag.c_str(), "inner_data");
    }

    // Outer scope should have its own metadata only
    EXPECT_EQ(outer->info.metadata_ptr->metadata_idx, 1);
    EXPECT_STREQ(outer->info.metadata_ptr->metadata[0].tag.c_str(), "outer_data");
  }
}

TEST(Macros, RSP_SCOPE_METADATA_NoScopeIsNoOp) {
  // When no scope is active, metadata macro should be a no-op (not crash)
  EXPECT_EQ(rsp::GetScopeManager()->Current(), nullptr);
  RSP_SCOPE_METADATA("orphan", static_cast<int32_t>(0));
  // Should not crash
}

// --- RSP_FUNCTION_SCOPE macro ---

static void HelperFunctionForTest() {
  RSP_FUNCTION_SCOPE;
  EXPECT_NE(rsp::GetScopeManager()->Current(), nullptr);
}

TEST(Macros, RSP_FUNCTION_SCOPE_CreatesScope) {
  HelperFunctionForTest();
  EXPECT_EQ(rsp::GetScopeManager()->Current(), nullptr);
}

// --- Multiple scopes in sequence ---

TEST(ActiveScope, SequentialScopes) {
  for (int i = 0; i < 10; ++i) {
    rsp::ActiveScope scope("Sequential");
    EXPECT_NE(rsp::GetScopeManager()->Current(), nullptr);
  }
  EXPECT_EQ(rsp::GetScopeManager()->Current(), nullptr);
}
