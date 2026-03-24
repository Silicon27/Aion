//
// Created by David Yang on 2026-03-22.
//

#ifndef AION_TYPE_HPP
#define AION_TYPE_HPP

#include "ast.hpp"
#include <support/global_constants.hpp>

namespace aion::ast {
    class BuiltinType;
    class MutableType;
    class UserDefinedType;

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

        static Kind get_kind(const lexer::TokenType type) {
            switch (type) {
                case lexer::TokenType::kw_i4: return Kind::i4;
                case lexer::TokenType::kw_i8: return Kind::i8;
                case lexer::TokenType::kw_i16: return Kind::i16;
                case lexer::TokenType::kw_i32: return Kind::i32;
                case lexer::TokenType::kw_i64: return Kind::i64;
                case lexer::TokenType::kw_i128: return Kind::i128;
                case lexer::TokenType::kw_f4: return Kind::f4;
                case lexer::TokenType::kw_f8: return Kind::f8;
                case lexer::TokenType::kw_f16: return Kind::f16;
                case lexer::TokenType::kw_f32: return Kind::f32;
                case lexer::TokenType::kw_f64: return Kind::f64;
                case lexer::TokenType::kw_f128: return Kind::f128;
                case lexer::TokenType::kw_char: return Kind::char_;
                case lexer::TokenType::kw_bool: return Kind::bool_;
                default:
                    // This should not happen if is_builtin_type_token(type) is true
                    return Kind::i32;
            }
        }

        explicit BuiltinType(const Kind BK)
            : Type(Type::Kind::builtin), builtin_kind(BK) {}

    protected:
        [[nodiscard]] Kind get_builtin_kind() const { return builtin_kind; }
    private:
        Kind builtin_kind;
    };
    static_assert(std::is_trivially_destructible_v<BuiltinType>);


    class MutableType {
        Type* base;
        bool is_mut = false;
    public:
        explicit MutableType(Type* base, const bool is_mutable)
            : base(base), is_mut(is_mutable) {}

        bool is_mutable() const { return is_mut; }
        void set_mutable(const bool is_mutable) { is_mut = is_mutable; }
        Type* get_base() const { return base; }
        void set_base(Type* new_base) { base = new_base; } // needed for casting
    };
    static_assert(std::is_trivially_destructible_v<MutableType>);

    class UserDefinedType : public Type{
        std::string_view name;
    public:
        explicit UserDefinedType(std::string_view name)
            : Type(Kind::user_defined), name(name) {}

        std::string_view get_name() const { return name; }
    };
}

#endif //AION_TYPE_HPP