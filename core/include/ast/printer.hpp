//
// Created by David Yang on 2026-04-11.
//

#ifndef AION_PRINTER_HPP
#define AION_PRINTER_HPP

#include <ast/RecursiveAstVisitor.hpp>
#include <iostream>
#include <string>

namespace aion::ast {
    class AstPrinter : public RecursiveAstVisitor<AstPrinter> {
    public:
        int indent = 0;
        const SourceManager* sm = nullptr;

        static constexpr const char* kReset = "\x1b[0m";
        static constexpr const char* kBoldGreen = "\x1b[1;32m";
        static constexpr const char* kYellow = "\x1b[33m";
        static constexpr const char* kCyan = "\x1b[36m";
        static constexpr const char* kMagenta = "\x1b[35m";
        static constexpr const char* kGray = "\x1b[38;5;244m";

        static constexpr const char* kMemberLabelColor = "\x1b[38;5;246m";
        static constexpr const char* kMemberValueColor = "\x1b[38;5;250m";
        static constexpr const char* kMutedNodeColor = "\x1b[38;5;181m";

        explicit AstPrinter(const SourceManager* sm = nullptr) : sm(sm) {}

        static const char* decl_kind_name(const DeclKind kind) {
            switch (kind) {
                case DeclKind::unresolved: return "unresolved";
                case DeclKind::translation_unit: return "translation_unit";
                case DeclKind::named: return "named";
                case DeclKind::error: return "error";
                case DeclKind::value: return "value";
                case DeclKind::variable: return "variable";
                case DeclKind::function: return "function";
                case DeclKind::struct_: return "struct";
                case DeclKind::enum_: return "enum";
                case DeclKind::module: return "module";
            }
            return "unknown";
        }

        static const char* expr_kind_name(const ExprKind kind) {
            switch (kind) {
                case ExprKind::typed_expr: return "typed_expr";
                case ExprKind::binary_expr: return "binary_expr";
                case ExprKind::unary_expr: return "unary_expr";
                case ExprKind::call_expr: return "call_expr";
                case ExprKind::identifier_expr: return "identifier_expr";
                case ExprKind::integer_literal_expr: return "integer_literal_expr";
                case ExprKind::float_literal_expr: return "float_literal_expr";
                case ExprKind::string_literal_expr: return "string_literal_expr";
                case ExprKind::member_expr: return "member_expr";
                case ExprKind::array_expr: return "array_expr";
                case ExprKind::struct_expr: return "struct_expr";
                case ExprKind::enum_expr: return "enum_expr";
                case ExprKind::cast_expr: return "cast_expr";
            }
            return "unknown";
        }

        static const char* binary_op_name(const BinaryOp op) {
            switch (op) {
                case BinaryOp::add: return "+";
                case BinaryOp::sub: return "-";
                case BinaryOp::mul: return "*";
                case BinaryOp::div: return "/";
                case BinaryOp::mod: return "%";
                case BinaryOp::pow: return "**";
                case BinaryOp::equal: return "==";
                case BinaryOp::not_equal: return "!=";
                case BinaryOp::less: return "<";
                case BinaryOp::less_equal: return "<=";
                case BinaryOp::greater: return ">";
                case BinaryOp::greater_equal: return ">=";
                case BinaryOp::logical_and: return "&&";
                case BinaryOp::logical_or: return "||";
                case BinaryOp::bit_and: return "&";
                case BinaryOp::bit_or: return "|";
                case BinaryOp::bit_xor: return "^";
                case BinaryOp::lshift: return "<<";
                case BinaryOp::rshift: return ">>";
                case BinaryOp::assign: return "=";
                case BinaryOp::add_assign: return "+=";
                case BinaryOp::sub_assign: return "-=";
                case BinaryOp::mul_assign: return "*=";
                case BinaryOp::div_assign: return "/=";
                case BinaryOp::mod_assign: return "%=";
                case BinaryOp::and_assign: return "&=";
                case BinaryOp::or_assign: return "|=";
                case BinaryOp::xor_assign: return "^=";
                case BinaryOp::lshift_assign: return "<<=";
                case BinaryOp::rshift_assign: return ">>=";
                case BinaryOp::dot: return ".";
                case BinaryOp::arrow: return "->";
                case BinaryOp::scope_resolution: return "::";
                case BinaryOp::range: return "..";
                case BinaryOp::inclusive_range: return "...";
                case BinaryOp::fat_arrow: return "=>";
                case BinaryOp::as: return "as";
            }
            return "?";
        }

