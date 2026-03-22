//
// Created by David Yang on 2026-03-22.
//

#ifndef AION_TYPE_HPP
#define AION_TYPE_HPP

#include "ast.hpp"

namespace aion::ast {
    class BuiltinType : public Type {
    public:
        enum class Kind {
            i4,
            i8,
            i16,
            i32,
            i64,
            i128,
            f4,
            f8,
            f16,
            f32,
            f64,
            f128,
            char_,
            bool_,
        };
    protected:
        explicit BuiltinType(const Kind BK)
            : Type(Type::Kind::builtin), builtin_kind(BK) {}

        [[nodiscard]] Kind get_builtin_kind() const { return builtin_kind; }
    private:
        Kind builtin_kind;
    };
    static_assert(std::is_trivially_destructible_v<BuiltinType>);
}

#endif //AION_TYPE_HPP