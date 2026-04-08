//
// AST Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "ast_test.hpp"
#include <ast/ast.hpp>
#include <ast/decl.hpp>
#include <ast/stmt.hpp>
#include <ast/StringMap.hpp>
#include <ast/ShortVec.hpp>
#include <ast/ASTContext.hpp>
#include <global_constants.hpp>
#include <cstddef>
#include <initializer_list>
#include <utility>

namespace aion::test {

namespace {
    template <typename T>
    void assert_shortvec_contents(aion::ast::ShortVec<T>* vec, std::initializer_list<T> expected) {
        AION_ASSERT_EQ(vec->size(), expected.size());

        std::size_t index = 0;
        for (const auto& value : expected) {
            AION_ASSERT_EQ(vec->begin()[index], value);
            ++index;
        }
    }
}

void register_ast_tests(TestRunner& runner) {

    // ========================================================================
    // ShortVec Tests
    // ========================================================================

    auto shortvec_suite = std::make_unique<TestSuite>("AST::ShortVec");

    shortvec_suite->add_test("default_constructed_vector_starts_empty", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> vec(context);

        AION_ASSERT_EQ(vec.size(), std::size_t{0});
        AION_ASSERT_EQ(vec.capacity(), std::size_t{0});
        AION_ASSERT_NULL(vec.begin());
        AION_ASSERT_EQ(vec.begin(), vec.end());
    });

