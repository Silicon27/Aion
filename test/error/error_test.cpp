//
// Error Handling Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "error_test.hpp"
#include <error/error.hpp>
#include <lexer/lexer.hpp>
#include <sstream>

namespace aion::test {

void register_error_tests(TestRunner& runner) {

    // ========================================================================
    // Error Creation Tests
    // ========================================================================

    auto creation_suite = std::make_unique<TestSuite>("Error::Creation");

    creation_suite->add_test("CharSourceRange", []() {
        Source_Location start(1, 10);
        Source_Location end(1, 20);
        
        diag::CharSourceRange range = diag::CharSourceRange::get_char_range(start, end);
        AION_ASSERT_TRUE(range.is_valid());
        AION_ASSERT_TRUE(range.is_char_range());
        AION_ASSERT_FALSE(range.is_token_range());
        AION_ASSERT_EQ(range.begin.offset, 10);
        AION_ASSERT_EQ(range.end.offset, 20);

        diag::CharSourceRange token_range = diag::CharSourceRange::get_token_range(start, end);
        AION_ASSERT_TRUE(token_range.is_token_range());
        AION_ASSERT_FALSE(token_range.is_char_range());
    });

    creation_suite->add_test("FixItHint", []() {
        Source_Location loc(1, 5);
        diag::FixItHint insert = diag::FixItHint::create_insertion(loc, "foo");
        AION_ASSERT_FALSE(insert.is_null());
        AION_ASSERT_STREQ(insert.code_to_insert, "foo");
        AION_ASSERT_EQ(insert.remove_range.begin.offset, 5);

        diag::CharSourceRange range = diag::CharSourceRange::get_char_range(Source_Location(1, 5), Source_Location(1, 10));
        diag::FixItHint remove = diag::FixItHint::create_removal(range);
        AION_ASSERT_STREQ(remove.code_to_insert, "");
        AION_ASSERT_TRUE(remove.remove_range.is_valid());

        diag::FixItHint replace = diag::FixItHint::create_replacement(range, "bar");
        AION_ASSERT_STREQ(replace.code_to_insert, "bar");
        AION_ASSERT_TRUE(replace.remove_range.is_valid());
    });

    runner.add_suite(std::move(creation_suite));

    // ========================================================================
    // Error Formatting Tests
    // ========================================================================

    auto format_suite = std::make_unique<TestSuite>("Error::Formatting");

    format_suite->add_test("DiagnosticBuilder_Formatting", []() {
        std::stringstream ss;
        Source_Manager sm;
        diag::TextDiagnosticPrinter* printer = new diag::TextDiagnosticPrinter(ss, &sm, false);
        diag::DiagnosticsEngine engine(&sm, printer, true);
        
        engine.report(diag::common::err_unknown_identifier) << "my_var";
        AION_ASSERT_CONTAINS(ss.str(), "error: unknown identifier 'my_var'");
        
        ss.str("");
        engine.report(diag::common::warn_unused_variable) << "unused_val";
        AION_ASSERT_CONTAINS(ss.str(), "warning: unused variable 'unused_val'");

        ss.str("");
        engine.report(diag::parse::err_expected_semicolon);
        AION_ASSERT_CONTAINS(ss.str(), "error: expected ';'");
    });

    runner.add_suite(std::move(format_suite));

    // ========================================================================
    // Diagnostic Engine Tests
    // ========================================================================

    auto engine_suite = std::make_unique<TestSuite>("Error::Engine");

    engine_suite->add_test("SeverityMapping", []() {
        diag::DiagnosticsEngine engine;
        
        AION_ASSERT_ENUM_EQ(engine.get_severity(diag::common::err_unknown_identifier), diag::Severity::error);
        AION_ASSERT_ENUM_EQ(engine.get_severity(diag::common::warn_unused_variable), diag::Severity::warning);
        
        engine.set_warnings_as_errors(true);
        // getSeverity doesn't apply warnings_as_errors, EmitDiagnostic does.
        // But we can check if it has the flag.
        AION_ASSERT_TRUE(engine.get_warnings_as_errors());
    });

    engine_suite->add_test("ErrorLimit", []() {
        diag::DiagnosticsEngine engine;
        engine.set_error_limit(1);
        
        // num_errors starts at 0. error_limit is 1.
        
        engine.report(diag::common::err_unknown_identifier).emit();
        // num_errors becomes 1. 1 > 1 is false. Not suppressed.
        AION_ASSERT_EQ(engine.get_num_errors(), 1);

        engine.report(diag::common::err_unknown_identifier).emit();
        // num_errors becomes 2. 2 > 1 is true. 
        // EmitDiagnostic returns early.
        AION_ASSERT_EQ(engine.get_num_errors(), 2);
        
        engine.report(diag::common::err_unknown_identifier).emit();
        // num_errors becomes 3. 3 > 1 is true.
        AION_ASSERT_EQ(engine.get_num_errors(), 3);
    });

    runner.add_suite(std::move(engine_suite));

    // ========================================================================
    // SourceManager Tests
    // ========================================================================

    auto sm_suite = std::make_unique<TestSuite>("Error::SourceManager");

    sm_suite->add_test("TokenToLocation", []() {
        Source_Manager sm;
        FileID fid = sm.add_buffer("line 1\nline 2\nline 3", "test.aion");

        // "line 2" starts at offset 7
        // 'l' is line 2, column 1
        lexer::Token tok;
        tok.line = 2;
        tok.column = 1;
        tok.lexeme = "line";

        Source_Location loc = sm.get_location(fid, tok);
        AION_ASSERT_EQ(loc.file, fid);
        AION_ASSERT_EQ(loc.offset, 7);

        // '2' in "line 2" is line 2, column 6
        tok.column = 6;
        tok.lexeme = "2";
        loc = sm.get_location(fid, tok);
        AION_ASSERT_EQ(loc.offset, 12);
    });

    runner.add_suite(std::move(sm_suite));

    // ========================================================================
    // Text Printer Tests (Extensive)
    // ========================================================================

    auto printer_suite = std::make_unique<TestSuite>("Error::Printing");

    printer_suite->add_test("BasicPrinting", []() {
        OutputCapture capture("BasicPrinting");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false); 
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "unknown identifier 'x'";
        d.severity = diag::Severity::error;
        
        printer.handle_diagnostic(diag::Severity::error, d);
        
        std::string output = capture.get_output();
        AION_ASSERT_CONTAINS(output, "error: unknown identifier 'x'");
        capture.finish();
    });

