#include <support/global_constants.hpp>
#include <ast/ASTContext.hpp>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <cstdlib>

namespace aion::ast {

ASTContext::Slab::Slab(const std::size_t size) {
    // ensure the allocated buffer is of a size that is a
    // multiple of the cache line size for better cache locality
    const std::size_t alloc_size = (size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
    buffer = new (std::nothrow) char[alloc_size];
    if (!buffer) {
        std::cerr << "ASTContext: Fatal error: out of memory during slab allocation.\n";
        std::abort();
    }
    current = buffer;
    capacity = size;
}

ASTContext::Slab::~Slab() {
    delete[] buffer;
}

ASTContext::Slab::Slab(Slab&& other) noexcept
    : buffer(other.buffer), current(other.current), capacity(other.capacity) {
    other.buffer = nullptr;
    other.current = nullptr;
    other.capacity = 0;
}

ASTContext::Slab& ASTContext::Slab::operator=(Slab&& other) noexcept {
    if (this != &other) {
        delete[] buffer;
        buffer = other.buffer;
        current = other.current;
        capacity = other.capacity;
        other.buffer = nullptr;
        other.current = nullptr;
        other.capacity = 0;
    }
    return *this;
}

bool ASTContext::Slab::is_full(const std::size_t size, const std::size_t align) const {
    const auto curr_addr = reinterpret_cast<std::uintptr_t>(current);
    const std::size_t padding = (align - (curr_addr % align)) % align;
    return (capacity - (current - buffer)) < (size + padding);
}

std::size_t ASTContext::Slab::get_remaining_capacity() const {
    return capacity - (current - buffer);
}

void* ASTContext::Slab::allocate(const std::size_t size, const std::size_t align) {
    std::size_t space_left = capacity - (current - buffer);
    void* ptr = current;

    void* align_ptr = std::align(align, size, ptr, space_left);
    if (!align_ptr) return nullptr;

    current = static_cast<char*>(align_ptr) + size;
    return align_ptr;
}

void ASTContext::Slab::reset() {
    current = buffer; // overwrite all data
}

ASTContext::ASTContext(std::size_t initial_slab_size)
    : allocator(initial_slab_size), identifiers(*this) {
    tu_decl = create<TranslationUnitDecl>();
}

} // namespace aion::ast