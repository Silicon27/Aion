//
// Created by David Yang on 2026-03-17.
//

#ifndef AION_STMT_HPP
#define AION_STMT_HPP

#include <ast/ast.hpp>
#include <ast/ASTContext.hpp>

namespace aion::ast {
    class CompoundStmt;

    /// For statement chaining
    class CompoundStmt final : public Stmt {
        std::uint32_t num_stmts = 0;

        explicit CompoundStmt(std::uint32_t num_stmts)
            : Stmt(Kind::compound_stmt), num_stmts(num_stmts) {}

    public:
        static CompoundStmt* create(ASTContext& context, Stmt** stmts, std::uint32_t num_stmts);

        using iterator = Stmt**;
        using const_iterator = Stmt* const*;

        [[nodiscard]] std::uint32_t size() const { return num_stmts; }
        [[nodiscard]] Stmt** get_stmts() { return reinterpret_cast<Stmt**>(this + 1); }
        [[nodiscard]] Stmt* const* get_stmts() const { return reinterpret_cast<Stmt* const*>(this + 1); }

        [[nodiscard]] iterator begin() { return get_stmts(); }
        [[nodiscard]] iterator end() { return get_stmts() + num_stmts; }

        [[nodiscard]] const_iterator begin() const { return get_stmts(); }
        [[nodiscard]] const_iterator end() const { return get_stmts() + num_stmts; }
    };
    static_assert(std::is_trivially_destructible_v<CompoundStmt>);
}

#endif //AION_STMT_HPP