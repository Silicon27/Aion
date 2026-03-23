//
// Created by David Yang on 2026-03-22.
//

#ifndef AION_TYPE_HPP
#define AION_TYPE_HPP

#include "ast.hpp"

namespace aion::ast {
    class BuiltinType;
    class MutableType;

    class BuiltinType : public Type {
        friend class ASTContext;
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

        explicit BuiltinType(const Kind BK)
            : Type(Type::Kind::builtin), builtin_kind(BK) {}

    protected:
        [[nodiscard]] Kind get_builtin_kind() const { return builtin_kind; }
    private:
        Kind builtin_kind;
    };
    static_assert(std::is_trivially_destructible_v<BuiltinType>);

    class MutableType : public Type {
        Type* base_type;
        bool is_mut = false;
    public:
        explicit MutableType(const Kind kind, Type* base_type)
            : Type(kind), base_type(base_type) {}

        MutableType& operator=(Type parent) {
            base_type = &parent;
            return *this;
        }

        [[nodiscard]] Type* get_base_type() const { return base_type; }
        void set_base_type(Type* new_base_type) { base_type = new_base_type; }
        bool is_mutable() const { return is_mut; }
        void set_mutable(const bool is_mutable) { is_mut = is_mutable; }

    };
}

#endif //AION_TYPE_HPP