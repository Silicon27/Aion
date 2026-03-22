//
// Created by David Yang on 2026-03-17.
//

#ifndef AION_DECL_HPP
#define AION_DECL_HPP
#include <string_view>
#include "ast.hpp"

namespace aion::ast {
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


}

#endif //AION_DECL_HPP