        static const char* unary_op_name(const UnaryOp op) {
            switch (op) {
                case UnaryOp::plus: return "+";
                case UnaryOp::minus: return "-";
                case UnaryOp::logical_not: return "!";
                case UnaryOp::bit_not: return "~";
                case UnaryOp::address_of: return "&";
                case UnaryOp::deref: return "*";
                case UnaryOp::preincrement: return "++(pre)";
                case UnaryOp::postincrement: return "++(post)";
                case UnaryOp::predecrement: return "--(pre)";
                case UnaryOp::postdecrement: return "--(post)";
            }
            return "?";
        }

        static const char* storage_class_name(const StorageClass storage) {
            switch (storage) {
                case StorageClass::stack: return "stack";
                case StorageClass::heap: return "heap";
                case StorageClass::persistent: return "persistent";
            }
            return "unknown";
        }

        static const char* value_category_name(const ValueCategory category) {
            switch (category) {
                case ValueCategory::named: return "named";
                case ValueCategory::unnamed: return "unnamed";
                case ValueCategory::invalid: return "invalid";
            }
            return "unknown";
        }

        static const char* type_kind_name(const TypeKind kind) {
            switch (kind) {
                case TypeKind::builtin: return "builtin";
                case TypeKind::user_defined: return "user_defined";
            }
            return "unknown";
        }

        std::string format_type(const MutableType* type) const {
            if (type == nullptr) {
                return "<null type>";
            }

            std::string result = type->is_mutable() ? "mut " : "imm ";
            const Type* base = type->get_base();
            if (base == nullptr) {
                result += "<null base>";
                return result;
            }

            if (base->get_kind() == TypeKind::user_defined) {
                const auto* ud = static_cast<const UserDefinedType*>(base);
                result += std::string(ud->get_name());
                return result;
            }

            result += type_kind_name(base->get_kind());
            return result;
        }

        static std::string format_identifier(const IdentifierInfo* ident) {
            if (ident == nullptr || ident->get_name() == nullptr) {
                return "<anonymous>";
            }
            return ident->get_name();
        }

        const char* detail_color(const char* color) const {
            return enable_colors ? color : "";
        }

        void print_member(const std::string& key, const std::string& value, const bool trailing_space = true) const {
            std::cout << detail_color(kMemberLabelColor) << key << detail_color(kReset)
                      << detail_color(kMemberValueColor) << value << detail_color(kReset);
            if (trailing_space) {
                std::cout << ' ';
            }
        }

