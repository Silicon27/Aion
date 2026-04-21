//
// Created by David Yang on 2026-03-17.
//

#ifndef AION_DECL_HPP
#define AION_DECL_HPP
#include <string_view>
#include <global_constants.hpp>
#include <ast/ast.hpp>
#include <ast/type.hpp>

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
        SourceRange unit_range;

    public:
        explicit TranslationUnitDecl(const SourceRange& range = SourceRange())
            : Decl(DeclKind::translation_unit, range.begin), unit_range(range) {}

        [[nodiscard]] SourceRange getSourceRange() const { return unit_range; }
    };
    static_assert(std::is_trivially_destructible_v<TranslationUnitDecl>);

    class NamedDecl : public Decl {
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
        [[nodiscard]] std::string_view get_identifier() const { return name->get_name(); }
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

    class FuncDecl : public ValueDecl, public DeclContext {
    public:
        bool is_export = false;

        FuncDecl(IdentifierInfo* name, MutableType* type, bool is_export, const SourceRange& range)
        : ValueDecl(name, type, DeclKind::function, range), is_export(is_export) {}
    };
    static_assert(std::is_trivially_destructible_v<FuncDecl>);

    /// provisional, temporary VarDecl mirror
    class ParamVarDecl : public ValueDecl {
    public:
        Expr* default_value;
        SourceRange id_range;

        ParamVarDecl(IdentifierInfo* name, MutableType* type,
                     StorageClass storage_class, const SourceRange &range,
                     Expr* default_value = nullptr)
            : ValueDecl(name, type, DeclKind::variable, range), default_value(default_value), id_range(range) {}

        Expr* get_default_value() const { return default_value; }
        void set_init(Expr* init) { this->default_value = init; }

        SourceRange get_id_loc() const { return id_range; }
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