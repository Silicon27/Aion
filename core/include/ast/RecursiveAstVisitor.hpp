//
// Created by David Yang on 2026-02-08.
//

#ifndef AION_RECURSIVE_AST_VISITOR_HPP
#define AION_RECURSIVE_AST_VISITOR_HPP

#include <ast/ast.hpp>
#include <ast/decl.hpp>
#include <ast/expr.hpp>
#include <ast/stmt.hpp>

#include <iostream>

namespace aion::ast {
    template <typename Derived>
    class RecursiveAstVisitor {
    public:
        bool enable_colors = true;

    protected:
        int indent_level = 0;

        static constexpr const char* kReset = "\x1b[0m";
        static constexpr const char* kExprColor = "\x1b[38;5;110m";
        static constexpr const char* kDeclColor = "\x1b[38;5;145m";
        static constexpr const char* kStmtColor = "\x1b[38;5;109m";
        static constexpr const char* kInfoColor = "\x1b[38;5;244m";
        static constexpr const char* kErrorColor = "\x1b[38;5;174m";

        Derived& getDerived() { return *static_cast<Derived*>(this); }

        void print_indent() const {
            for (int i = 0; i < indent_level; ++i) {
                std::cout << "  ";
            }
        }

        const char* colorize(const char* color) const {
            return enable_colors ? color : "";
        }

        void print_enter(const char* category_color, const char* node_name) {
            print_indent();
            std::cout << colorize(category_color) << "> " << node_name << colorize(kReset) << '\n';
            ++indent_level;
        }

        void print_exit(const char* category_color, const char* node_name, const bool ok) {
            --indent_level;
            print_indent();
            std::cout << colorize(category_color) << "< " << node_name << " "
                      << colorize(ok ? kInfoColor : kErrorColor)
                      << (ok ? "[ok]" : "[failed]")
                      << colorize(kReset) << '\n';
        }

        template <typename Node>
        bool run_traverse(const char* category_color, const char* node_name, Node* node,
            bool (Derived::*visit)(Node*), bool (Derived::*walk)(Node*)) {
            if (node == nullptr) {
                print_indent();
                std::cout << colorize(kErrorColor) << "- null " << node_name << colorize(kReset) << '\n';
                return true;
            }

            print_enter(category_color, node_name);
            const bool visited = (getDerived().*visit)(node);
            const bool walked = visited && (getDerived().*walk)(node);
            print_exit(category_color, node_name, walked);
            return walked;
        }

        bool TraverseExpr(Expr* expr);
        bool TraverseDecl(Decl* decl);
        bool TraverseStmt(Stmt* stmt);

        // -------------------------------------------------
        // -Expr nodes
        // -------------------------------------------------

        bool TraverseErrorExpr(ErrorExpr* expr);
        bool TraverseTypedExpr(TypedExpr* expr);
        bool TraverseDeclRefExpr(DeclRefExpr* expr);
        bool TraverseBinaryExpr(BinaryExpr* expr);
        bool TraverseUnaryExpr(UnaryExpr* expr);
        bool TraverseIntegerLiteralExpr(IntegerLiteralExpr* expr);
        bool TraverseFloatLiteralExpr(FloatLiteralExpr* expr);
        bool TraverseStringLiteralExpr(StringLiteralExpr* expr);
        bool TraverseCallExpr(CallExpr* expr);

        // -------------------------------------------------
        // -Decl nodes
        // -------------------------------------------------

        bool TraverseTranslationUnitDecl(TranslationUnitDecl* decl);
        bool TraverseNamedDecl(NamedDecl* decl);
        bool TraverseValueDecl(ValueDecl* decl);
        bool TraverseVarDecl(VarDecl* decl);
        bool TraverseErrorDecl(ErrorDecl* decl);

        // -------------------------------------------------
        // -Stmt nodes
        // -------------------------------------------------

        bool TraverseCompoundStmt(CompoundStmt* stmt);


        // -------------------------------------------------
        // defaults
        // -------------------------------------------------


