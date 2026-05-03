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

FuncDecl *ASTContext::create_func_decl(IdentifierInfo* name, MutableType* type, bool is_export, std::size_t num_params, const SourceRange& range) {
    std::size_t total_size = sizeof(FuncDecl) + (num_params * sizeof(ParamVarDecl*));

    void* storage = allocate(total_size, alignof(FuncDecl));
    FuncDecl* fn = new(storage) FuncDecl(name, type, is_export, num_params, range);

    if (num_params > 0) {
        std::memset(fn->get_params(), 0, num_params * sizeof(ParamVarDecl*));
    }

    return fn;
}

FunctionType* ASTContext::create_function_type(MutableType* return_type, const std::vector<MutableType*>& param_types) {
    std::size_t num_params = param_types.size();
    std::size_t total_size = sizeof(FunctionType) + (num_params * sizeof(MutableType*));
    void* storage = allocate(total_size, alignof(FunctionType));
    FunctionType* ft = new(storage) FunctionType(return_type, num_params);
    if (num_params > 0) {
        std::memcpy(ft->get_param_types(), param_types.data(), num_params * sizeof(MutableType*));
    }
    return ft;
}

FunctionType* ASTContext::create_function_type(MutableType* return_type, const ShortVec<MutableType*>& param_types) {
    std::size_t num_params = param_types.size();
    std::size_t total_size = sizeof(FunctionType) + (num_params * sizeof(MutableType*));
    void* storage = allocate(total_size, alignof(FunctionType));
    FunctionType* ft = new(storage) FunctionType(return_type, num_params);
    for (std::size_t i = 0; i < num_params; ++i) {
        ft->get_param_types()[i] = param_types[i];
    }
    return ft;
}

} // namespace aion::ast