        bool print(TranslationUnitDecl* tu_decl) {
            if (tu_decl == nullptr) {
                std::cout << colorize(kErrorColor) << "<null translation unit>" << colorize(kReset) << '\n';
                return false;
            }

            std::cout << colorize(kDeclColor) << "TranslationUnitDecl" << colorize(kReset) << '\n';
            for (Decl* current = tu_decl->get_first_decl(); current != nullptr; current = current->next) {
                ++indent;
                print_decl(current);
                --indent;
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

    private:
        void print_decl(Decl* decl) {
            if (decl == nullptr) {
                print_indent();
                std::cout << colorize(kErrorColor) << "<null decl>" << colorize(kReset) << '\n';
                return;
            }

            switch (decl->get_kind()) {
                case DeclKind::translation_unit:
                    print_indent();
                    std::cout << colorize(kDeclColor) << "TranslationUnitDecl" << colorize(kReset) << '\n';
                    break;

                case DeclKind::variable: {
                    auto* var = static_cast<VarDecl*>(decl);
                    print_indent();
                    std::cout << colorize(kDeclColor) << "VarDecl"
                              << colorize(kReset) << ' ';
                    print_member("name=", format_identifier(var->get_name()));
                    print_member("storage=", storage_class_name(var->get_storage_class()));
                    print_member("type=", format_type(var->get_type()));
                    print_member("has_init=", (var->get_init() != nullptr ? "true" : "false"), false);
                    std::cout << '\n';

                    if (var->get_init() != nullptr) {
                        ++indent;
                        print_indent();
                        std::cout << detail_color(kMemberLabelColor) << "init:" << colorize(kReset) << '\n';
                        ++indent;
                        print_expr(var->get_init());
                        --indent;
                        --indent;
                    }
                    break;
                }

                case DeclKind::error: {
                    auto* err = static_cast<ErrorDecl*>(decl);
                    print_indent();
                    std::cout << detail_color(kMutedNodeColor) << "ErrorDecl"
                              << colorize(kReset) << ' ';
                    print_member("name=", format_identifier(err->get_name()), false);
                    std::cout << '\n';
                    break;
                }

                case DeclKind::named: {
                    auto* named = static_cast<NamedDecl*>(decl);
                    print_indent();
                    std::cout << colorize(kDeclColor) << "NamedDecl"
                              << colorize(kReset) << ' ';
                    print_member("name=", format_identifier(named->get_name()), false);
                    std::cout << '\n';
                    break;
                }

                case DeclKind::value: {
                    auto* value = static_cast<ValueDecl*>(decl);
                    print_indent();
                    std::cout << colorize(kDeclColor) << "ValueDecl"
                              << colorize(kReset) << ' ';
                    print_member("name=", format_identifier(value->get_name()));
                    print_member("type=", format_type(value->get_type()), false);
                    std::cout << '\n';
                    break;
                }

                default:
                    print_indent();
                    std::cout << colorize(kDeclColor) << "Decl"
                              << colorize(kReset) << ' ';
                    print_member("kind=", decl_kind_name(decl->get_kind()), false);
                    std::cout << '\n';
                    break;
            }
        }

        void print_expr(Expr* expr) {
            if (expr == nullptr) {
                print_indent();
                std::cout << colorize(kErrorColor) << "<null expr>" << colorize(kReset) << '\n';
                return;
            }

            switch (expr->get_kind()) {
                case ExprKind::binary_expr: {
                    auto* bin = static_cast<BinaryExpr*>(expr);
                    print_indent();
                    std::cout << colorize(kExprColor) << "BinaryExpr"
                              << colorize(kReset) << ' ';
                    print_member("op=", binary_op_name(bin->op));
                    print_member("category=", value_category_name(bin->get_category()));
                    print_member("comp=", (bin->is_comp ? "true" : "false"));
                    print_member("type=", format_type(bin->get_type()), false);
                    std::cout << '\n';

                    ++indent;
                    print_indent();
                    std::cout << detail_color(kMemberLabelColor) << "lhs:" << colorize(kReset) << '\n';
                    ++indent;
                    print_expr(bin->lhs);
                    --indent;

                    print_indent();
                    std::cout << detail_color(kMemberLabelColor) << "rhs:" << colorize(kReset) << '\n';
                    ++indent;
                    print_expr(bin->rhs);
                    --indent;
                    --indent;
                    break;
                }

                case ExprKind::unary_expr: {
                    auto* unary = static_cast<UnaryExpr*>(expr);
                    print_indent();
                    std::cout << colorize(kExprColor) << "UnaryExpr"
                              << colorize(kReset) << ' ';
                    print_member("op=", unary_op_name(unary->get_op()));
                    print_member("category=", value_category_name(unary->get_category()));
                    print_member("comp=", (unary->is_compile_time_computable() ? "true" : "false"));
                    print_member("type=", format_type(unary->get_type()), false);
                    std::cout << '\n';

                    ++indent;
                    print_indent();
                    std::cout << detail_color(kMemberLabelColor) << "operand:" << colorize(kReset) << '\n';
                    ++indent;
                    print_expr(unary->get_operand());
                    --indent;
                    --indent;
                    break;
                }

                case ExprKind::identifier_expr: {
                    auto* ref = static_cast<DeclRefExpr*>(expr);
                    print_indent();
                    std::cout << colorize(kExprColor) << "DeclRefExpr"
                              << colorize(kReset) << ' ';
                    print_member("name=", (ref->decl ? format_identifier(ref->decl->get_name()) : "<unbound>"));
                    print_member("category=", value_category_name(ref->get_category()));
                    print_member("type=", format_type(ref->get_type()), false);
                    std::cout << '\n';
                    break;
                }

                case ExprKind::call_expr: {
                    auto* call = static_cast<CallExpr*>(expr);
                    print_indent();
                    std::cout << colorize(kExprColor) << "CallExpr"
                              << colorize(kReset) << ' ';
                    print_member("target=", (call->decl ? format_identifier(call->decl->get_name()) : "<unknown>"));
                    print_member("args=", std::to_string(call->num_args));
                    print_member("type=", format_type(call->get_type()), false);
                    std::cout << '\n';

                    ++indent;
                    print_indent();
                    std::cout << detail_color(kMemberLabelColor) << "callee:" << colorize(kReset) << '\n';
                    ++indent;
                    print_expr(call->callee);
                    --indent;

                    for (unsigned i = 0; i < call->num_args; ++i) {
                        print_indent();
                        std::cout << detail_color(kMemberLabelColor) << "arg[" << i << "]:" << colorize(kReset) << '\n';
                        ++indent;
                        print_expr(call->args[i]);
                        --indent;
                    }
                    --indent;
                    break;
                }

                case ExprKind::integer_literal_expr: {
                    auto* lit = static_cast<IntegerLiteralExpr*>(expr);
                    print_indent();
                    std::cout << colorize(kExprColor) << "IntegerLiteralExpr"
                              << colorize(kReset) << ' ';
                    print_member("value=", std::string(lit->value));
                    print_member("type=", format_type(lit->get_type()), false);
                    std::cout << '\n';
                    break;
                }

                case ExprKind::float_literal_expr: {
                    auto* lit = static_cast<FloatLiteralExpr*>(expr);
                    print_indent();
                    std::cout << colorize(kExprColor) << "FloatLiteralExpr"
                              << colorize(kReset) << ' ';
                    print_member("value=", std::string(lit->value));
                    print_member("type=", format_type(lit->get_type()), false);
                    std::cout << '\n';
                    break;
                }

                case ExprKind::string_literal_expr: {
                    auto* lit = static_cast<StringLiteralExpr*>(expr);
                    print_indent();
                    std::cout << colorize(kExprColor) << "StringLiteralExpr"
                              << colorize(kReset) << ' ';
                    print_member("value=", "\"" + std::string(lit->value) + "\"");
                    print_member("flags=", std::to_string(static_cast<unsigned>(lit->prefix_flags)));
                    print_member("type=", format_type(lit->get_type()), false);
                    std::cout << '\n';
                    break;
                }

                case ExprKind::typed_expr: {
                    auto* typed = static_cast<TypedExpr*>(expr);
                    print_indent();
                    std::cout << colorize(kExprColor) << "TypedExpr"
                              << colorize(kReset) << ' ';
                    print_member("category=", value_category_name(typed->get_category()));
                    print_member("type=", format_type(typed->get_type()), false);
                    std::cout << '\n';
                    break;
                }

                default:
                    print_indent();
                    std::cout << colorize(kExprColor) << "Expr"
                              << colorize(kReset) << ' ';
                    print_member("kind=", expr_kind_name(expr->get_kind()));
                    print_member("category=", value_category_name(expr->get_category()), false);
                    std::cout << '\n';
                    break;
            }
        }
    };
}

#endif //AION_PRINTER_HPP