    printer_suite->add_test("LocationPrinting", []() {
        OutputCapture capture("LocationPrinting");
        Source_Manager sm;
        FileID fid = sm.add_buffer("int x = y;", "test.aion");
        
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "use of undeclared identifier 'y'";
        d.severity = diag::Severity::error;
        d.location = Source_Location(fid, 8); 
        
        printer.handle_diagnostic(diag::Severity::error, d);
        
        std::string output = capture.get_output();
        AION_ASSERT_CONTAINS(output, "test.aion:1:9: error: use of undeclared identifier 'y'");
        AION_ASSERT_CONTAINS(output, " 1 | int x = y;");
        AION_ASSERT_CONTAINS(output, "   |         ^");
        capture.finish();
    });

    printer_suite->add_test("MultiCaretPrinting", []() {
        OutputCapture capture("MultiCaretPrinting");
        Source_Manager sm;
        FileID fid = sm.add_buffer("int x = y + z;", "test.aion");
        
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "unknown identifiers 'y' and 'z'";
        d.severity = diag::Severity::error;
        d.location = Source_Location(fid, 8); // 'y'
        d.extra_locations.push_back(Source_Location(fid, 12)); // 'z'
        
        printer.handle_diagnostic(diag::Severity::error, d);
        
        std::string output = capture.get_output();
        AION_ASSERT_CONTAINS(output, " 1 | int x = y + z;");
        AION_ASSERT_CONTAINS(output, "   |         ^   ^");
        capture.finish();
    });

    printer_suite->add_test("RangePrinting", []() {
        OutputCapture capture("RangePrinting");
        Source_Manager sm;
        FileID fid = sm.add_buffer("int x = y + z;", "test.aion");
        
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "highlighted expression";
        d.severity = diag::Severity::error;
        d.location = Source_Location(fid, 10); // '+'
        // Highlight 'y' and 'z'
        d.ranges.push_back(diag::CharSourceRange::get_char_range(Source_Location(fid, 8), Source_Location(fid, 9)));
        d.ranges.push_back(diag::CharSourceRange::get_char_range(Source_Location(fid, 12), Source_Location(fid, 13)));
        
        printer.handle_diagnostic(diag::Severity::error, d);
        
        std::string output = capture.get_output();
        AION_ASSERT_CONTAINS(output, " 1 | int x = y + z;");
        AION_ASSERT_CONTAINS(output, "   |         ~ ^ ~");
        capture.finish();
    });

