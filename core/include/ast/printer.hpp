//
// Created by David Yang on 2026-04-11.
//

#ifndef AION_PRINTER_HPP
#define AION_PRINTER_HPP

#include <ast/RecursiveAstVisitor.hpp>
#include <iostream>

namespace aion::ast {
    class AstPrinter : public RecursiveAstVisitor<AstPrinter> {
    public:
        int indent = 0;

        bool print(TranslationUnitDecl* tu_decl) {
            if (tu_decl == nullptr) {
                std::cout << colorize(kErrorColor) << "<null translation unit>" << colorize(kReset) << '\n';
                return false;
            }

            std::cout << colorize(kDeclColor) << "TranslationUnitDecl" << colorize(kReset) << '\n';
            for (Decl* current = tu_decl->get_first_decl(); current != nullptr; current = current->next) {
                std::cout << "  " << colorize(kDeclColor) << "Decl(kind="
                          << static_cast<int>(current->get_kind()) << ")"
                          << colorize(kReset) << '\n';
            }
            return true;
        }

        void print_indent() {
            for (int i = 0; i < indent; i++) {
                std::cout << "  ";
            }
        }

        bool VisitBinaryExpr(BinaryExpr* expr) {
            print_indent();

            return true;
        }
    };
}

#endif //AION_PRINTER_HPP