        // -------------------------------------------------
        // -Expr node visitors
        // -------------------------------------------------

        bool VisitErrorExpr(ErrorExpr* expr) {
            return true;
        }

        bool WalkUpFromErrorExpr(ErrorExpr* expr) {
            return true;
        }

        bool VisitTypedExpr(TypedExpr* expr) {
            return true;
        }

        bool WalkUpFromTypedExpr(TypedExpr* expr) {
            return true;
        }

        bool VisitDeclRefExpr(DeclRefExpr* expr) {
            return true;
        }

        bool WalkUpFromDeclRefExpr(DeclRefExpr* expr) {
            return getDerived().WalkUpFromTypedExpr(expr);
        }

        bool VisitBinaryExpr(BinaryExpr* expr) {
            return true;
        }

        bool WalkUpFromBinaryExpr(BinaryExpr* expr) {
            return getDerived().WalkUpFromTypedExpr(expr);
        }

        bool VisitUnaryExpr(UnaryExpr* expr) {
            return true;
        }

        bool WalkUpFromUnaryExpr(UnaryExpr* expr) {
            return getDerived().WalkUpFromTypedExpr(expr);
        }

        bool VisitIntegerLiteralExpr(IntegerLiteralExpr* expr) {
            return true;
        }

        bool WalkUpFromIntegerLiteralExpr(IntegerLiteralExpr* expr) {
            return getDerived().WalkUpFromTypedExpr(expr);
        }

        bool VisitFloatLiteralExpr(FloatLiteralExpr* expr) {
            return true;
        }

        bool WalkUpFromFloatLiteralExpr(FloatLiteralExpr* expr) {
            return getDerived().WalkUpFromTypedExpr(expr);
        }

        bool VisitStringLiteralExpr(StringLiteralExpr* expr) {
            return true;
        }

        bool WalkUpFromStringLiteralExpr(StringLiteralExpr* expr) {
            return getDerived().WalkUpFromTypedExpr(expr);
        }

        bool VisitCallExpr(CallExpr* expr) {
            return true;
        }

        bool WalkUpFromCallExpr(CallExpr* expr) {
            return getDerived().WalkUpFromDeclRefExpr(expr);
        }

        // -------------------------------------------------
        // -Decl node visitors
        // -------------------------------------------------

        bool VisitTranslationUnitDecl(TranslationUnitDecl* decl) {
            return true;
        }

        bool WalkUpFromTranslationUnitDecl(TranslationUnitDecl* decl) {
            return true;
        }

        bool VisitNamedDecl(NamedDecl* decl) {
            return true;
        }

        bool WalkUpFromNamedDecl(NamedDecl* decl) {
            return true;
        }

        bool VisitValueDecl(ValueDecl* decl) {
            return true;
        }

        bool WalkUpFromValueDecl(ValueDecl* decl) {
            return getDerived().WalkUpFromNamedDecl(decl);
        }

        bool VisitVarDecl(VarDecl* decl) {
            return true;
        }

        bool WalkUpFromVarDecl(VarDecl* decl) {
            return getDerived().WalkUpFromValueDecl(decl);
        }

        bool VisitErrorDecl(ErrorDecl* decl) {
            return true;
        }

        bool WalkUpFromErrorDecl(ErrorDecl* decl) {
            return getDerived().WalkUpFromNamedDecl(decl);
        }


        // -------------------------------------------------
        // -Stmt node visitors
        // -------------------------------------------------

        bool VisitCompoundStmt(CompoundStmt* stmt) {
            return true;
        }

