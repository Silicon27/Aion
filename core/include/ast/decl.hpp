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
            : Decl(Kind::translation_unit) {}
    };
    static_assert(std::is_trivially_destructible_v<TranslationUnitDecl>);

    class NamedDecl : public Decl {
        IdentifierInfo* name;
    public:
        NamedDecl(Kind declaration_kind, IdentifierInfo* name)
        : Decl(declaration_kind), name(name) {}

        [[nodiscard]] IdentifierInfo* get_name() const { return name; }
        [[nodiscard]] std::string_view get_identifier() const { return name->get_name(); }
        void set_name(IdentifierInfo* new_name) { name = new_name; }
    };

    class ValueDecl : public NamedDecl {
        MutableType type;
    public:
        ValueDecl(const Kind declaration_kind, IdentifierInfo* name, MutableType type)
        : NamedDecl(declaration_kind, name), type(type) {}

        [[nodiscard]] MutableType get_type() const { return type; }
        void set_type(const MutableType& new_type) { type = new_type; }
    };

    class VarDecl : public ValueDecl {
        Expr* init;
        StorageClass storage_class;
        SourceRange id_range;

    public:
        VarDecl(IdentifierInfo* name, const MutableType &type,
            StorageClass storage_class, const SourceRange &range,
            Expr* init = nullptr)
            : ValueDecl(Kind::variable, name, type), init(init),
        storage_class(storage_class), id_range(range) {}

        Expr* get_init() const { return init; }
        void set_init(Expr* init) { this->init = init; }

        StorageClass get_storage_class() const { return storage_class; }
        void set_storage_class(const StorageClass storage_class) { this->storage_class = storage_class; }

        SourceRange get_id_loc() const { return id_range; }
    };
}

#endif //AION_DECL_HPP