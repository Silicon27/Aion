//
// Created by David Yang on 2026-02-08.
//

#ifndef AION_AST_CONTEXT_HPP
#define AION_AST_CONTEXT_HPP

#include <vector>
#include <deque>
#include <memory>
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <string_view>
#include <new>
#include <cstring>
#include <support/si/include/si/memory.hpp>
#include <support/si/include/si/string.hpp>
#include <ast/StringMap.hpp>
#include <ast/ShortVec.hpp>
#include <ast/ast.hpp>
#include <ast/decl.hpp>
#include <error/error.hpp>

namespace aion::ast {
    class ASTContext {
    public:
        using BumpPtrAllocator = si::bump_ptr_allocator<>;

        template<typename Value>
        using StringMap = StringMap<Value>;

    private:
        BumpPtrAllocator allocator;
        TranslationUnitDecl *tu_decl;
        StringMap<IdentifierInfo *> identifiers;

    public:
        explicit ASTContext(std::size_t initial_slab_size = 1024 * 1024);

        [[nodiscard]] TranslationUnitDecl *get_translation_unit_decl() const { return tu_decl; }
        void set_translation_unit_decl(TranslationUnitDecl *tu_decl) { this->tu_decl = tu_decl; }
        [[nodiscard]] const StringMap<IdentifierInfo *>& get_identifiers() const { return identifiers; }

        [[nodiscard]] IdentifierInfo* emplace_or_get_identifier(const std::string_view name) {
            auto &info = identifiers.emplace_or_get(name, nullptr);
            if (!info) {
                info = create<IdentifierInfo>(allocate_string(name), name.size());
            }
            return info;
        }

        [[nodiscard]] IdentifierInfo* get_identifier(const std::string_view name) {
            const auto it = identifiers.find(name);
            if (!it) {
                return nullptr;
            }
            return *it;
        }

        void *allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
            return allocator.allocate(size, alignment);
        }

        template<typename T, typename... Args>
            requires (std::is_trivially_destructible_v<T> 
                && std::is_constructible_v<T, Args...>)
        T *create(Args &&... args) {
            void *storage = allocate(sizeof(T), alignof(T));
            return new(storage) T(std::forward<Args>(args)...);
        }

        // ––––––––––––––––––––––––––––––––––––––––––
        // custom allocation functions for non-trivially allocatable types
        // (ex. types that may have a pointer as an array in their members)
        // ––––––––––––––––––––––––––––––––––––––––––

        FuncDecl *create_func_decl(IdentifierInfo* name, MutableType* type, bool is_export, std::size_t num_params, const SourceRange& range);
        FunctionType* create_function_type(MutableType* return_type, const std::vector<MutableType*>& param_types);
        FunctionType* create_function_type(MutableType* return_type, const ShortVec<MutableType*>& param_types);

        char *allocate_string(std::string_view str) {
            if (str.empty()) return nullptr;
            char *storage = static_cast<char *>(allocate(str.size() + 1, 1));
            std::ranges::copy(str, storage);
            storage[str.size()] = '\0';
            return storage;
        }
    };
} // namespace aion::ast

#endif //AION_AST_CONTEXT_HPP
