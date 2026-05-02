#include <support/global_constants.hpp>
#include <ast/ASTContext.hpp>
#include <ast/decl.hpp>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <cstdlib>

namespace aion::ast {

ASTContext::ASTContext(std::size_t initial_slab_size)
    : allocator(initial_slab_size), identifiers(*this) {
    tu_decl = create<TranslationUnitDecl>();
}

FuncDecl *ASTContext::create_func_decl(const std::string_view name, const std::size_t num_args, const std::size_t num_ret_vals) {
    if (name.empty()) {
        return nullptr;
    }

    IdentifierInfo *ident = emplace_or_get_identifier(name);

    std::size_t total_size = sizeof(FuncDecl) +
                             (num_args * sizeof(ParamVarDecl*)) +
                             (num_ret_vals * sizeof(MutableType*));

    void* storage = allocate(total_size, alignof(FuncDecl));
    FuncDecl* fn = new(storage) FuncDecl(ident, nullptr, false, num_args, num_ret_vals, SourceRange());


    if (num_args > 0) {
        std::memset(fn->get_params(), 0, num_args * sizeof(ParamVarDecl*));
    }

    if (num_ret_vals > 0) {
        std::memset(fn->get_return_types(), 0, num_ret_vals * sizeof(MutableType*));
    }

    return fn;
}

} // namespace aion::ast