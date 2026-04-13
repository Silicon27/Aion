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
