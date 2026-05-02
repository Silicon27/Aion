//
// Created by David Yang on 2026-03-17.
//

#ifndef AION_DECL_HPP
#define AION_DECL_HPP
#include <string_view>
#include <global_constants.hpp>
#include <ast/ast.hpp>
#include <ast/type.hpp>

#include <ast/ShortVec.hpp>

namespace aion::ast {
    class TranslationUnitDecl;
    class NamedDecl;
    class ValueDecl;
    class VarDecl;
    class FuncDecl;
    class ParamVarDecl;
    class ErrorDecl;

    /// The top-level declaration that represents the entire translation unit.
    class TranslationUnitDecl : public Decl, public DeclContext {
        SourceLocation unit_end;

    public:
        explicit TranslationUnitDecl(const SourceRange& range = SourceRange())
            : Decl(DeclKind::translation_unit, range.begin), unit_end(range.end) {}

        [[nodiscard]] SourceRange getSourceRange() const { return SourceRange(source_location, unit_end); }
    };
    static_assert(std::is_trivially_destructible_v<TranslationUnitDecl>);

    class  NamedDecl : public Decl {
    public:
        IdentifierInfo* name;
        SourceLocation decl_end;

    protected:
        NamedDecl(IdentifierInfo* name, const DeclKind kind, const SourceRange &range)
        : Decl(kind, range.begin), name(name), decl_end(range.end) {}

    public:
        explicit NamedDecl(IdentifierInfo* name, const SourceRange& range= {})
        : Decl(DeclKind::named, range.begin), name(name), decl_end(range.end) {}

        [[nodiscard]] IdentifierInfo* get_name() const { return name; }
        [[nodiscard]] std::string_view get_identifier() const {
            return name == nullptr ? std::string_view() : std::string_view(name->get_name(), name->get_length());
        }
        [[nodiscard]] SourceRange getSourceRange() const { return SourceRange(source_location, decl_end); }
        void set_name(IdentifierInfo* new_name) { name = new_name; }
    };
    static_assert(std::is_trivially_destructible_v<NamedDecl>);

    class ValueDecl : public NamedDecl {
        MutableType* type;

    protected:
        ValueDecl(IdentifierInfo* name, MutableType* type, const DeclKind kind, const SourceRange& range)
        : NamedDecl(name, kind, range), type(type) {}

    public:
        ValueDecl(IdentifierInfo* name, MutableType* type, const SourceRange& range)
        : NamedDecl(name, DeclKind::value, range), type(type) {}

        [[nodiscard]] MutableType* get_type() const { return type; }
        void set_type(MutableType* new_type) { type = new_type; }
    };
    static_assert(std::is_trivially_destructible_v<ValueDecl>);

    class VarDecl : public ValueDecl {
        Expr* init;
        StorageClass storage_class;

    public:
        VarDecl(IdentifierInfo* name, MutableType* type,
            StorageClass storage_class, const SourceRange &range,
            Expr* init = nullptr)
            : ValueDecl(name, type, DeclKind::variable, range), init(init),
        storage_class(storage_class) {}

        Expr* get_init() const { return init; }
        void set_init(Expr* init) { this->init = init; }

        StorageClass get_storage_class() const { return storage_class; }
        void set_storage_class(const StorageClass storage_class) { this->storage_class = storage_class; }

        SourceRange get_id_loc() const { return getSourceRange(); }
    };
    static_assert(std::is_trivially_destructible_v<VarDecl>);

    /// Function declarations; parameters (ParamVarDecl) are stored immediately after the FuncDecl object,
    /// with special getters get_params, get_param, and operator[] specialized to get these trailing objects
    class FuncDecl : public ValueDecl, public DeclContext {
    public:
        bool is_export = false;
        std::size_t num_params = 0;
        std::size_t num_ret_vals = 0;

        FuncDecl(IdentifierInfo* name, MutableType* type, bool is_export, std::size_t num_params, std::size_t num_ret_vals, const SourceRange& range)
        : ValueDecl(name, type, DeclKind::function, range), is_export(is_export), num_params(num_params), num_ret_vals(num_ret_vals) {}

        [[nodiscard]] ParamVarDecl** get_params() {
            return num_params == 0 ? nullptr : reinterpret_cast<ParamVarDecl**>(this + 1);
        }

        [[nodiscard]] ParamVarDecl* get_param(const std::size_t index) {
            if (index >= num_params) {
                return nullptr;
            }
            return get_params()[index];
        }

        ParamVarDecl* operator[](const std::size_t index) {
            return get_param(index);
        }

        [[nodiscard]] MutableType** get_return_types() {
            if (num_ret_vals == 0) return nullptr;
            return reinterpret_cast<MutableType**>(reinterpret_cast<ParamVarDecl**>(this + 1) + num_params);
        }

        // std::size_t param_curr_end() {
        //     ParamVarDecl** params = get_params();
        //
        //     // Safety check for functions with zero parameters
        //     if (!params) {
        //         return 0;
        //     }
        //
        //     std::size_t c = 0;
        //     // Stop if we reach capacity OR we find an uninitialized (null) slot
        //     while (c < num_params && params[c] != nullptr) {
        //         c++;
        //     }
        //
        //     return c;
        // }
    };
    static_assert(std::is_trivially_destructible_v<FuncDecl>);

    class ParamVarDecl : public ValueDecl {
    public:
        Expr* default_value;

        ParamVarDecl(IdentifierInfo* name, MutableType* type, const SourceRange &range,
                     Expr* default_value = nullptr)
            : ValueDecl(name, type, DeclKind::function_parameter, range), default_value(default_value) {}

        bool is_default_argument() const { return default_value != nullptr; }

        SourceRange get_id_loc() const { return getSourceRange(); }
    };
    static_assert(std::is_trivially_destructible_v<ParamVarDecl>);

    class ErrorDecl : public NamedDecl {
    public:
        explicit ErrorDecl(IdentifierInfo* name, const SourceRange& range)
        : NamedDecl(name, DeclKind::error, range) {}
    };
    static_assert(std::is_trivially_destructible_v<ErrorDecl>);
}

#endif //AION_DECL_HPP