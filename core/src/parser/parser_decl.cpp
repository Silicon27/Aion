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
            case TokenType::kw_fn: {
                context.get_translation_unit_decl()->add_decl(parse_function_decl());
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
            auto mut_tok = blind_consume();
            if (silent_probe(comp)) {
                auto comp_tok = peek();
                SourceLocation mut_loc = diagnostics.get_token_location(file_id, mut_tok);
                SourceLocation comp_loc = diagnostics.get_token_location(file_id, comp_tok);
                diagnostics.report(mut_loc, diag::parse::err_unexpected_attribute)
                    << "compile time variables cannot be attributed with mut"
                    << diag::FixItHint::create_removal(diagnostics.token_range(mut_tok));
                blind_consume();
            }
        } else if (silent_probe(comp)) {
            is_comp = true;
            auto comp_tok = blind_consume();
            if (silent_probe(mut)) {
                auto mut_tok = peek();
                SourceLocation mut_loc = diagnostics.get_token_location(file_id, mut_tok);
                SourceLocation comp_loc = diagnostics.get_token_location(file_id, comp_tok);
                diagnostics.report(mut_loc, diag::parse::err_unexpected_attribute)
                    << "compile time variables cannot be attributed with mut"
                    << diag::FixItHint::create_removal(diagnostics.token_range(mut_tok));
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
            type_annotation = match_type(is_mut);

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
        auto comma = MatchToken(TokenType::comma);
        auto arrow = MatchToken(TokenType::arrow);
        auto colon = MatchToken(TokenType::colon);
        auto semicolon = MatchToken(TokenType::semicolon);

        SourceLocation decl_start_location = diagnostics.get_source_manager()->get_location(file_id, peek());
        IdentifierInfo* function_id_info = nullptr;
        Token function_id_token;

        ast::ShortVec<ParamVarDecl*> collected_params(context);
        ast::ShortVec<MutableType*> collected_return_types(context);

        auto parse_argument = [&]() -> Decl* {
            bool is_mut = false;
            Token id;
            MutableType* type = nullptr;
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
                // type parsing branch
                blind_consume(); // consume colon
            } else {
                SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                diagnostics.report(loc, diag::parse::err_expected_type)
                    << "expected colon for explicit type declaration for function parameter"
                    << diag::FixItHint::create_insertion(loc, "<colon>");
            }

            if (silent_probe(TokenType::kw_mut)) {
                is_mut = true;
                auto mut_tok = blind_consume();
                if (silent_probe(TokenType::kw_comp)) {
                    auto comp_tok = peek();
                    SourceLocation mut_loc = diagnostics.get_token_location(file_id, mut_tok);
                    SourceLocation comp_loc = diagnostics.get_token_location(file_id, comp_tok);
                    diagnostics.report(mut_loc, diag::parse::err_unexpected_attribute)
                        << "parameters cannot be declared as comp; if you meant to declare a compile time function use fn comp"
                        << diag::FixItHint::create_removal(diagnostics.token_range(mut_tok));
                    blind_consume();
                }
            }

            type = match_type(is_mut);

            if (silent_probe(TokenType::equal)) {
                blind_consume();

                // perform possible erroneous token checks
                if (silent_probe(TokenType::rparen)) {
                    SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                    diagnostics.report(diagnostics.get_token_location(file_id, peek()), diag::parse::err_unexpected_token)
                        << "expected expression after '='"
                        << diag::but_got("expected <expr> but got %1");
                    skip_until_any_of(TokenType::comma, TokenType::rparen, TokenType::semicolon);
                    return context.create<ErrorDecl>(nullptr, SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek())));
                } else if (silent_probe(TokenType::comma)) {
                    SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                    diagnostics.report(diagnostics.get_token_location(file_id, peek()), diag::parse::err_unexpected_token)
                        << "expected expression after ','"
                        << diag::but_got("expected <expr> but got %1");
                    skip_until_any_of(TokenType::comma, TokenType::rparen, TokenType::semicolon);
                    return context.create<ErrorDecl>(nullptr, SourceRange(decl_start_location, loc));
                }

                default_value = parse_expression(0, TokenType::comma, TokenType::rparen);
            }

            return context.create<ParamVarDecl>(
                context.emplace_or_get_identifier(id.lexeme),
                type,
                SourceRange(diagnostics.get_source_manager()->get_location(file_id, id), diagnostics.get_source_manager()->get_location(file_id, peek())),
                default_value
            );
        };


        diffuse_match(TokenType::kw_fn, peek().type, "expected 'fn' keyword");

        if (silent_probe(identifier)) {
            function_id_token = blind_consume();
            if (context.get_identifier(function_id_token.lexeme) != nullptr) {
                diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, function_id_token), diag::parse::err_redefinition);
            }
            function_id_info = context.emplace_or_get_identifier(function_id_token.lexeme);
            // function_id_info being nullptr case handled here if that exists
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

        if (silent_consume(lparen).type != TokenType::invalid_token) {
            // argument parsing branch
            while (true) {
                skip_newlines();
                if (peek().type == TokenType::rparen) {
                    break;
                }
                if (peek().type == TokenType::eof) {
                    diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_unexpected_eof)
                        << "unexpected end of file while parsing function arguments";
                    return context.create<ErrorDecl>(
                        function_id_info,
                        SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
                    );
                }

                Decl* argument = parse_argument();

                if (argument->get_kind() == DeclKind::error) {
                    return argument;
                }

                collected_params.push_back(static_cast<ParamVarDecl*>(argument));

                skip_newlines();
                if (peek().type == TokenType::comma) {
                    blind_consume();
                } else if (peek().type != TokenType::rparen) {
                    SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                    diagnostics.report(loc, diag::parse::err_expected_rparen)
                        << "expected ',' or ')' after function argument";
                    skip_until_any_of(TokenType::comma, TokenType::rparen, TokenType::semicolon);
                    if (peek().type == TokenType::semicolon) {
                        break;
                    }
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

        if (silent_probe(arrow)) {
            blind_consume();
            while (true) {
                skip_newlines();
                MutableType* ret_type = match_type(false);
                if (ret_type) {
                    collected_return_types.push_back(ret_type);
                } else {
                    SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
                    diagnostics.report(loc, diag::parse::err_expected_type)
                        << "expected return type after '->'";
                    break;
                }

                skip_newlines();
                if (peek().type == TokenType::comma) {
                    blind_consume();
                } else {
                    break;
                }
            }
        } else {
            SourceLocation loc = diagnostics.get_source_manager()->get_location(file_id, peek());
            diagnostics.report(loc, diag::parse::err_expected_arrow_operator)
                << "expected '->' for function return type"
                << diag::FixItHint::create_insertion(loc, "<arrow>");
            skip_until(TokenType::semicolon);
            return context.create<ErrorDecl>(
                function_id_info,
                SourceRange(decl_start_location, diagnostics.get_source_manager()->get_location(file_id, peek()))
            );
        }

        

        SourceLocation decl_end_location = diagnostics.get_source_manager()->get_location(file_id, peek());
        SourceRange full_range(decl_start_location, decl_end_location);

        FuncDecl* function = context.create_func_decl(
            function_id_info ? function_id_info->get_name() : "<error>",
            collected_params.size(),
            collected_return_types.size()
        );

        function->source_location = decl_start_location;
        function->decl_end = decl_end_location;

        ParamVarDecl** params_ptr = function->get_params();
        for (std::size_t i = 0; i < collected_params.size(); ++i) {
            params_ptr[i] = collected_params[i];
        }

        MutableType** rets_ptr = function->get_return_types();
        for (std::size_t i = 0; i < collected_return_types.size(); ++i) {
            rets_ptr[i] = collected_return_types[i];
        }

        if (silent_probe(TokenType::lbrace)) {
            // TODO: Parse function body. For now, we just skip it if we find it.
            // But we should probably report that it's not implemented if we want to be honest.
            diagnostics.report(diagnostics.get_source_manager()->get_location(file_id, peek()), diag::parse::err_unexpected_token)
                << "function body parsing is not implemented yet; skipping block";
            // Simple brace skipping logic could go here, but let's just stick to semicolon for now
            // as the grammar suggested block.
        }

        expect(semicolon);
        return function;
    }
}
