//
// Created by David Yang on 2026-03-17.
//

#ifndef AION_DECL_HPP
#define AION_DECL_HPP
#include <string_view>
#include "ast.hpp"
#include "type.hpp"

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


}

#endif //AION_DECL_HPP