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

        void print_address(const void* ptr) const {
            if (!ptr) return;
            std::cout << ' ' << colorize(kYellow) << ptr << colorize(kReset);
        }

        void print_location(const SourceLocation& loc) const {
            if (!sm || loc.is_invalid()) return;
            auto lc = sm->get_line_column(loc);
            std::cout << ' ' << colorize(kGray) << "<line:" << lc.first << ":" << lc.second << ">" << colorize(kReset);
        }

        void print_range(const SourceRange& range) const {
            if (!sm || !range.is_valid()) return;
            auto start = sm->get_line_column(range.begin);
            auto end = sm->get_line_column(range.end);
            std::cout << ' ' << colorize(kGray) << '<';
            if (start.first == end.first) {
                std::cout << "line:" << start.first << ":" << start.second << ", col:" << end.second;
            } else {
                std::cout << "line:" << start.first << ":" << start.second << ", line:" << end.first << ":" << end.second;
            }
            std::cout << '>' << colorize(kReset);
        }

        static const char* decl_kind_name(const DeclKind kind) {
            switch (kind) {
                case DeclKind::variable: return "VarDecl";
                case DeclKind::translation_unit: return "TranslationUnitDecl";
                case DeclKind::named: return "NamedDecl";
                case DeclKind::error: return "ErrorDecl";
                case DeclKind::value: return "ValueDecl";
                case DeclKind::function: return "FunctionDecl";
                case DeclKind::struct_: return "RecordDecl";
                case DeclKind::enum_: return "EnumDecl";
                case DeclKind::module: return "ModuleDecl";
                default: return "Decl";
            }
        }

        static const char* expr_kind_name(const ExprKind kind) {
            switch (kind) {
                case ExprKind::binary_expr: return "BinaryOperator";
                case ExprKind::unary_expr: return "UnaryOperator";
                case ExprKind::call_expr: return "CallExpr";
                case ExprKind::identifier_expr: return "DeclRefExpr";
                case ExprKind::integer_literal_expr: return "IntegerLiteral";
                case ExprKind::float_literal_expr: return "FloatingLiteral";
                case ExprKind::string_literal_expr: return "StringLiteral";
                case ExprKind::typed_expr: return "TypedExpr";
                case ExprKind::member_expr: return "MemberExpr";
                case ExprKind::array_expr: return "ArrayExpr";
                case ExprKind::struct_expr: return "InitListExpr";
                default: return "Expr";
            }
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

            std::cout << colorize(kBoldGreen) << "TranslationUnitDecl" << colorize(kReset);
            print_address(tu_decl);
            print_range(tu_decl->source_range);
            std::cout << '\n';

            Decl* current = tu_decl->get_first_decl();
            while (current) {
                Decl* next = current->next;
                print_decl(current, "", next == nullptr);
                current = next;
            }
            return true;
        }

        void print_decl(Decl* decl, const std::string& prefix, bool is_last) {
            if (decl == nullptr) {
                std::cout << prefix << (is_last ? "`- " : "|- ") << colorize(kErrorColor) << "<null decl>" << colorize(kReset) << '\n';
                return;
            }

            std::cout << prefix << (is_last ? "`- " : "|- ");
            std::cout << colorize(kBoldGreen) << decl_kind_name(decl->get_kind()) << colorize(kReset);
            print_address(decl);
            print_range(decl->source_range);

            switch (decl->get_kind()) {
                case DeclKind::variable: {
                    auto* var = static_cast<VarDecl*>(decl);
                    std::cout << " " << colorize(kCyan) << var->get_identifier() << colorize(kReset);
                    std::cout << " " << colorize(kCyan) << " '" << format_type(var->get_type()) << "'" << colorize(kReset);
                    std::cout << '\n';

                    if (var->get_init() != nullptr) {
                        print_expr(var->get_init(), prefix + (is_last ? "  " : "| "), true);
                    }
                    break;
                }
                case DeclKind::error: {
                    auto* err = static_cast<ErrorDecl*>(decl);
                    std::cout << " " << colorize(kCyan) << format_identifier(err->get_name()) << colorize(kReset) << '\n';
                    break;
                }
                case DeclKind::named: {
                    auto* named = static_cast<NamedDecl*>(decl);
                    std::cout << " " << colorize(kCyan) << format_identifier(named->get_name()) << colorize(kReset) << '\n';
                    break;
                }
                case DeclKind::value: {
                    auto* value = static_cast<ValueDecl*>(decl);
                    std::cout << " " << colorize(kCyan) << format_identifier(value->get_name()) << colorize(kReset);
                    std::cout << " " << colorize(kCyan) << " '" << format_type(value->get_type()) << "'" << colorize(kReset) << '\n';
                    break;
                }
                default:
                    std::cout << '\n';
                    break;
            }
        }

        void print_expr(Expr* expr, const std::string& prefix, bool is_last) {
            if (expr == nullptr) {
                std::cout << prefix << (is_last ? "`- " : "|- ") << colorize(kErrorColor) << "<null expr>" << colorize(kReset) << '\n';
                return;
            }

            std::cout << prefix << (is_last ? "`- " : "|- ");
            std::cout << colorize(kBoldGreen) << expr_kind_name(expr->get_kind()) << colorize(kReset);
            print_address(expr);

            switch (expr->get_kind()) {
                case ExprKind::binary_expr: {
                    auto* bin = static_cast<BinaryExpr*>(expr);
                    print_range(bin->range);
                    std::cout << " " << colorize(kCyan) << "'" << format_type(bin->get_type()) << "'" << colorize(kReset);
                    std::cout << " " << colorize(kMagenta) << " '" << binary_op_name(bin->op) << "'" << colorize(kReset) << '\n';
                    std::string next_prefix = prefix + (is_last ? "  " : "| ");
                    print_expr(bin->lhs, next_prefix, false);
                    print_expr(bin->rhs, next_prefix, true);
                    break;
                }
                case ExprKind::unary_expr: {
                    auto* unary = static_cast<UnaryExpr*>(expr);
                    print_location(unary->loc);
                    std::cout << " " << colorize(kCyan) << "'" << format_type(unary->get_type()) << "'" << colorize(kReset);
                    std::cout << " " << colorize(kMagenta) << " '" << unary_op_name(unary->get_op()) << "'" << colorize(kReset) << '\n';
                    print_expr(unary->get_operand(), prefix + (is_last ? "  " : "| "), true);
                    break;
                }
                case ExprKind::identifier_expr: {
                    auto* ref = static_cast<DeclRefExpr*>(expr);
                    print_location(ref->loc);
                    std::cout << " " << colorize(kCyan) << "'" << format_type(ref->get_type()) << "'" << colorize(kReset);
                    std::cout << " " << colorize(kCyan) << " " << (ref->decl ? format_identifier(ref->decl->get_name()) : "<unbound>") << colorize(kReset) << '\n';
                    break;
                }
                case ExprKind::call_expr: {
                    auto* call = static_cast<CallExpr*>(expr);
                    std::cout << " " << colorize(kCyan) << "'" << format_type(call->get_type()) << "'" << colorize(kReset) << '\n';
                    std::string next_prefix = prefix + (is_last ? "  " : "| ");
                    print_expr(call->callee, next_prefix, call->num_args == 0);
                    for (unsigned i = 0; i < call->num_args; ++i) {
                        print_expr(call->args[i], next_prefix, i == call->num_args - 1);
                    }
                    break;
                }
                case ExprKind::integer_literal_expr: {
                    auto* lit = static_cast<IntegerLiteralExpr*>(expr);
                    print_location(lit->loc);
                    std::cout << " " << colorize(kCyan) << "'" << format_type(lit->get_type()) << "'" << colorize(kReset);
                    std::cout << " " << colorize(kCyan) << " " << lit->value << colorize(kReset) << '\n';
                    break;
                }
                case ExprKind::float_literal_expr: {
                    auto* lit = static_cast<FloatLiteralExpr*>(expr);
                    print_location(lit->loc);
                    std::cout << " " << colorize(kCyan) << "'" << format_type(lit->get_type()) << "'" << colorize(kReset);
                    std::cout << " " << colorize(kCyan) << " " << lit->value << colorize(kReset) << '\n';
                    break;
                }
                case ExprKind::string_literal_expr: {
                    auto* lit = static_cast<StringLiteralExpr*>(expr);
                    print_location(lit->loc);
                    std::cout << " " << colorize(kCyan) << "'" << format_type(lit->get_type()) << "'" << colorize(kReset);
                    std::cout << " " << colorize(kCyan) << " \"" << lit->value << "\"" << colorize(kReset) << '\n';
                    break;
                }
                case ExprKind::typed_expr: {
                    auto* typed = static_cast<TypedExpr*>(expr);
                    std::cout << " " << colorize(kCyan) << "'" << format_type(typed->get_type()) << "'" << colorize(kReset) << '\n';
                    break;
                }
                default:
                    std::cout << '\n';
                    break;
            }
        }
    };
}

#endif //AION_PRINTER_HPP
