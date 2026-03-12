//
// Error Handling Test Suite - Implementation
// Created by David Yang on 2026-02-03.
//

#include "error_test.hpp"
#include <error/error.hpp>
#include <sstream>

namespace udo::test {

// Global flag to enable neat output
bool g_show_neat_output = false;

// Helper to capture output and optionally print it neatly
class OutputCapture {
    std::stringstream buffer;
    std::string test_name;

public:
    explicit OutputCapture(std::string name) : test_name(std::move(name)) {}

    std::string get_output() const { return buffer.str(); }
    std::stringstream& get_stream() { return buffer; }
    
    void finish() {
        if (g_show_neat_output) {
            std::cout << "\n[NEAT OUTPUT for " << test_name << "]:\n";
            std::cout << "------------------------------------------\n";
            std::cout << buffer.str();
            std::cout << "------------------------------------------\n";
        }
    }
};

void register_error_tests(TestRunner& runner) {

    // ========================================================================
    // Error Creation Tests
    // ========================================================================

    auto creation_suite = std::make_unique<TestSuite>("Error::Creation");

    creation_suite->add_test("CharSourceRange", []() {
        Source_Location start(1, 10);
        Source_Location end(1, 20);
        
        diag::CharSourceRange range = diag::CharSourceRange::getCharRange(start, end);
        UDO_ASSERT_TRUE(range.isValid());
        UDO_ASSERT_TRUE(range.isCharRange());
        UDO_ASSERT_FALSE(range.isTokenRange());
        UDO_ASSERT_EQ(range.begin.offset, 10);
        UDO_ASSERT_EQ(range.end.offset, 20);

        diag::CharSourceRange token_range = diag::CharSourceRange::getTokenRange(start, end);
        UDO_ASSERT_TRUE(token_range.isTokenRange());
        UDO_ASSERT_FALSE(token_range.isCharRange());
    });

    creation_suite->add_test("FixItHint", []() {
        Source_Location loc(1, 5);
        diag::FixItHint insert = diag::FixItHint::CreateInsertion(loc, "foo");
        UDO_ASSERT_FALSE(insert.isNull());
        UDO_ASSERT_STREQ(insert.code_to_insert, "foo");
        UDO_ASSERT_EQ(insert.remove_range.begin.offset, 5);

        diag::CharSourceRange range = diag::CharSourceRange::getCharRange(Source_Location(1, 5), Source_Location(1, 10));
        diag::FixItHint remove = diag::FixItHint::CreateRemoval(range);
        UDO_ASSERT_STREQ(remove.code_to_insert, "");
        UDO_ASSERT_TRUE(remove.remove_range.isValid());

        diag::FixItHint replace = diag::FixItHint::CreateReplacement(range, "bar");
        UDO_ASSERT_STREQ(replace.code_to_insert, "bar");
        UDO_ASSERT_TRUE(replace.remove_range.isValid());
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
        
        engine.Report(diag::common::err_unknown_identifier) << "my_var";
        UDO_ASSERT_CONTAINS(ss.str(), "error: unknown identifier 'my_var'");
        
        ss.str("");
        engine.Report(diag::common::warn_unused_variable) << "unused_val";
        UDO_ASSERT_CONTAINS(ss.str(), "warning: unused variable 'unused_val'");

        ss.str("");
        engine.Report(diag::parse::err_expected_semicolon);
        UDO_ASSERT_CONTAINS(ss.str(), "error: expected ';'");
    });

    runner.add_suite(std::move(format_suite));

    // ========================================================================
    // Diagnostic Engine Tests
    // ========================================================================

    auto engine_suite = std::make_unique<TestSuite>("Error::Engine");

    engine_suite->add_test("SeverityMapping", []() {
        diag::DiagnosticsEngine engine;
        
        UDO_ASSERT_ENUM_EQ(engine.getSeverity(diag::common::err_unknown_identifier), diag::Severity::Error);
        UDO_ASSERT_ENUM_EQ(engine.getSeverity(diag::common::warn_unused_variable), diag::Severity::Warning);
        
        engine.setWarningsAsErrors(true);
        // getSeverity doesn't apply warnings_as_errors, EmitDiagnostic does.
        // But we can check if it has the flag.
        UDO_ASSERT_TRUE(engine.getWarningsAsErrors());
    });

    engine_suite->add_test("ErrorLimit", []() {
        diag::DiagnosticsEngine engine;
        engine.setErrorLimit(1);
        
        // num_errors starts at 0. error_limit is 1.
        
        engine.Report(diag::common::err_unknown_identifier).emit();
        // num_errors becomes 1. 1 > 1 is false. Not suppressed.
        UDO_ASSERT_EQ(engine.getNumErrors(), 1);

        engine.Report(diag::common::err_unknown_identifier).emit();
        // num_errors becomes 2. 2 > 1 is true. 
        // EmitDiagnostic returns early.
        UDO_ASSERT_EQ(engine.getNumErrors(), 2); 
        
        engine.Report(diag::common::err_unknown_identifier).emit();
        // num_errors becomes 3. 3 > 1 is true.
        UDO_ASSERT_EQ(engine.getNumErrors(), 3); 
    });

    runner.add_suite(std::move(engine_suite));

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
        d.severity = diag::Severity::Error;
        
        printer.HandleDiagnostic(diag::Severity::Error, d);
        
        std::string output = capture.get_output();
        UDO_ASSERT_CONTAINS(output, "error: unknown identifier 'x'");
        capture.finish();
    });

    printer_suite->add_test("LocationPrinting", []() {
        OutputCapture capture("LocationPrinting");
        Source_Manager sm;
        FileID fid = sm.add_buffer("int x = y;", "test.udo");
        
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "use of undeclared identifier 'y'";
        d.severity = diag::Severity::Error;
        d.location = Source_Location(fid, 8); 
        
        printer.HandleDiagnostic(diag::Severity::Error, d);
        
        std::string output = capture.get_output();
        UDO_ASSERT_CONTAINS(output, "test.udo:1:9: error: use of undeclared identifier 'y'");
        UDO_ASSERT_CONTAINS(output, " 1 | int x = y;");
        UDO_ASSERT_CONTAINS(output, "   |         ^");
        capture.finish();
    });

    printer_suite->add_test("MultiCaretPrinting", []() {
        OutputCapture capture("MultiCaretPrinting");
        Source_Manager sm;
        FileID fid = sm.add_buffer("int x = y + z;", "test.udo");
        
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "unknown identifiers 'y' and 'z'";
        d.severity = diag::Severity::Error;
        d.location = Source_Location(fid, 8); // 'y'
        d.extra_locations.push_back(Source_Location(fid, 12)); // 'z'
        
        printer.HandleDiagnostic(diag::Severity::Error, d);
        
        std::string output = capture.get_output();
        UDO_ASSERT_CONTAINS(output, " 1 | int x = y + z;");
        UDO_ASSERT_CONTAINS(output, "   |         ^   ^");
        capture.finish();
    });

    printer_suite->add_test("RangePrinting", []() {
        OutputCapture capture("RangePrinting");
        Source_Manager sm;
        FileID fid = sm.add_buffer("int x = y + z;", "test.udo");
        
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "highlighted expression";
        d.severity = diag::Severity::Error;
        d.location = Source_Location(fid, 10); // '+'
        // Highlight 'y' and 'z'
        d.ranges.push_back(diag::CharSourceRange::getCharRange(Source_Location(fid, 8), Source_Location(fid, 9)));
        d.ranges.push_back(diag::CharSourceRange::getCharRange(Source_Location(fid, 12), Source_Location(fid, 13)));
        
        printer.HandleDiagnostic(diag::Severity::Error, d);
        
        std::string output = capture.get_output();
        UDO_ASSERT_CONTAINS(output, " 1 | int x = y + z;");
        UDO_ASSERT_CONTAINS(output, "   |         ~ ^ ~");
        capture.finish();
    });

    printer_suite->add_test("FixItPrinting", []() {
        OutputCapture capture("FixItPrinting");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        FileID fid = sm.add_buffer("int x = y", "test.udo");
        diag::Diagnostic d;
        d.id = diag::parse::err_expected_semicolon;
        d.message = "expected ';'";
        d.severity = diag::Severity::Error;
        d.location = Source_Location(fid, 9);
        d.fixits.push_back(diag::FixItHint::CreateInsertion(Source_Location(fid, 9), ";"));
        
        printer.HandleDiagnostic(diag::Severity::Error, d);
        
        std::string output = capture.get_output();
        UDO_ASSERT_CONTAINS(output, "help: insert \";\"");
        UDO_ASSERT_CONTAINS(output, " 1 | int x = y;");
        UDO_ASSERT_CONTAINS(output, "   |          ~");
        capture.finish();
    });

    printer_suite->add_test("ColorPrinting", []() {
        OutputCapture capture("ColorPrinting");
        Source_Manager sm;
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, true); // Enable colors
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "colored error";
        d.severity = diag::Severity::Error;
        
        printer.HandleDiagnostic(diag::Severity::Error, d);
        
        std::string output = capture.get_output();
        // The printer uses \033[1;31merror\033[0m: for Error
        UDO_ASSERT_CONTAINS(output, "\033[1;31merror\033[0m: ");
        UDO_ASSERT_CONTAINS(output, "\033[1mcolored error\033[0m");
        
        // Check for caret color too
        capture.get_stream().str("");
        FileID fid = sm.add_buffer("foo", "test.udo");
        d.location = Source_Location(fid, 0);
        printer.HandleDiagnostic(diag::Severity::Error, d);
        // Green color for caret
        UDO_ASSERT_CONTAINS(capture.get_output(), "\033[1;32m");
        UDO_ASSERT_CONTAINS(capture.get_output(), "^");
        
        capture.finish();
    });

    printer_suite->add_test("MultilineSourcePrinting", []() {
        OutputCapture capture("MultilineSourcePrinting");
        Source_Manager sm;
        FileID fid = sm.add_buffer("line 1\nline 2\nline 3", "test.udo");
        
        diag::TextDiagnosticPrinter printer(capture.get_stream(), &sm, false);
        
        diag::Diagnostic d;
        d.id = diag::common::err_unknown_identifier;
        d.message = "error on line 2";
        d.severity = diag::Severity::Error;
        // Offset for "line 2"
        // line 1\n = 7 chars
        d.location = Source_Location(fid, 7); 
        
        printer.HandleDiagnostic(diag::Severity::Error, d);
        
        std::string output = capture.get_output();
        UDO_ASSERT_CONTAINS(output, "test.udo:2:1: error: error on line 2");
        UDO_ASSERT_CONTAINS(output, " 2 | line 2");
        UDO_ASSERT_CONTAINS(output, "   | ^");
        capture.finish();
    });

    runner.add_suite(std::move(printer_suite));
}

} // namespace udo::test

// ============================================================================
// Main function for standalone error test executable
// Only compiled when building as standalone (ERROR_TEST_STANDALONE defined)
// ============================================================================
#ifdef ERROR_TEST_STANDALONE
int main(int argc, char* argv[]) {
    using namespace udo::test;

    TestRunner runner;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "--neat") {
            g_show_neat_output = true;
        }
    }
    g_show_neat_output = true;

    register_error_tests(runner);

    return runner.run_all(verbose);
}
#endif
