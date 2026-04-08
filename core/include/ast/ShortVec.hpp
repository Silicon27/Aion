//
// Created by David Yang on 2026-04-08.
//

#ifndef AION_SHORTVEC_HPP
#define AION_SHORTVEC_HPP

#include <utility>
#include <initializer_list>

namespace aion::ast {
    class ASTContext;

    /// ShortVec - A lightweight alternative to std::vector, allocating
    /// within context boundaries and optimized for fewer allocations/elements.
    ///
    /// @tparam EleT - Element type stored in the vector
    /// @tparam Context - Context type for allocation and management
    template <typename EleT, typename Context = ASTContext> // element type
    class ShortVec {
        EleT *data;
        std::size_t size;
        std::size_t capacity;
        Context& context;

    public:
        using value_type = EleT;
        using context_type = Context;
        using size_type = std::size_t;
        using reference = EleT&;
        using const_reference = const EleT&;
        using pointer = EleT*;
        using const_pointer = const EleT*;
        using iterator = pointer;
        using const_iterator = const_pointer;

        explicit ShortVec(Context& context) : data(nullptr), size(0), capacity(0), context(context) {}
    };
}

#endif //AION_SHORTVEC_HPP
