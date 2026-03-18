//
// Created by David Yang on 2026-02-08.
//

#ifndef AION_AST_CONTEXT_HPP
#define AION_AST_CONTEXT_HPP

#include <vector>
#include <deque>
#include <memory>
#include <algorithm>
#include <map>
#include <string_view>
#include <ast/ast.hpp>
#include <error/error.hpp>
#include <support/xxhash.h>

namespace aion::ast {

    template<typename T>
    concept Hashable = requires(const T& value) {
            { std::hash<T>{}(value) } -> std::convertible_to<std::size_t>;
    };

class ASTContext {
    struct Slab {
        char* buffer;
        char* current;
        std::size_t capacity;

        explicit Slab(std::size_t size);
        ~Slab();

        Slab(Slab&& other) noexcept;
        Slab& operator=(Slab&& other) noexcept;

        Slab(const Slab&) = delete;
        Slab& operator=(const Slab&) = delete;

        [[nodiscard]] bool is_full(std::size_t size = 0, std::size_t align = alignof(std::max_align_t)) const;
        [[nodiscard]] std::size_t get_remaining_capacity() const;
        void* allocate(std::size_t size, std::size_t align = alignof(std::max_align_t));
        void reset();
    };

public:
    template <typename VecAlloc = std::allocator<Slab>>
    class BumpPtrAllocator {
        std::deque<Slab, VecAlloc> slabs;
        std::vector<std::size_t> partially_used_slabs;
        std::size_t current_slab_idx{};
        std::size_t slab_size;


    public:
        explicit BumpPtrAllocator(std::size_t initial_slab_size = 1024 * 1024);

        /// @brief Allocates storage of at least the size (may allocate more than requested due to cache line alignment) and returns a pointer to it.
        /// If the current slab does not have enough space, a new slab is allocated.
        ///
        /// @param size the size of the memory chunk being allocated
        /// @param alignment alignment of chunk within the slab
        /// @param size_of_new_slab size of the new slab to be allocated if the current slab is full, a value <0 means to use member slab_size to construct the new slab
        /// @param reuse_free_slab if true, the allocator would try to reuse the slab that was cast aside in favor of a new bigger slab when allocating storage more than the available amount in the current slab.
        ///
        /// @returns pointer to the allocated memory chunk, or nullptr if allocation fails (e.g. if even a new slab cannot accommodate the requested size - fix: increase size_of_new_slab to allocate a new slab of a custom size)
        void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t),
                       std::size_t size_of_new_slab = 0, bool reuse_free_slab = true);

        void reset_slab(std::size_t idx);

        // ----------- getters --------------

        [[nodiscard]] int current_slab_index() const { return current_slab_idx; }
        [[nodiscard]] std::size_t num_slabs() const { return slabs.size(); }
        [[nodiscard]] std::size_t num_partially_used_slabs() const { return partially_used_slabs.size(); }

        [[nodiscard]] std::size_t num_allocated_bytes() const;
        [[nodiscard]] std::size_t num_allocated_bytes_used() const;
        [[nodiscard]] std::size_t slab_sizes() const { return slab_size; }
    };

    /// Open addressing
    template <typename Value>
    requires Hashable<Value>
    class StringMap {
        class Slot {
            std::size_t hash;
            std::string_view key;
            Value value;
            bool occupied;

            Slot(const std::string_view key, Value value, const std::size_t hash, const bool occupied)
            : hash(hash), key(key), value(value), occupied(occupied) {}
        };

        std::size_t capacity;       // num of allocated slots
        std::size_t size;           // num of occupied slots
        std::size_t bytes_used;     // num of bytes used
        ASTContext& ctx;
        Slot* slots;

    public:
        using value_type = Value;
        using allocator_type = BumpPtrAllocator<>;
        using key_type = std::string_view;
        using mapped_type = Value;
        using reference = Value&;
        using const_reference = const Value&;
        using pointer = Value*;
        using const_pointer = const Value*;
        using iterator = typename std::map<key_type, Value, std::less<>>::iterator;
        using const_iterator = typename std::map<key_type, Value, std::less<>>::const_iterator;

        explicit StringMap(ASTContext& ctx, std::size_t initial_capacity = 64) : capacity(initial_capacity), size(0), bytes_used(0), ctx(ctx) {
            std::size_t bytes = initial_capacity * sizeof(Slot);
            auto slots = static_cast<Slot*>(ctx.allocate(bytes, alignof(Slot)));
            this->slots = slots;
        }

        bool is_full() const {
            return size == capacity;
        }

        void push_back(std::pair<key_type, value_type>) {
            if (is_full()) {

            }
        }

    private:
    };

