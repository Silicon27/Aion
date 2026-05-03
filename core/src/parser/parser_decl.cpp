//
// Created by David Yang on 2025-10-18.
//

#include <parser/parser.hpp>

namespace aion::parse {
    void Parser::parse_top_level_decl() {
        bool is_export = false;
        if (silent_probe(TokenType::kw_export)) {
            is_export = true;
            blind_consume();
        }

        switch (peek().get_type()) {
            case TokenType::kw_let: {
                Decl* decl = parse_variable_decl();
                if (is_export) {
                    // TODO: VarDecl might need is_export too if it's allowed
                }
                context.get_translation_unit_decl()->add_decl(decl);
                break;
            }
            case TokenType::kw_fn: {
                Decl* decl = parse_function_decl();
                if (is_export && decl->get_kind() == DeclKind::function) {
                    static_cast<FuncDecl*>(decl)->is_export = true;
                }
                context.get_translation_unit_decl()->add_decl(decl);
                break;
            }
            case TokenType::comment:
            case TokenType::doc_comment: {
                blind_consume();
                break;
            }
            case TokenType::newline: {
                blind_consume();
                break;
            }
            case TokenType::semicolon: {
                // consume for now, assumptions on why it may be there can come later;;
                blind_consume();
                break;
            }
            case TokenType::kw_if: {
                diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_unexpected_token)
                << diag::note("if statements cannot exist at top-level");
                skip_until(TokenType::semicolon);
                context.get_translation_unit_decl()->add_decl(context.create<ErrorDecl>(nullptr, SourceRange()));
                break;
            }
            default: {
                diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_unexpected_token);
                skip_until(TokenType::semicolon);
                context.get_translation_unit_decl()->add_decl(context.create<ErrorDecl>(nullptr, SourceRange()));
                break;
            }
        }
    }

    bool Parser::parse_first_top_level_decl() {
        /// TODO for module declaration
        return false;
    }

    Decl* Parser::parse_variable_decl() {
        auto initial_let = MatchToken(TokenType::kw_let); // should not error if not matched, if error it could mean memory corruption during program runtime
        auto mut = MatchToken(TokenType::kw_mut);
        auto comp = MatchToken(TokenType::kw_comp);
        auto variable_identifier = MatchToken(TokenType::identifier);
        auto colon = MatchToken(TokenType::colon);
        auto equal = MatchToken(TokenType::equal);
        auto semicolon = MatchToken(TokenType::semicolon);

        SourceLocation decl_start_location = diagnostics.get_source_manager()->get_location(file_id, peek());
        Token variable_id_token;
        IdentifierInfo* variable_identifier_info = nullptr;
        MutableType* type_annotation = nullptr;
        // TODO assuming all variables are stack for now until heap allocation syntax is finalized
        StorageClass storage_class;
        bool need_auto_type_deduction = false;
        bool is_mut = false;
        bool is_comp = false; // TODO dont think this is used; if not, make it a part of a MutableType derived that includes a member for comp
        Expr* expression = nullptr;

        // should always progress - if this faults, it indicates memory corruption during program runtime
        diffuse_match(initial_let.token, peek().type, "expected 'let' keyword - memory corruption possible");

        // check for attributes
        if (silent_probe(mut)) {
            is_mut = true;
            blind_consume();
            if (silent_probe(comp)) {
                is_comp = true;
                blind_consume();
            }
        } else if (silent_probe(comp)) {
            is_comp = true;
            blind_consume();
            if (silent_probe(mut)) {
                is_mut = true;
                blind_consume();
            }
        }

        // get the identifier
        if (silent_probe(variable_identifier)) {
            variable_id_token = blind_consume();
            variable_identifier_info = context.get_identifier(variable_id_token.lexeme);
            if (variable_identifier_info != nullptr) {
                diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, variable_id_token), diag::parse::err_redefinition);
            }
            variable_identifier_info = context.emplace_or_get_identifier(variable_id_token.lexeme);
        } else {
            SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
            auto fixit_hint = diag::FixItHint::create_insertion(loc, "<identifier>");

            diagnostics.report(loc, diag::parse::err_expected_identifier)
                << "expected identifier for variable declaration, none provided."
                << fixit_hint;

            skip_until(TokenType::semicolon); // TODO integrate more advanced recovery functions
            return context.create<ErrorDecl>(
                context.emplace_or_get_identifier("<missing_identifier>"),
                SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
            );
        }

        // check for early semicolons placed before types are specified
        if (silent_probe(semicolon)) {
            SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
            auto fixit_hint = diag::FixItHint::create_insertion(loc, "<type or initalizer>");
            diagnostics.report(loc, diag::parse::err_untyped_uninitialized_variable_declaration)
                << "untyped, uninitialized variables are effectively non-existent, thereof not derivable of semantic value."
                << fixit_hint;
            blind_consume(); // consume this semicolon as it effectively ends the declaration
            return context.create<ErrorDecl>(
                variable_identifier_info,
                SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
            );
        }

        if (silent_probe(equal)) {
            blind_consume();
            need_auto_type_deduction = true;
            goto expression_matching;
        }

        // type matching
        if (silent_probe(colon)) {
            blind_consume();
            type_annotation = match_type(is_mut, is_comp);

            if (!type_annotation) {
                SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                auto fixit_hint = diag::FixItHint::create_insertion(loc, "<type>");
                diagnostics.report(loc, diag::parse::err_expected_type)
                    << "expected type for variable declaration, none provided."
                    << fixit_hint;
                skip_until(TokenType::semicolon);
                return context.create<ErrorDecl>(
                    variable_identifier_info,
                    SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
                );
            }
        }

        if (silent_probe(semicolon)) {
            if (is_comp) {
                // not initialized but is declared a comp -> constants cannot be uninitialized
                SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                auto fixit_hint = diag::FixItHint::create_insertion(loc, "<initializer or expression>");
                diagnostics.report(loc, diag::parse::err_expected_initialization)
                    << "expected initializer for variable attributed with comp, none provided."
                    << fixit_hint;
                skip_until(TokenType::semicolon);
                return context.create<ErrorDecl>(
                    variable_identifier_info,
                    SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
                );
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
                << fixit_hint
                << diag::fixit_message("insert an initializer expression before ';'");
            silent_consume(TokenType::semicolon);
            return context.create<ErrorDecl>(
                variable_identifier_info,
                SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
            );
        }
        expression = parse_expression(0, TokenType::semicolon);


        ast_construction:

        auto allocated_name = variable_identifier_info;
        auto allocated_storage_class = context.create<StorageClass>(storage_class);
        auto allocated_range = context.create<SourceRange>(decl_start_location, SourceLocation(diagnostics.get_source_manager()->get_location(file_id, peek())));
        auto variable = context.create<VarDecl>(allocated_name, type_annotation, *allocated_storage_class, *allocated_range, expression);

        if (!silent_probe(semicolon)) {
            diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_expected_semicolon);
            skip_until(TokenType::semicolon);
            return context.create<ErrorDecl>(
                variable_identifier_info,
                SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
            );
        }
        silent_consume(TokenType::semicolon);
        return variable;
    }

    Decl* Parser::parse_function_decl() {
        auto lparen = MatchToken(TokenType::lparen);
        auto rparen = MatchToken(TokenType::rparen);
        auto identifier = MatchToken(TokenType::identifier);
        auto arrow = MatchToken(TokenType::arrow);
        auto semicolon = MatchToken(TokenType::semicolon);

        SourceLocation decl_start_location = diagnostics.get_source_manager()->get_location(file_id, peek());
        
        diffuse_match(TokenType::kw_fn, peek().type, "expected 'fn' keyword");

        bool fn_is_mut = false;
        bool fn_is_comp = false;

        if (silent_probe(TokenType::kw_mut)) {
            fn_is_mut = true;
            blind_consume();
            if (silent_probe(TokenType::kw_comp)) {
                fn_is_comp = true;
                blind_consume();
            }
        } else if (silent_probe(TokenType::kw_comp)) {
            fn_is_comp = true;
            blind_consume();
            if (silent_probe(TokenType::kw_mut)) {
                fn_is_mut = true;
                blind_consume();
            }
        }

        IdentifierInfo* function_id_info = nullptr;
        Token function_id_token;
        if (silent_probe(identifier)) {
            function_id_token = blind_consume();
            function_id_info = context.emplace_or_get_identifier(function_id_token.lexeme);
        } else {
            SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
            auto fixit_hint = diag::FixItHint::create_insertion(loc, "<identifier>");
            diagnostics.report(loc, diag::parse::err_expected_identifier)
                << "expected identifier for function declaration, none provided."
                << fixit_hint;
            skip_until(TokenType::semicolon);
            return context.create<ErrorDecl>(
                context.emplace_or_get_identifier("<missing_identifier>"),
                SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
            );
        }

        ast::ShortVec<ParamVarDecl*> collected_params(context);
        ast::ShortVec<MutableType*> param_types(context);

        auto parse_argument = [&]() -> Decl* {
            bool arg_is_mut = false;
            Token id;
            MutableType* arg_type = nullptr;
            Expr* default_value = nullptr;
            if (peek().type == TokenType::identifier) {
                id = blind_consume();
            } else {
                SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                auto fixit_hint = diag::FixItHint::create_insertion(loc, "<identifier>");
                diagnostics.report(loc, diag::parse::err_expected_identifier)
                    << "expected identifier for function argument, none provided."
                    << fixit_hint;
                skip_until_any_of(TokenType::comma, TokenType::rparen, TokenType::semicolon);
                return context.create<ErrorDecl>(nullptr, SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek())));
            }

            if (peek().type == TokenType::colon) {
                blind_consume(); // consume colon
            } else {
                SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                diagnostics.report(loc, diag::parse::err_expected_type)
                    << "expected colon for explicit type declaration for function parameter"
                    << diag::FixItHint::create_insertion(loc, "<colon>");
            }

            if (silent_probe(TokenType::kw_mut)) {
                arg_is_mut = true;
                blind_consume();
            }
            if (silent_probe(TokenType::kw_comp)) {
                diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_unexpected_attribute)
                    << "parameters cannot be declared as comp; if you meant to declare a compile time function use fn comp";
                blind_consume();
            }

            arg_type = match_type(arg_is_mut);

            if (silent_probe(TokenType::equal)) {
                blind_consume();
                default_value = parse_expression(0, TokenType::comma, TokenType::rparen);
            }

            return context.create<ParamVarDecl>(
                context.emplace_or_get_identifier(id.lexeme),
                arg_type,
                SourceRange(diagnostics.get_source_manager()->get_location(file_id, id), diagnostics.get_source_manager()->get_location(file_id, peek())),
                default_value
            );
        };

        if (silent_consume(lparen).type != TokenType::invalid_token) {
            while (true) {
                skip_newlines();
                if (peek().type == TokenType::rparen) break;
                if (peek().type == TokenType::eof) break;

                Decl* argument = parse_argument();
                if (argument->get_kind() == DeclKind::error) return argument;

                auto* param = static_cast<ParamVarDecl*>(argument);
                collected_params.push_back(param);
                param_types.push_back(param->get_type());

                skip_newlines();
                if (peek().type == TokenType::comma) {
                    blind_consume();
                } else if (peek().type != TokenType::rparen) {
                    SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                    diagnostics.report(loc, diag::parse::err_expected_rparen)
                        << "expected ',' or ')' after function argument";
                    skip_until_any_of(TokenType::comma, TokenType::rparen, TokenType::semicolon);
                    if (peek().type == TokenType::semicolon) break;
                }
            }
            expect(rparen);
        } else {
            SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
            diagnostics.report(loc, diag::parse::err_expected_lparen)
                << "expected '(' for function argument list"
                << diag::FixItHint::create_insertion(loc, "<lparen>");
            skip_until(TokenType::semicolon);
            return context.create<ErrorDecl>(
                function_id_info,
                SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
            );
        }

        MutableType* return_type = nullptr;
        if (silent_consume(arrow).type != TokenType::invalid_token) {
            return_type = match_type(false, false);
            if (!return_type) {
                SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                diagnostics.report(loc, diag::parse::err_expected_type)
                    << "expected return type after '->'";
            }
        } else {
            auto* void_type = context.create<BuiltinType>(BuiltinTypeKind::void_);
            return_type = context.create<MutableType>(void_type, false, false);
        }

        FunctionType* ft = context.create_function_type(return_type, param_types);
        MutableType* qt = context.create<MutableType>(ft, fn_is_mut, fn_is_comp);

        SourceLocation decl_end_location = diagnostics.get_source_manager()->get_location(file_id, peek());
        FuncDecl* function = context.create_func_decl(
            function_id_info,
            qt,
            false,
            collected_params.size(),
            SourceRange(decl_start_location, decl_end_location)
        );

        ParamVarDecl** params_ptr = function->get_params();
        for (std::size_t i = 0; i < collected_params.size(); ++i) {
            params_ptr[i] = collected_params[i];
        }

        if (silent_probe(TokenType::lbrace)) {
            // TODO: Parse function body.
            diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_unexpected_token)
                << "function body parsing is not implemented yet; skipping block";
        }

        expect(semicolon);
        return function;
    }
}
