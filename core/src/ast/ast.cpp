//
// Created by David Yang on 2025-11-12.
//

#include <ast/ast.hpp>
#include <ast/ASTContext.hpp>
#include <cstring>

namespace aion::ast {
    void DeclContext::add_decl(Decl *decl) {
        if (!first_decl) {
            first_decl = last_decl = decl;
        } else {
            last_decl->next = decl;
            last_decl = decl;
        }
    }

    /**
     * Creates a `CompoundStmt` instance by allocating memory for it within the specified `ASTContext`
     * and initializing it with the given statements.
     *
     * @param C The `ASTContext` used for allocating memory for the `CompoundStmt` instance.
     * @param stmts Array of pointers to `Stmt` objects that will be included in the `CompoundStmt`.
     * @param num_stmts The number of statements in the `stmts` array.
     * @return A pointer to the newly created `CompoundStmt` instance containing the provided statements.
     */
    CompoundStmt* CompoundStmt::create(ASTContext& context, Stmt** stmts, std::uint32_t num_stmts) {
        const std::size_t size = sizeof(CompoundStmt) + num_stmts * sizeof(Stmt*);
        void* storage = context.allocate(size, alignof(CompoundStmt));
        auto* compound_stmt = new (storage) CompoundStmt(num_stmts);
        if (num_stmts > 0) {
            std::memcpy(compound_stmt->get_stmts(), stmts, num_stmts * sizeof(Stmt*));
        }
        return compound_stmt;
    }
}
