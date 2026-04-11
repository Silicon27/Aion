//
// Created by David Yang on 2025-10-18.
//

#include <parser/parser.hpp>

namespace aion::parse {
    void Parser::parse_top_level_decl() {
        switch (peek().get_type()) {
            case TokenType::kw_let: {
                context.get_translation_unit_decl()->add_decl(parse_variable_decl());
                break;
            }
            case TokenType::comment:
            case TokenType::doc_comment: {
                blind_consume();
            }
            case TokenType::newline: {
                blind_consume();
                break;
            }
            default: {
                diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_unexpected_token);
                skip_until(TokenType::semicolon);
                context.get_translation_unit_decl()->add_decl(context.create<ErrorDecl>(Decl::DeclKind::variable, nullptr));
            }
        }
    }

    bool Parser::parse_first_top_level_decl() {
        /// TODO for module declaration
        return false;
    }

    Decl* Parser::parse_variable_decl() {
        // TODO: improve grammar-driven recovery around optional type annotation and initializer.

        static auto initial_let = MatchToken(TokenType::kw_let); // should not error if not matched, if error it could mean memory corruption during program runtime
        static auto mut = MatchToken(TokenType::kw_mut);
        static auto comp = MatchToken(TokenType::kw_comp);
        static auto variable_identifier = MatchToken(TokenType::identifier);
        static auto colon = MatchToken(TokenType::colon);
        static auto equal = MatchToken(TokenType::equal);
        static auto semicolon = MatchToken(TokenType::semicolon);

        SourceLocation decl_start_location = diagnostics.get_source_manager()->get_location(file_id, peek());
        Token variable_id_token;
        MutableType* type_annotation = nullptr;
        // TODO assuming all variables are stack for now until heap allocation syntax is finalized
        StorageClass storage_class;
        bool need_auto_type_deduction = false;
        bool is_mut = false;
        bool is_comp = false;
        Expr* expression = nullptr;

        // should always progress - if this faults, it indicates memory corruption during program runtime
        diffuse_match(initial_let.token, peek().type, "expected 'let' keyword - memory corruption possible");

        // check for attributes
        if (silent_probe(mut)) {
            is_mut = true;
            blind_consume();
        } else if (silent_probe(comp)) {
            is_comp = true;
            blind_consume();
        }

        // get the identifier
        if (silent_probe(variable_identifier)) {
            variable_id_token = blind_consume();
        } else {
            SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
            auto fixit_hint = diag::FixItHint::create_insertion(loc, "<identifier>");

            diagnostics.report(loc, diag::parse::err_expected_identifier)
                << "expected identifier for variable declaration, none provided."
                << fixit_hint;

            skip_until(TokenType::semicolon); // TODO integrate more advanced recovery functions
            return context.create<ErrorDecl>(Decl::DeclKind::variable, context.create<IdentifierInfo>(variable_id_token.lexeme.c_str()));
        }

        // check for early semicolons placed before types are specified
        if (silent_probe(semicolon)) {
            SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
            diagnostics.report(loc, diag::parse::err_untyped_uninitialized_variable_declaration)
                << "untyped, uninitialized variables are effectively non-existent, thereof not derivable of semantic value.";
        }

        if (silent_probe(equal)) {
            blind_consume();
            need_auto_type_deduction = true;
            goto expression_matching;
        }

        // type matching
        if (silent_probe(colon)) {
            blind_consume();
            type_annotation = match_type(is_mut);

            if (!type_annotation) {
                SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                auto fixit_hint = diag::FixItHint::create_insertion(loc, "<type>");
                diagnostics.report(loc, diag::parse::err_expected_type)
                    << "expected type for variable declaration, none provided."
                    << fixit_hint;
                skip_until(TokenType::semicolon);
                return context.create<ErrorDecl>(Decl::DeclKind::variable, context.create<IdentifierInfo>(variable_id_token.lexeme.c_str()));
            }
        }

        if (silent_probe(semicolon)) {
            blind_consume();
            if (is_comp) {
                // not initialized but is declared a comp -> constants cannot be uninitialized
                SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                auto fixit_hint = diag::FixItHint::create_insertion(loc, "<initializer or expression>");
                diagnostics.report(loc, diag::parse::err_expected_initialization)
                    << "expected initializer for variable attributed with comp, none provided."
                    << fixit_hint;
                skip_until(TokenType::semicolon);
                return context.create<ErrorDecl>(Decl::DeclKind::variable, context.create<IdentifierInfo>(variable_id_token.lexeme.c_str()));
            }
            goto ast_construction;
        }

        if (silent_probe(equal)) {
            blind_consume();
            goto expression_matching;
        }

        expression_matching:
        if (silent_probe(semicolon)) {
            SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
            auto fixit_hint = diag::FixItHint::create_insertion(loc, "<expr>");
            diagnostics.report(loc, diag::parse::err_expected_expression)
                << "expected initializer expression after '='."
                << fixit_hint;
            silent_consume(TokenType::semicolon);
            return context.create<ErrorDecl>(Decl::DeclKind::variable, context.create<IdentifierInfo>(variable_id_token.lexeme.c_str()));
        }
        expression = parse_expression(0, TokenType::semicolon);


        ast_construction:

        auto allocated_name = context.create<IdentifierInfo>(variable_id_token.lexeme.c_str());
        auto allocated_storage_class = context.create<StorageClass>(storage_class);
        auto allocated_range = context.create<SourceRange>(decl_start_location, SourceLocation(diagnostics.get_source_manager()->get_location(file_id, peek())));
        auto variable = context.create<VarDecl>(allocated_name, type_annotation, *allocated_storage_class, *allocated_range, expression);

        if (!silent_probe(semicolon)) {
            diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_expected_semicolon);
            skip_until(TokenType::semicolon);
            return context.create<ErrorDecl>(Decl::DeclKind::variable, context.create<IdentifierInfo>(variable_id_token.lexeme.c_str()));
        }
        silent_consume(TokenType::semicolon);
        return variable;
    }
}