        bool WalkUpFromCompoundStmt(CompoundStmt* stmt) {
            return true;
        }

    };

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseExpr(Expr* expr) {
        if (expr == nullptr) {
            print_indent();
            std::cout << colorize(kErrorColor) << "- null Expr" << colorize(kReset) << '\n';
            return true;
        }

        switch (expr->get_kind()) {
            case ExprKind::typed_expr:
                return getDerived().TraverseTypedExpr(static_cast<TypedExpr*>(expr));
            case ExprKind::binary_expr:
                return getDerived().TraverseBinaryExpr(static_cast<BinaryExpr*>(expr));
            case ExprKind::unary_expr:
                return getDerived().TraverseUnaryExpr(static_cast<UnaryExpr*>(expr));
            case ExprKind::call_expr:
                return getDerived().TraverseCallExpr(static_cast<CallExpr*>(expr));
            case ExprKind::identifier_expr:
                return getDerived().TraverseDeclRefExpr(static_cast<DeclRefExpr*>(expr));
            case ExprKind::integer_literal_expr:
                return getDerived().TraverseIntegerLiteralExpr(static_cast<IntegerLiteralExpr*>(expr));
            case ExprKind::float_literal_expr:
                return getDerived().TraverseFloatLiteralExpr(static_cast<FloatLiteralExpr*>(expr));
            case ExprKind::string_literal_expr:
                return getDerived().TraverseStringLiteralExpr(static_cast<StringLiteralExpr*>(expr));
            case ExprKind::member_expr:
            case ExprKind::array_expr:
            case ExprKind::struct_expr:
            case ExprKind::enum_expr:
            case ExprKind::cast_expr:
                print_indent();
                std::cout << colorize(kErrorColor) << "- unhandled ExprKind" << colorize(kReset) << '\n';
                return true;
        }

        return true;
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseDecl(Decl* decl) {
        if (decl == nullptr) {
            print_indent();
            std::cout << colorize(kErrorColor) << "- null Decl" << colorize(kReset) << '\n';
            return true;
        }

        switch (decl->get_kind()) {
            case DeclKind::translation_unit:
                return getDerived().TraverseTranslationUnitDecl(static_cast<TranslationUnitDecl*>(decl));
            case DeclKind::named:
                return getDerived().TraverseNamedDecl(static_cast<NamedDecl*>(decl));
            case DeclKind::value:
                return getDerived().TraverseValueDecl(static_cast<ValueDecl*>(decl));
            case DeclKind::variable:
                return getDerived().TraverseVarDecl(static_cast<VarDecl*>(decl));
            case DeclKind::error:
                return getDerived().TraverseErrorDecl(static_cast<ErrorDecl*>(decl));
            case DeclKind::unresolved:
            case DeclKind::function:
            case DeclKind::struct_:
            case DeclKind::enum_:
            case DeclKind::module:
                print_indent();
                std::cout << colorize(kErrorColor) << "- unhandled DeclKind" << colorize(kReset) << '\n';
                return true;
        }

        return true;
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseStmt(Stmt* stmt) {
        if (stmt == nullptr) {
            print_indent();
            std::cout << colorize(kErrorColor) << "- null Stmt" << colorize(kReset) << '\n';
            return true;
        }

        switch (stmt->get_kind()) {
            case StmtKind::compound_stmt:
                return getDerived().TraverseCompoundStmt(static_cast<CompoundStmt*>(stmt));
            case StmtKind::if_stmt:
            case StmtKind::while_stmt:
            case StmtKind::for_stmt:
            case StmtKind::return_stmt:
                print_indent();
                std::cout << colorize(kErrorColor) << "- unhandled StmtKind" << colorize(kReset) << '\n';
                return true;
        }

        return true;
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseErrorExpr(ErrorExpr* expr) {
        return run_traverse(kExprColor, "ErrorExpr", expr,
            &Derived::VisitErrorExpr,
            &Derived::WalkUpFromErrorExpr);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseTypedExpr(TypedExpr* expr) {
        return run_traverse(kExprColor, "TypedExpr", expr,
            &Derived::VisitTypedExpr,
            &Derived::WalkUpFromTypedExpr);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseDeclRefExpr(DeclRefExpr* expr) {
        return run_traverse(kExprColor, "DeclRefExpr", expr,
            &Derived::VisitDeclRefExpr,
            &Derived::WalkUpFromDeclRefExpr);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseBinaryExpr(BinaryExpr* expr) {
        if (!run_traverse(kExprColor, "BinaryExpr", expr,
            &Derived::VisitBinaryExpr,
            &Derived::WalkUpFromBinaryExpr)) {
            return false;
        }

        if (!getDerived().TraverseExpr(expr->lhs)) {
            return false;
        }

        return getDerived().TraverseExpr(expr->rhs);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseUnaryExpr(UnaryExpr* expr) {
        if (!run_traverse(kExprColor, "UnaryExpr", expr,
            &Derived::VisitUnaryExpr,
            &Derived::WalkUpFromUnaryExpr)) {
            return false;
        }

        return getDerived().TraverseExpr(expr->operand);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseIntegerLiteralExpr(IntegerLiteralExpr* expr) {
        return run_traverse(kExprColor, "IntegerLiteralExpr", expr,
            &Derived::VisitIntegerLiteralExpr,
            &Derived::WalkUpFromIntegerLiteralExpr);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseFloatLiteralExpr(FloatLiteralExpr* expr) {
        return run_traverse(kExprColor, "FloatLiteralExpr", expr,
            &Derived::VisitFloatLiteralExpr,
            &Derived::WalkUpFromFloatLiteralExpr);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseStringLiteralExpr(StringLiteralExpr* expr) {
        return run_traverse(kExprColor, "StringLiteralExpr", expr,
            &Derived::VisitStringLiteralExpr,
            &Derived::WalkUpFromStringLiteralExpr);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseCallExpr(CallExpr* expr) {
        if (!run_traverse(kExprColor, "CallExpr", expr,
            &Derived::VisitCallExpr,
            &Derived::WalkUpFromCallExpr)) {
            return false;
        }

        if (!getDerived().TraverseExpr(expr->callee)) {
            return false;
        }

        for (unsigned i = 0; i < expr->num_args; ++i) {
            if (!getDerived().TraverseExpr(expr->args[i])) {
                return false;
            }
        }

        return true;
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseTranslationUnitDecl(TranslationUnitDecl* decl) {
        if (!run_traverse(kDeclColor, "TranslationUnitDecl", decl,
            &Derived::VisitTranslationUnitDecl,
            &Derived::WalkUpFromTranslationUnitDecl)) {
            return false;
        }

        for (Decl* current = decl->get_first_decl(); current != nullptr; current = current->next) {
            if (!getDerived().TraverseDecl(current)) {
                return false;
            }
        }

        return true;
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseNamedDecl(NamedDecl* decl) {
        return run_traverse(kDeclColor, "NamedDecl", decl,
            &Derived::VisitNamedDecl,
            &Derived::WalkUpFromNamedDecl);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseValueDecl(ValueDecl* decl) {
        return run_traverse(kDeclColor, "ValueDecl", decl,
            &Derived::VisitValueDecl,
            &Derived::WalkUpFromValueDecl);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseVarDecl(VarDecl* decl) {
        if (!run_traverse(kDeclColor, "VarDecl", decl,
            &Derived::VisitVarDecl,
            &Derived::WalkUpFromVarDecl)) {
            return false;
        }

        return getDerived().TraverseExpr(decl->get_init());
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseErrorDecl(ErrorDecl* decl) {
        return run_traverse(kDeclColor, "ErrorDecl", decl,
            &Derived::VisitErrorDecl,
            &Derived::WalkUpFromErrorDecl);
    }

    template <typename Derived>
    bool RecursiveAstVisitor<Derived>::TraverseCompoundStmt(CompoundStmt* stmt) {
        if (!run_traverse(kStmtColor, "CompoundStmt", stmt,
            &Derived::VisitCompoundStmt,
            &Derived::WalkUpFromCompoundStmt)) {
            return false;
        }

        for (Stmt* child : *stmt) {
            if (!getDerived().TraverseStmt(child)) {
                return false;
            }
        }

        return true;
    }
}

#endif //AION_RECURSIVE_AST_VISITOR_HPP