    shortvec_suite->add_test("initializer_list_constructor_allocates_power_of_two_capacity", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> vec(context, {1, 2, 3});

        AION_ASSERT_EQ(vec.size(), std::size_t{3});
        AION_ASSERT_EQ(vec.capacity(), std::size_t{4});
        AION_ASSERT_NOT_NULL(vec.begin());
        assert_shortvec_contents(&vec, {1, 2, 3});
    });

    shortvec_suite->add_test("empty_initializer_list_still_uses_minimum_storage", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> vec(context, {});

        AION_ASSERT_EQ(vec.size(), std::size_t{0});
        AION_ASSERT_EQ(vec.capacity(), std::size_t{1});
        AION_ASSERT_NOT_NULL(vec.begin());
    });

    shortvec_suite->add_test("reserve_rounds_up_and_preserves_values", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> vec(context, {10, 20});

        AION_ASSERT_EQ(vec.capacity(), std::size_t{2});
        vec.reserve(3);

        AION_ASSERT_EQ(vec.capacity(), std::size_t{4});
        assert_shortvec_contents(&vec, {10, 20});

        vec.reserve(2);
        AION_ASSERT_EQ(vec.capacity(), std::size_t{4});
        assert_shortvec_contents(&vec, {10, 20});
    });

    shortvec_suite->add_test("push_back_grows_across_capacity_boundaries", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> vec(context, {1});

        AION_ASSERT_EQ(vec.capacity(), std::size_t{1});

        vec.push_back(2);
        AION_ASSERT_EQ(vec.capacity(), std::size_t{2});
        assert_shortvec_contents(&vec, {1, 2});

        vec.push_back(3);
        AION_ASSERT_EQ(vec.capacity(), std::size_t{4});
        assert_shortvec_contents(&vec, {1, 2, 3});

        vec.push_back(4);
        assert_shortvec_contents(&vec, {1, 2, 3, 4});
        AION_ASSERT_EQ(vec.capacity(), std::size_t{4});
    });

    shortvec_suite->add_test("push_back_from_empty_vector_allocates_first_slot", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> vec(context);

        vec.push_back(99);

        AION_ASSERT_EQ(vec.size(), std::size_t{1});
        AION_ASSERT_EQ(vec.capacity(), std::size_t{1});
        assert_shortvec_contents(&vec, {99});
    });

    shortvec_suite->add_test("emplace_back_appends_values_directly", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> vec(context, {5});

        vec.emplace_back(6);
        vec.emplace_back(7);

        AION_ASSERT_EQ(vec.capacity(), std::size_t{4});
        assert_shortvec_contents(&vec, {5, 6, 7});
    });

    shortvec_suite->add_test("copy_constructor_creates_independent_storage", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> original(context, {8, 9});
        ShortVec<int> copy(original);

        AION_ASSERT_EQ(copy.size(), original.size());
        AION_ASSERT_EQ(copy.capacity(), original.capacity());
        AION_ASSERT_NE(copy.begin(), original.begin());
        assert_shortvec_contents(&copy, {8, 9});

        original.begin()[0] = 42;
        AION_ASSERT_EQ(copy.begin()[0], 8);
        AION_ASSERT_EQ(original.begin()[0], 42);
    });

    shortvec_suite->add_test("copy_assignment_creates_independent_storage", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> target(context, {1});
        ShortVec<int> source(context, {2, 3});

        target = source;

        AION_ASSERT_EQ(target.size(), source.size());
        AION_ASSERT_EQ(target.capacity(), source.capacity());
        AION_ASSERT_NE(target.begin(), source.begin());
        assert_shortvec_contents(&target, {2, 3});

        source.begin()[0] = 99;
        AION_ASSERT_EQ(target.begin()[0], 2);
        AION_ASSERT_EQ(source.begin()[0], 99);
    });

    shortvec_suite->add_test("move_constructor_transfers_storage_and_resets_source", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> source(context, {11, 12});
        ShortVec<int> moved(std::move(source));

        AION_ASSERT_EQ(source.size(), std::size_t{0});
        AION_ASSERT_EQ(source.capacity(), std::size_t{0});
        AION_ASSERT_NULL(source.begin());

        AION_ASSERT_EQ(moved.size(), std::size_t{2});
        AION_ASSERT_EQ(moved.capacity(), std::size_t{2});
        assert_shortvec_contents(&moved, {11, 12});
    });

    shortvec_suite->add_test("move_assignment_transfers_storage_and_resets_source", []() {
        using namespace aion::ast;

        ASTContext context;
        ShortVec<int> target(context, {1});
        ShortVec<int> source(context, {2, 3});

        target = std::move(source);

        AION_ASSERT_EQ(target.size(), std::size_t{2});
        AION_ASSERT_EQ(target.capacity(), std::size_t{2});
        assert_shortvec_contents(&target, {2, 3});

        AION_ASSERT_EQ(source.size(), std::size_t{0});
        AION_ASSERT_EQ(source.capacity(), std::size_t{0});
        AION_ASSERT_NULL(source.begin());
    });

    runner.add_suite(std::move(shortvec_suite));

    // ========================================================================
    // AST Node Creation Tests
    // ========================================================================

    auto node_suite = std::make_unique<TestSuite>("AST::NodeCreation");

    node_suite->add_test("placeholder_test", []() {
        // TODO: Add actual AST tests
        AION_ASSERT_TRUE(true);
    });

    node_suite->add_test("compound_stmt_trailing_objects", []() {
        using namespace aion::ast;
        ASTContext context;

        Stmt* s1 = context.create<Stmt>(Stmt::Kind::if_stmt);
        Stmt* s2 = context.create<Stmt>(Stmt::Kind::return_stmt);

        Stmt* stmts[] = {s1, s2};
        CompoundStmt* cs = CompoundStmt::create(context, stmts, 2);

        AION_ASSERT_EQ(cs->size(), 2);
        AION_ASSERT_EQ(cs->get_stmts()[0], s1);
        AION_ASSERT_EQ(cs->get_stmts()[1], s2);

        // Verify iterators
        int count = 0;
        for (Stmt* s : *cs) {
            if (count == 0) AION_ASSERT_EQ(s, s1);
            if (count == 1) AION_ASSERT_EQ(s, s2);
            count++;
        }
        AION_ASSERT_EQ(count, 2);
    });

    node_suite->add_test("decl_context_linked_list", []() {
        using namespace aion::ast;
        ASTContext context;
        TranslationUnitDecl* tu = context.get_translation_unit_decl();

        Decl* d1 = context.create<Decl>(Decl::DeclKind::variable);
        Decl* d2 = context.create<Decl>(Decl::DeclKind::function);

        tu->add_decl(d1);
        tu->add_decl(d2);

        AION_ASSERT_EQ(tu->get_first_decl(), d1);
        AION_ASSERT_EQ(tu->get_last_decl(), d2);
        AION_ASSERT_EQ(d1->next, d2);
        AION_ASSERT_NULL(d2->next);
    });

    node_suite->add_test("vardecl_and_identifier_info_test", []() {
        using namespace aion::ast;
        ASTContext context;
        FileId fid = 42;
        Offset offset = 1337;
        
        BuiltinType* bt = context.create<BuiltinType>(BuiltinType::Kind::i32);
        MutableType* type = context.create<MutableType>(bt, false);
        IdentifierInfo* ii = context.emplace_or_get_identifier("x");
        VarDecl* vd = context.create<VarDecl>(ii, type,
            StorageClass::stack,
            SourceRange(SourceLocation(fid, offset), SourceLocation(fid, offset + 4)));

        AION_ASSERT_EQ(vd->get_identifier(), "x");
        AION_ASSERT_EQ(static_cast<int>(vd->get_storage_class()), static_cast<int>(StorageClass::stack));

        AION_ASSERT_EQ(context.emplace_or_get_identifier("x"), ii);
    });

    runner.add_suite(std::move(node_suite));

    // ========================================================================
    // AST Context Tests
    // ========================================================================

    auto context_suite = std::make_unique<TestSuite>("AST::Context");

    context_suite->add_test("alignment_bug_reproduction", []() {
        using namespace aion::ast;
        // We want to verify that is_full(size, alignment) correctly accounts for padding.

        ASTContext::BumpPtrAllocator allocator(64);

        // 1. Fill the slab partially so that the next allocation might require padding.
        // Allocate 4 bytes.
        void* p1 = allocator.allocate(4, 4);
        AION_ASSERT_NOT_NULL(p1);

        // 2. Request an allocation that exactly fits the remaining capacity IF no padding is needed,
        // but REQUIRES padding.
        // is_full(60, 8) should now return true, causing a new slab to be allocated.
        
        void* p2 = allocator.allocate(60, 8);
        
        // This should NOT be null because it should have been allocated in a new slab.
        AION_ASSERT_NOT_NULL(p2);

        // ensure a second slab is generated
        AION_ASSERT_GT(allocator.num_slabs(), 1);
        
        // Check that it's aligned
        AION_ASSERT_EQ(reinterpret_cast<std::uintptr_t>(p2) % 8, 0);
    });

    context_suite->add_test("partially_filled_slab_usage", [] {
        using namespace aion::ast;
        ASTContext::BumpPtrAllocator allocator(64);

        // partially fill a slab, this enables the allocator to push the semi used slab onto the partially filled vector
        void* p1 = allocator.allocate(4, 4);

        AION_ASSERT_NOT_NULL(p1);
        AION_ASSERT_EQ(allocator.num_slabs(), 1);

        // now we mandate the allocator create a new slab
        void* p2 = allocator.allocate(60, 8);

        AION_ASSERT_NOT_NULL(p2);
        AION_ASSERT_EQ(allocator.num_slabs(), 2);

        // and now we try to allocate some storage that would fit into the partially filled slab
        void* p3 = allocator.allocate(4, 4, 0,true);

        AION_ASSERT_NOT_NULL(p3);
        AION_ASSERT_EQ(allocator.num_slabs(), 2);
    });

    context_suite->add_test("string_map_rehash", [] {
        using namespace aion::ast;
        ASTContext context;

        // Use a small initial capacity to trigger rehash early
        StringMap<int> map(context, 4);

        map.insert({"a", 1});
        map.insert({"b", 2});
        map.insert({"c", 3});
        map.insert({"d", 4}); // Should trigger rehash to 8

        AION_ASSERT_NOT_NULL(map.find("a"));
        AION_ASSERT_EQ(*map.find("a"), 1);
        AION_ASSERT_NOT_NULL(map.find("b"));
        AION_ASSERT_EQ(*map.find("b"), 2);
        AION_ASSERT_NOT_NULL(map.find("c"));
        AION_ASSERT_EQ(*map.find("c"), 3);
        AION_ASSERT_NOT_NULL(map.find("d"));
        AION_ASSERT_EQ(*map.find("d"), 4);
        
        AION_ASSERT_NULL(map.find("e"));
    });

    context_suite->add_test("string_map_many_insertions", [] {
        using namespace aion::ast;
        ASTContext context;

        StringMap<int> map(context, 4);

        for (int i = 0; i < 100; ++i) {
            std::string key = "key" + std::to_string(i);
            char* key_ptr = context.allocate_string(key);
            map.insert({std::string_view(key_ptr, key.size()), i});
        }

        for (int i = 0; i < 100; ++i) {
            std::string key = "key" + std::to_string(i);
            AION_ASSERT_NOT_NULL(map.find(key));
            AION_ASSERT_EQ(*map.find(key), i);
        }
    });

    context_suite->add_test("string_map_member_functions", [] {
        using namespace aion::ast;
        ASTContext context;

        StringMap<int> map(context, 3);

        AION_ASSERT_EQ(map.get_capacity(), 4);

        map.insert({"a", 1});
        map.insert({"b", 2});
        map.insert({"c", 3});
        AION_ASSERT_EQ(map.get_capacity(), 4);
        AION_ASSERT_EQ(map.get_size(), 3);
        map.insert({"d", 4});

        AION_ASSERT_EQ(map.get_size(), 4);
        AION_ASSERT_EQ(map.get_capacity(), 8);
        AION_ASSERT_EQ(map.get_bytes_used(), 4);

        // test out rehashing

        map.insert({"e", 5});

        AION_ASSERT_EQ(map.get_capacity(), 8);

        AION_ASSERT_EQ(map["a"], 1);

        map.insert({"a", 6});

        AION_ASSERT_EQ(map["a"], 6);
    });

    context_suite->add_test("string_map_custom_load_factor", [] {
        using namespace aion::ast;
        ASTContext context;

        StringMap<int> map(context, 4);
        map.set_max_load_factor(0.5f);

        AION_ASSERT_EQ(map.get_capacity(), 4);
        AION_ASSERT_EQ(map.get_max_load_factor(), 0.5f);

        map.insert({"a", 1});
        AION_ASSERT_EQ(map.get_capacity(), 4);

        map.insert({"b", 2});
        // At this point size = 1.
        // is_full() check when inserting "b": size=1, capacity=4. 1 >= 4 * 0.5 (2) is false.
        
        // After inserting "b", size = 2.
        // is_full() check when inserting "c": size=2, capacity=4. 2 >= 4 * 0.5 (2) is true.
        // So inserting "c" should trigger rehash.
        
        map.insert({"c", 3});
        AION_ASSERT_EQ(map.get_capacity(), 8);
    });

    context_suite->add_test("reset_slab_reorders_partially_used", [] {
        using namespace aion::ast;
        ASTContext::BumpPtrAllocator allocator(64);

        allocator.allocate(40); // Slab 0, 24 left.
        allocator.allocate(40); // Slab 1 (current), 24 left. Slab 0 is in partially_used.

        AION_ASSERT_EQ(allocator.num_slabs(), 2);
        AION_ASSERT_EQ(allocator.num_partially_used_slabs(), 1);

        // Reset Slab 0. It's already there, so it should be moved to front (it's already the only one).
        allocator.reset_slab(0);
        AION_ASSERT_EQ(allocator.num_partially_used_slabs(), 1);

        // Allocate 40 again. Slab 1 is current, but Slab 0 is in partially_used and it's tried first.
        // Slab 0 now has 64 available.
        void* p = allocator.allocate(40);
        AION_ASSERT_NOT_NULL(p);
        AION_ASSERT_EQ(allocator.num_slabs(), 2);

        // To verify it used Slab 0:
        // Slab 0 should now have 40 used.
        // Slab 1 should still have 40 used.
        // Total used should be 80.
        AION_ASSERT_EQ(allocator.num_allocated_bytes_used(), 80);

        // Now trigger Slab 2
        allocator.allocate(40); // Current is Slab 1, but it only has 24 left.
        // Wait, if it tries partially_used first, it tries Slab 0.
        // Slab 0 has 64 - 40 = 24 left.
        // So it tries Slab 0 (24 left) -> fails.
        // Then it tries Slab 1 (current, 24 left) -> fails.
        // Then it creates Slab 2.
        AION_ASSERT_EQ(allocator.num_slabs(), 3);
        // Slab 1 should have been added to partially_used.
        // partially_used order: [1, 0]
        AION_ASSERT_EQ(allocator.num_partially_used_slabs(), 2);

        // Now reset Slab 0. It should be moved to front: [0, 1]
        allocator.reset_slab(0);
        AION_ASSERT_EQ(allocator.num_partially_used_slabs(), 2);

        // Allocation should now prioritize Slab 0.
        // Slab 0 is now empty (64 available).
        void* p2 = allocator.allocate(60);
        AION_ASSERT_NOT_NULL(p2);
        AION_ASSERT_EQ(allocator.num_slabs(), 3); // No new slab needed.
    });

    context_suite->add_test("large_allocation_handling", [] {
        using namespace aion::ast;
        ASTContext::BumpPtrAllocator allocator(64);

        // Allocate more than slab_size
        void* p = allocator.allocate(128);
        AION_ASSERT_NOT_NULL(p);
        // Should have allocated a dedicated slab (total 2 slabs)
        AION_ASSERT_EQ(allocator.num_slabs(), 2);
        
        // Ensure it's correctly aligned
        AION_ASSERT_EQ(reinterpret_cast<std::uintptr_t>(p) % alignof(std::max_align_t), 0);
    });

    context_suite->add_test("geometric_growth", [] {
        using namespace aion::ast;
        ASTContext::BumpPtrAllocator allocator(64);

        AION_ASSERT_EQ(allocator.slab_sizes(), 64);
        
        // Trigger first new slab allocation
        allocator.allocate(60); // Slab 0 (64)
        allocator.allocate(60); // Slab 1 (64)
        
        // After Slab 1 is created, slab_size should double for the NEXT allocation
        AION_ASSERT_EQ(allocator.slab_sizes(), 128);
        
        // Trigger second new slab allocation
        allocator.allocate(120); // Slab 2 (128)
        AION_ASSERT_EQ(allocator.slab_sizes(), 256);
    });

    context_suite->add_test("string_map_comprehensive", [] {
        using namespace aion::ast;
        ASTContext context;
        StringMap<int> map(context, 4);

        // Test insertion and retrieval
        map["hello"] = 42;
        AION_ASSERT_EQ(map["hello"], 42);
        AION_ASSERT_EQ(map.get_size(), 1);
        AION_ASSERT_EQ(map.get_bytes_used(), 5);

        // Test overwrite via operator[]
        map["hello"] = 43;
        AION_ASSERT_EQ(map["hello"], 43);
        AION_ASSERT_EQ(map.get_size(), 1);
        AION_ASSERT_EQ(map.get_bytes_used(), 5);

        // Test multiple elements
        map["world"] = 100;
        AION_ASSERT_EQ(map["world"], 100);
        AION_ASSERT_EQ(map.get_size(), 2);
        AION_ASSERT_EQ(map.get_bytes_used(), 10);

        // Test rehash (cap 4 -> 8 when size reaches 3)
        // size is 2, cap is 4. Next insertion (3rd) won't trigger rehash yet.
        map["test"] = 7; // size becomes 3.
        AION_ASSERT_EQ(map.get_size(), 3);
        AION_ASSERT_EQ(map.get_capacity(), 4);

        // Now size is 3, 3*4 >= 4*3 is True. Next insertion triggers rehash.
        map["rehash"] = 123;
        AION_ASSERT_EQ(map.get_size(), 4);
        AION_ASSERT_EQ(map.get_capacity(), 8);
        AION_ASSERT_EQ(map.get_bytes_used(), 20); // hello(5), world(5), test(4), rehash(6) = 20

        AION_ASSERT_EQ(map["hello"], 43);
        AION_ASSERT_EQ(map["world"], 100);
        AION_ASSERT_EQ(map["test"], 7);
        AION_ASSERT_EQ(map["rehash"], 123);
    });

    runner.add_suite(std::move(context_suite));

    // ========================================================================
    // AST Traversal Tests
    // ========================================================================

    auto traversal_suite = std::make_unique<TestSuite>("AST::Traversal");

    traversal_suite->add_test("placeholder_test", []() {
        // TODO: Add actual AST traversal tests
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(traversal_suite));

    // ========================================================================
    // AST Visitor Tests
    // ========================================================================

    auto visitor_suite = std::make_unique<TestSuite>("AST::Visitor");

    visitor_suite->add_test("placeholder_test", []() {
        // TODO: Add actual visitor tests
        AION_ASSERT_TRUE(true);
    });

    runner.add_suite(std::move(visitor_suite));
}

} // namespace aion::test

// ============================================================================
// Main function for standalone AST test executable
// Only compiled when building as standalone (AST_TEST_STANDALONE defined)
// ============================================================================
#ifdef AST_TEST_STANDALONE
int main(int argc, char* argv[]) {
    using namespace aion::test;

    TestRunner runner;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
    }

    register_ast_tests(runner);

    return runner.run_all(verbose);
}
#endif