    printer_suite->add_test("FixItPrinting", []() {
        OutputCapture capture("FixItPrinting");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        FileID fid = sm.add_buffer("int x = y", "test.aion");
        diag::Diagnostic d;
        d.id = diag::parse::err_expected_semicolon;
        d.message = "expected ';'";
        d.severity = diag::Severity::error;
        d.location = Source_Location(fid, 9);
        d.fixits.push_back(diag::FixItHint::create_insertion(Source_Location(fid, 9), ";"));
        
        printer.handle_diagnostic(diag::Severity::error, d);
        
        std::string output = capture.get_output();
        AION_ASSERT_CONTAINS(output, "help: insert \";\"");
        AION_ASSERT_CONTAINS(output, " 1 | int x = y;");
        AION_ASSERT_CONTAINS(output, "   |          ~");
        capture.finish();
    });
    
    printer_suite->add_test("FixItPrinting_streams", [] {
        OutputCapture capture("FixItPrinting_streams");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        FileID fid = sm.add_buffer("int x = y", "test.aion");
        diag::DiagnosticsEngine d(&sm, &printer, false);
        d.report(Source_Location(fid, 9), diag::parse::err_expected_semicolon) 
        << diag::FixItHint::create_insertion(Source_Location(fid, 9), ";");
        
        std::string output = capture.get_output();
        AION_ASSERT_CONTAINS(output, "help: insert \";\"");
        AION_ASSERT_CONTAINS(output, " 1 | int x = y;");
        AION_ASSERT_CONTAINS(output, "   |          ~");
        capture.finish();
    });

    printer_suite->add_test("ColorPrinting", []() {
        OutputCapture capture("ColorPrinting");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, true); // Enable colors
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "colored error";
        d.severity = diag::Severity::error;
        
        printer.handle_diagnostic(diag::Severity::error, d);
        
        std::string output = capture.get_output();
        // The printer uses \033[1;31merror\033[0m: for Error
        AION_ASSERT_CONTAINS(output, "\033[1;31merror\033[0m: ");
        AION_ASSERT_CONTAINS(output, "\033[1mcolored error\033[0m");
        
        // Check for caret color too
        capture.get_stream().str("");
        FileID fid = sm.add_buffer("foo", "test.aion");
        d.location = Source_Location(fid, 0);
        printer.handle_diagnostic(diag::Severity::error, d);
        // Green color for caret
        AION_ASSERT_CONTAINS(capture.get_output(), "\033[1;32m");
        AION_ASSERT_CONTAINS(capture.get_output(), "^");
        
        capture.finish();
    });

    printer_suite->add_test("MultilineSourcePrinting", []() {
        OutputCapture capture("MultilineSourcePrinting");
        Source_Manager sm;
        FileID fid = sm.add_buffer("line 1\nline 2\nline 3", "test.aion");
        
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "error on line 2";
        d.severity = diag::Severity::error;
        // Offset for "line 2"
        // line 1\n = 7 chars
        d.location = Source_Location(fid, 7); 
        
        printer.handle_diagnostic(diag::Severity::error, d);
        
        std::string output = capture.get_output();
        AION_ASSERT_CONTAINS(output, "test.aion:2:1: error: error on line 2");
        AION_ASSERT_CONTAINS(output, " 2 | line 2");
        AION_ASSERT_CONTAINS(output, "   | ^");
        capture.finish();
    });

    runner.add_suite(std::move(printer_suite));
}

} // namespace aion::test

// ============================================================================
// Main function for standalone error test executable
// Only compiled when building as standalone (ERROR_TEST_STANDALONE defined)
// ============================================================================
#ifdef ERROR_TEST_STANDALONE
int main(int argc, char* argv[]) {
    using namespace aion::test;

    TestRunner runner;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
    }

    register_error_tests(runner);

    return runner.run_all(verbose);
}
#endif