private:
    BumpPtrAllocator<> allocator;
    TranslationUnitDecl* tu_decl;

public:
    explicit ASTContext(std::size_t initial_slab_size = 1024 * 1024);

    [[nodiscard]] TranslationUnitDecl* get_translation_unit_decl() const { return tu_decl; }

    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
        return allocator.allocate(size, alignment);
    }

    template <typename T, typename... Args>
    requires std::is_trivially_destructible_v<T>
    T* create(Args&&... args) {
        void* storage = allocate(sizeof(T), alignof(T));
        return new (storage) T(std::forward<Args>(args)...);
    }

    char* allocate_string(std::string_view str) {
        if (str.empty()) return nullptr;
        char* storage = static_cast<char*>(allocate(str.size() + 1, 1));
        std::ranges::copy(str, storage);
        storage[str.size()] = '\0';
        return storage;
    }
};

} // namespace aion::ast

// Template implementation for BumpPtrAllocator
namespace aion::ast {

template <typename VecAlloc>
ASTContext::BumpPtrAllocator<VecAlloc>::BumpPtrAllocator(const std::size_t initial_slab_size)
    : slab_size(initial_slab_size) {
    slabs.emplace_back(slab_size);
}

template <typename VecAlloc>
void* ASTContext::BumpPtrAllocator<VecAlloc>::allocate(const std::size_t size, const std::size_t alignment,
                                                      const std::size_t size_of_new_slab, const bool reuse_free_slab) {
    if (reuse_free_slab && !partially_used_slabs.empty()) {
        for (auto it = partially_used_slabs.begin(); it != partially_used_slabs.end(); ) {
            Slab& slab = slabs[*it];
            if (void* result = slab.allocate(size, alignment)) {
                if (slab.get_remaining_capacity() == 0) {
                    it = partially_used_slabs.erase(it);
                }
                return result;
            }
            ++it;
        }
    }

    if (void* result = slabs[current_slab_idx].allocate(size, alignment)) {
        return result;
    }

    if (slabs[current_slab_idx].get_remaining_capacity() > 0) {
        partially_used_slabs.push_back(current_slab_idx);
    }

    // Handle large allocations by creating a dedicated slab.
    if (size_of_new_slab == 0 && size >= slab_size) {
        slabs.emplace_back(size + alignment);
        return slabs.back().allocate(size, alignment);
    }

    const std::size_t new_slab_size = size_of_new_slab > 0 ? size_of_new_slab : slab_size;
    slabs.emplace_back(new_slab_size);
    current_slab_idx = slabs.size() - 1;

    // Implement geometric growth: double the default slab size for future allocations, up to 128MB.
    if (size_of_new_slab == 0 && slab_size < 128 * 1024 * 1024) {
        slab_size *= 2;
    }

    return slabs[current_slab_idx].allocate(size, alignment);
}

template<typename VecAlloc>
void ASTContext::BumpPtrAllocator<VecAlloc>::reset_slab(std::size_t idx) {
    Slab& slab = slabs[idx];
    slab.reset();

    auto it = std::ranges::find(partially_used_slabs, idx);
    if (it != partially_used_slabs.end()) {
        partially_used_slabs.erase(it);
    }
    partially_used_slabs.insert(partially_used_slabs.begin(), idx);
}

template<typename VecAlloc>
std::size_t ASTContext::BumpPtrAllocator<VecAlloc>::num_allocated_bytes() const {
    std::size_t total = 0;
    for (const auto& slab : slabs) {
        total += slab.capacity;
    }
    return total;
}

template<typename VecAlloc>
std::size_t ASTContext::BumpPtrAllocator<VecAlloc>::num_allocated_bytes_used() const {
    std::size_t total = 0;
    for (const auto& slab : slabs) {
        total += slab.current - slab.buffer;
    }
    return total;
}

} // namespace aion::ast

#endif //AION_AST_CONTEXT_HPP