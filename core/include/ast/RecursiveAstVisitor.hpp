//
// Created by David Yang on 2026-02-08.
//

#ifndef AION_RECURSIVE_AST_VISITOR_HPP
#define AION_RECURSIVE_AST_VISITOR_HPP

#include <ast/ast.hpp>
#include <ast/expr.hpp>

namespace aion::ast {
    template <typename Derived>
    class RecursiveAstVisitor;

    template <typename Derived>
    class RecursiveAstVisitor {
    public:
        bool TraverseExpr(Expr* expr) {

        }
    };
}

#endif //AION_RECURSIVE_AST_VISITOR_HPP