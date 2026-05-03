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
    class FunctionType;
    class UserDefinedType;

    enum class BuiltinTypeKind {
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
        void_,
        string_literal,
    };

    class BuiltinType : public Type {
        friend class ASTContext;
    public:
        static BuiltinTypeKind get_kind(const lexer::TokenType type) {
            switch (type) {
                case lexer::TokenType::kw_i4: return BuiltinTypeKind::i4;
                case lexer::TokenType::kw_i8: return BuiltinTypeKind::i8;
                case lexer::TokenType::kw_i16: return BuiltinTypeKind::i16;
                case lexer::TokenType::kw_i32: return BuiltinTypeKind::i32;
                case lexer::TokenType::kw_i64: return BuiltinTypeKind::i64;
                case lexer::TokenType::kw_i128: return BuiltinTypeKind::i128;
                case lexer::TokenType::kw_f4: return BuiltinTypeKind::f4;
                case lexer::TokenType::kw_f8: return BuiltinTypeKind::f8;
                case lexer::TokenType::kw_f16: return BuiltinTypeKind::f16;
                case lexer::TokenType::kw_f32: return BuiltinTypeKind::f32;
                case lexer::TokenType::kw_f64: return BuiltinTypeKind::f64;
                case lexer::TokenType::kw_f128: return BuiltinTypeKind::f128;
                case lexer::TokenType::kw_char: return BuiltinTypeKind::char_;
                case lexer::TokenType::kw_bool: return BuiltinTypeKind::bool_;
                case lexer::TokenType::kw_void: return BuiltinTypeKind::void_;
                case lexer::TokenType::int_literal: return BuiltinTypeKind::i32; // TODO make this platform dependent, make it the size of the platforms word size
                case lexer::TokenType::float_literal: return BuiltinTypeKind::f32; // todo this too
                case lexer::TokenType::string_literal: return BuiltinTypeKind::string_literal;
                default:
                    // This should not happen if is_builtin_type_token(type) is true
                    return BuiltinTypeKind::i32;
            }
        }

        explicit BuiltinType(const BuiltinTypeKind BK)
            : Type(TypeKind::builtin), builtin_kind(BK) {}

    protected:
        [[nodiscard]] BuiltinTypeKind get_builtin_kind() const { return builtin_kind; }
    private:
        BuiltinTypeKind builtin_kind;
    };
    static_assert(std::is_trivially_destructible_v<BuiltinType>);


    class MutableType {
        Type* base;
        bool is_mut = false;
        bool is_comp = false;
    public:
        explicit MutableType(Type* base, const bool is_mutable, const bool is_comp = false)
            : base(base), is_mut(is_mutable), is_comp(is_comp) {}

        bool is_mutable() const { return is_mut; }
        void set_mutable(const bool is_mutable) { is_mut = is_mutable; }
        bool is_compile_time() const { return is_comp; }
        void set_compile_time(const bool is_compile_time) { is_comp = is_compile_time; }
        Type* get_base() const { return base; }
        void set_base(Type* new_base) { base = new_base; } // needed for casting
    };
    static_assert(std::is_trivially_destructible_v<MutableType>);

    /**
     * @class FunctionType
     * @brief Represents a function type in the AST.
     *
     * This class is used to encapsulate the metadata of a function type,
     * including its return type and the number of parameters it expects.
     * It inherits from the base Type class and is identified as a function type
     * using the TypeKind::function enumeration value.
     */
    class FunctionType : public Type {
        friend class ASTContext;
        MutableType* return_type;
        std::size_t num_params;
    public:
        FunctionType(MutableType* return_type, std::size_t num_params)
            : Type(TypeKind::function), return_type(return_type), num_params(num_params) {}

        [[nodiscard]] MutableType* get_return_type() const { return return_type; }
        [[nodiscard]] std::size_t get_num_params() const { return num_params; }

        [[nodiscard]] MutableType** get_param_types() {
            return num_params == 0 ? nullptr : reinterpret_cast<MutableType**>(this + 1);
        }
    };
    static_assert(std::is_trivially_destructible_v<FunctionType>);

    class UserDefinedType : public Type{
        std::string_view name;
    public:
        explicit UserDefinedType(std::string_view name)
            : Type(TypeKind::user_defined), name(name) {}

        std::string_view get_name() const { return name; }
    };
    static_assert(std::is_trivially_destructible_v<UserDefinedType>);
}

#endif //AION_TYPE_HPP