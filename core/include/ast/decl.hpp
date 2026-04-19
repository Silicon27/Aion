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

    /// The top-level declaration that represents the entire translation unit.
    class TranslationUnitDecl : public Decl, public DeclContext {
    public:
        TranslationUnitDecl()
            : Decl(DeclKind::translation_unit) {}
    };
    static_assert(std::is_trivially_destructible_v<TranslationUnitDecl>);

    class NamedDecl : public Decl {
    public:
        IdentifierInfo* name;

    protected:
        NamedDecl(IdentifierInfo* name, const DeclKind kind)
        : Decl(kind), name(name) {}

    public:
        explicit NamedDecl(IdentifierInfo* name)
        : Decl(DeclKind::named), name(name) {}

        [[nodiscard]] IdentifierInfo* get_name() const { return name; }
        [[nodiscard]] std::string_view get_identifier() const { return name->get_name(); }
        void set_name(IdentifierInfo* new_name) { name = new_name; }
    };
    static_assert(std::is_trivially_destructible_v<NamedDecl>);

    class ValueDecl : public NamedDecl {
        MutableType* type;

    protected:
        ValueDecl(IdentifierInfo* name, MutableType* type, const DeclKind kind)
        : NamedDecl(name, kind), type(type) {}

    public:
        ValueDecl(IdentifierInfo* name, MutableType* type)
        : NamedDecl(name, DeclKind::value), type(type) {}

        [[nodiscard]] MutableType* get_type() const { return type; }
        void set_type(MutableType* new_type) { type = new_type; }
    };
    static_assert(std::is_trivially_destructible_v<ValueDecl>);

    class VarDecl : public ValueDecl {
        Expr* init;
        StorageClass storage_class;
        SourceRange id_range;

    public:
        VarDecl(IdentifierInfo* name, MutableType* type,
            StorageClass storage_class, const SourceRange &range,
            Expr* init = nullptr)
            : ValueDecl(name, type, DeclKind::variable), init(init),
        storage_class(storage_class), id_range(range) {}

        Expr* get_init() const { return init; }
        void set_init(Expr* init) { this->init = init; }

        StorageClass get_storage_class() const { return storage_class; }
        void set_storage_class(const StorageClass storage_class) { this->storage_class = storage_class; }

        SourceRange get_id_loc() const { return id_range; }
    };
    static_assert(std::is_trivially_destructible_v<VarDecl>);

    class FuncDecl : public ValueDecl, public DeclContext {
    public:
        bool is_export = false;

        FuncDecl(IdentifierInfo* name, MutableType* type, bool is_export)
        : ValueDecl(name, type, DeclKind::function), is_export(is_export) {}
    };

    class ErrorDecl : public NamedDecl {
    public:
        explicit ErrorDecl(IdentifierInfo* name)
        : NamedDecl(name, DeclKind::error) {}
    };
    static_assert(std::is_trivially_destructible_v<ErrorDecl>);
}

#endif //AION_DECL_HPP