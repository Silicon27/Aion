# Aion Diagnostics Engine

## Complete Reference Guide

**Version:** 1.0  
**Author:** David Yang  
**Last Updated:** February 2026  
**Based on:** LLVM/Clang DiagnosticsEngine

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [File Structure](#file-structure)
4. [Severity Levels](#severity-levels)
5. [Diagnostic IDs](#diagnostic-ids)
6. [Core Components](#core-components)
   - [DiagnosticsEngine](#diagnosticsengine)
   - [DiagnosticBuilder](#diagnosticbuilder)
   - [DiagnosticConsumer](#diagnosticconsumer)
   - [TextDiagnosticPrinter](#textdiagnosticprinter)
7. [Source Locations and Ranges](#source-locations-and-ranges)
8. [Fix-It Hints](#fix-it-hints)
9. [Usage Examples](#usage-examples)
10. [Configuration Options](#configuration-options)
11. [Extending the System](#extending-the-system)
12. [Best Practices](#best-practices)
13. [API Reference](#api-reference)

---

## Overview

The Aion Diagnostics Engine is a comprehensive error and warning reporting system modeled after LLVM/Clang's diagnostic infrastructure. It provides:

- **Structured error reporting** with unique diagnostic IDs
- **Severity levels** from notes to fatal errors
- **Source location tracking** with file, line, and column information
- **Source code snippets** with caret pointing to the exact error location
- **Fix-it hints** suggesting code modifications to resolve issues
- **Colored terminal output** for better readability
- **Configurable behavior** including `-Werror` style warning promotion
- **Extensible consumer architecture** for custom output handling

### Design Philosophy

The diagnostics engine follows several key principles:

1. **Separation of Concerns**: Diagnostic IDs, severity levels, and the engine itself are in separate files to avoid circular dependencies.

2. **Fluent Interface**: The `DiagnosticBuilder` uses operator overloading (`<<`) for a natural, stream-like syntax.

3. **RAII Semantics**: Diagnostics are automatically emitted when the `DiagnosticBuilder` goes out of scope.

4. **Extensibility**: Custom `DiagnosticConsumer` implementations can redirect output anywhere.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         DiagnosticsEngine                                │
│  ┌─────────────────┐  ┌──────────────────┐  ┌───────────────────────┐  │
│  │ Configuration   │  │ Severity Mapping │  │   Error Counting      │  │
│  │ - Werror        │  │ - Per-diagnostic │  │   - num_errors_       │  │
│  │ - error_limit   │  │ - User/pragma    │  │   - num_warnings_     │  │
│  │ - show_colors   │  │ - Promotions     │  │   - hasErrorOccurred  │  │
│  └─────────────────┘  └──────────────────┘  └───────────────────────┘  │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      Report() → DiagnosticBuilder                │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────┐   │   │
│  │  │ String args  │  │ Source ranges│  │    FixIt hints      │   │   │
│  │  │ Integer args │  │ Highlighting │  │ Insert/Remove/Replace│   │   │
│  │  └──────────────┘  └──────────────┘  └─────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                    │                                     │
│                                    ▼                                     │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                     DiagnosticConsumer                           │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │              TextDiagnosticPrinter                       │    │   │
│  │  │  - Colored output to std::ostream                        │    │   │
│  │  │  - Source line display with caret                        │    │   │
│  │  │  - Fix-it hint display                                   │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## File Structure

The diagnostics system is split across multiple files to avoid circular dependencies:

```
core/src/error/
├── DiagnosticSeverity.hpp   # Severity enum (standalone, no dependencies)
├── diagid.hpp               # Diagnostic IDs, categories, and mappings
├── error.hpp                # Main classes: Engine, Builder, Consumer
└── error.c++                # Implementation
```

### Why This Structure?

- **`DiagnosticSeverity.hpp`**: Contains only the `Severity` enum with no dependencies. This allows other headers (like `source_manager.hpp`) to forward-declare diagnostics types without including the full engine.

- **`diagid.hpp`**: Contains all diagnostic ID definitions, organized by compiler phase. Includes `DiagnosticSeverity.hpp` for the `Severity` type used in `DiagnosticInfo`.

- **`error.hpp`**: The main header that brings everything together. Includes the complete `DiagnosticsEngine`, `DiagnosticBuilder`, and consumer classes.

---

## Severity Levels

Severity levels determine how a diagnostic is presented and whether it affects compilation success.

### Enum Definition

```c++
enum class Severity : uint8_t {
    Ignored = 1,    // Do not present this diagnostic, ignore it
    Note    = 2,    // Informational note, no rule violation
    Remark  = 3,    // Optimization reports, verbose info
    Warning = 4,    // Rule violation, still compilable
    Error   = 5,    // Unrecoverable, compilation fails
    Fatal   = 6     // Immediately terminates compilation
};
```

### Severity Descriptions

| Severity | Description | Stops Compilation? | Example |
|----------|-------------|-------------------|---------|
| `Ignored` | Diagnostic is suppressed | No | Disabled warnings |
| `Note` | Additional context for another diagnostic | No | "previous definition was here" |
| `Remark` | Informational (optimization reports) | No | "loop vectorized" |
| `Warning` | Potential problem, code still valid | No | "unused variable 'x'" |
| `Error` | Invalid code, cannot compile | Yes (after phase) | "undeclared identifier 'foo'" |
| `Fatal` | Critical failure, stop immediately | Yes (immediately) | "too many errors emitted" |

### Severity Promotion

Severities can be promoted at runtime:

```c++
// Treat all warnings as errors (like -Werror)
diag.setWarningsAsErrors(true);

// Treat all errors as fatal (stop on first error)
diag.setErrorsAsFatal(true);
```

### Helper Functions

```c++
// Check if severity is error or worse
bool isErrorOrFatal(Severity S);

// Get human-readable name
const char* getSeverityName(Severity S);
// Returns: "ignored", "note", "remark", "warning", "error", "fatal error"
```

---

## Diagnostic IDs

Every diagnostic has a unique numeric ID that identifies it across the compiler. IDs are organized into categories based on the compiler phase that generates them.

### Category Structure

```c++
// Category sizes (maximum diagnostics per category)
enum DiagnosticSizes : unsigned {
    DIAG_SIZE_COMMON    = 300,   // General diagnostics
    DIAG_SIZE_LEXER     = 200,   // Lexer/tokenization
    DIAG_SIZE_PARSER    = 500,   // Parsing
    DIAG_SIZE_SEMA      = 1000,  // Semantic analysis
    DIAG_SIZE_CODEGEN   = 200,   // Code generation
};

// Category start positions
enum DiagnosticStart : unsigned {
    DIAG_START_COMMON   = 0,
    DIAG_START_LEXER    = 300,   // DIAG_START_COMMON + DIAG_SIZE_COMMON
    DIAG_START_PARSER   = 500,   // DIAG_START_LEXER + DIAG_SIZE_LEXER
    DIAG_START_SEMA     = 1000,  // DIAG_START_PARSER + DIAG_SIZE_PARSER
    DIAG_START_CODEGEN  = 2000,  // DIAG_START_SEMA + DIAG_SIZE_SEMA
    DIAG_UPPER_LIMIT    = 2200,  // End of built-in diagnostics
};
```

### Naming Conventions

Diagnostic IDs follow a strict naming convention:

| Prefix | Severity | Example |
|--------|----------|---------|
| `err_` | Error | `err_undeclared_identifier` |
| `warn_` | Warning | `warn_unused_variable` |
| `note_` | Note | `note_previous_definition` |
| `remark_` | Remark | `remark_loop_vectorized` |

### Built-in Diagnostic IDs

#### Common Diagnostics (`diag::common::`)

```c++
namespace common {
enum : DiagID {
    err_expected_token = DIAG_START_COMMON,  // ID: 0
    err_unknown_identifier,                   // ID: 1
    err_file_not_found,                       // ID: 2
    err_invalid_character,                    // ID: 3
    warn_unused_variable,                     // ID: 4
    warn_unused_parameter,                    // ID: 5
    note_previous_definition,                 // ID: 6
    note_declared_here,                       // ID: 7
};
}
```

#### Lexer Diagnostics (`diag::lex::`)

```c++
namespace lex {
enum : DiagID {
    err_unterminated_string = DIAG_START_LEXER,  // ID: 300
    err_unterminated_char,                        // ID: 301
    err_unterminated_block_comment,               // ID: 302
    err_invalid_numeric_literal,                  // ID: 303
    err_invalid_escape_sequence,                  // ID: 304
    err_empty_character_literal,                  // ID: 305
    warn_multi_char_constant,                     // ID: 306
};
}
```

#### Parser Diagnostics (`diag::parse::`)

```c++
namespace parse {
enum : DiagID {
    err_expected_expression = DIAG_START_PARSER,  // ID: 500
    err_expected_statement,                        // ID: 501
    err_expected_type,                             // ID: 502
    err_expected_identifier,                       // ID: 503
    err_expected_semicolon,                        // ID: 504
    err_expected_lparen,                           // ID: 505
    err_expected_rparen,                           // ID: 506
    err_expected_lbrace,                           // ID: 507
    err_expected_rbrace,                           // ID: 508
    err_expected_lbracket,                         // ID: 509
    err_expected_rbracket,                         // ID: 510
    err_unexpected_token,                          // ID: 511
    err_mismatched_brackets,                       // ID: 512
    warn_empty_statement,                          // ID: 513
};
}
```

#### Semantic Analysis Diagnostics (`diag::sema::`)

```c++
namespace sema {
enum : DiagID {
    err_undeclared_identifier = DIAG_START_SEMA,  // ID: 1000
    err_redefinition,                              // ID: 1001
    err_type_mismatch,                             // ID: 1002
    err_invalid_operands,                          // ID: 1003
    err_cannot_convert,                            // ID: 1004
    err_ambiguous_call,                            // ID: 1005
    err_no_matching_function,                      // ID: 1006
    err_too_many_arguments,                        // ID: 1007
    err_too_few_arguments,                         // ID: 1008
    warn_implicit_conversion,                      // ID: 1009
    warn_shadowed_variable,                        // ID: 1010
    note_previous_declaration,                     // ID: 1011
};
}
```

### Adding New Diagnostic IDs

To add a new diagnostic:

1. Choose the appropriate category namespace
2. Add the enum value (it auto-increments)
3. Follow the naming convention

```c++
namespace sema {
enum : DiagID {
    // ... existing diagnostics ...
    err_too_few_arguments,
    
    // Add new diagnostics here:
    err_invalid_array_size,        // New error
    warn_deprecated_function,      // New warning
    note_function_declared_here,   // New note
};
}
```

---

## Core Components

### DiagnosticsEngine

The `DiagnosticsEngine` is the central hub for all diagnostic emission. It manages configuration, severity mappings, error counting, and forwards diagnostics to consumers.

#### Construction

```c++
// Default construction (no source manager, no consumer)
DiagnosticsEngine diag;

// With source manager and consumer
Source_Manager sm;
TextDiagnosticPrinter printer(std::cerr, &sm);
DiagnosticsEngine diag(&sm, &printer, false);  // false = doesn't own consumer

// Using the convenience factory function
auto diag = createDiagnosticsEngine(&sm);  // Creates with TextDiagnosticPrinter
```

#### Configuration Methods

```c++
// Source Manager
void setSourceManager(Source_Manager* sm);
Source_Manager* getSourceManager() const;

// Diagnostic Consumer
void setClient(DiagnosticConsumer* client, bool owns = false);
DiagnosticConsumer* getClient() const;

// Warning/Error Promotion
void setWarningsAsErrors(bool val);   // -Werror
bool getWarningsAsErrors() const;

void setErrorsAsFatal(bool val);      // Stop on first error
bool getErrorsAsFatal() const;

// Suppression
void setSuppressAllDiagnostics(bool val);  // Silence everything
bool getSuppressAllDiagnostics() const;

// Display Options
void setShowColors(bool val);
bool getShowColors() const;

// Error Limiting
void setErrorLimit(unsigned limit);   // 0 = no limit
unsigned getErrorLimit() const;
```

#### Severity Mapping

You can override the default severity for any diagnostic:

```c++
// Make a specific warning an error
diag.setSeverity(diag::common::warn_unused_variable, Severity::Error);

// Make a specific error a warning (useful for gradual migration)
diag.setSeverity(diag::sema::err_type_mismatch, Severity::Warning);

// Suppress a specific diagnostic entirely
diag.setSeverity(diag::parse::warn_empty_statement, Severity::Ignored);

// Query current severity
Severity sev = diag.getSeverity(diag::common::warn_unused_variable);
```

#### Error Counting

```c++
unsigned getNumErrors() const;      // Total errors emitted
unsigned getNumWarnings() const;    // Total warnings emitted

bool hasErrorOccurred() const;      // Any errors?
bool hasFatalErrorOccurred() const; // Any fatal errors?

void reset();  // Clear all counts and state
```

#### Diagnostic Emission

```c++
// Report at a specific location
DiagnosticBuilder Report(Source_Location loc, DiagID id);

// Report without location (uses current position)
DiagnosticBuilder Report(DiagID id);
```

---

### DiagnosticBuilder

The `DiagnosticBuilder` provides a fluent interface for constructing diagnostics. It uses RAII: the diagnostic is automatically emitted when the builder goes out of scope.

#### Basic Usage

```c++
// Simple diagnostic
diag.Report(loc, diag::sema::err_undeclared_identifier)
    << "variableName";

// With multiple arguments
diag.Report(loc, diag::parse::err_expected_token)
    << ";" << "got 'return'";

// With integer argument
diag.Report(loc, diag::sema::err_too_many_arguments)
    << 3 << 5;  // "expected 3 arguments, got 5"
```

#### Operator Overloads

The `<<` operator is overloaded for various types:

```c++
// String arguments
DiagnosticBuilder& operator<<(const std::string& str);
DiagnosticBuilder& operator<<(const char* str);

// Integer arguments
DiagnosticBuilder& operator<<(int val);
DiagnosticBuilder& operator<<(unsigned val);
DiagnosticBuilder& operator<<(int64_t val);

// Source range highlighting
DiagnosticBuilder& operator<<(CharSourceRange range);

// Fix-it hints
DiagnosticBuilder& operator<<(FixItHint hint);
```

#### Chaining Example

```c++
diag.Report(location, diag::sema::err_type_mismatch)
    << "int"                          // Expected type
    << "float"                        // Got type
    << CharSourceRange::getTokenRange(expr_begin, expr_end)  // Highlight
    << FixItHint::CreateReplacement(range, "static_cast<int>(x)");
```

#### Manual Control

```c++
auto builder = diag.Report(loc, diag::parse::err_expected_semicolon);
builder << ";";

// Abandon without emitting
builder.clear();

// Or explicitly emit
builder.emit();
```

---

### DiagnosticConsumer

Abstract base class for handling emitted diagnostics. Implement this to customize output behavior.

#### Interface

```c++
class DiagnosticConsumer {
public:
    virtual ~DiagnosticConsumer() = default;

    // Lifecycle hooks
    virtual void BeginSourceFile() {}
    virtual void EndSourceFile() {}

    // Main handler - must be implemented
    virtual void HandleDiagnostic(Severity severity, const Diagnostic& diag) = 0;

    // Statistics
    unsigned getNumErrors() const;
    unsigned getNumWarnings() const;
    virtual void clear();

protected:
    unsigned num_errors_ = 0;
    unsigned num_warnings_ = 0;
};
```

#### Custom Consumer Example

```c++
class JSONDiagnosticConsumer : public DiagnosticConsumer {
    std::vector<nlohmann::json> diagnostics_;

public:
    void HandleDiagnostic(Severity severity, const Diagnostic& diag) override {
        nlohmann::json j;
        j["severity"] = getSeverityName(severity);
        j["message"] = diag.message;
        j["id"] = diag.id;
        
        if (diag.location.isValid()) {
            j["file"] = /* get file path */;
            j["line"] = /* get line */;
            j["column"] = /* get column */;
        }
        
        diagnostics_.push_back(j);
        
        if (isErrorOrFatal(severity)) ++num_errors_;
        else if (severity == Severity::Warning) ++num_warnings_;
    }
    
    std::string toJSON() const {
        return nlohmann::json(diagnostics_).dump(2);
    }
};
```

---

### TextDiagnosticPrinter

The default `DiagnosticConsumer` that prints formatted diagnostics to a stream.

#### Construction

```c++
TextDiagnosticPrinter(
    std::ostream& os,           // Output stream (e.g., std::cerr)
    Source_Manager* sm = nullptr,  // For source snippets
    bool colors = true          // Enable ANSI colors
);
```

#### Output Format

```
path/to/file.aion:42:15: error: undeclared identifier 'foo'
    let x = foo + bar;
              ^
  fix-it: insert "let foo = 0; "
```

#### Color Coding

| Severity | Color |
|----------|-------|
| Note | Bold Cyan |
| Remark | Bold Magenta |
| Warning | Bold Yellow |
| Error | Bold Red |
| Fatal | Bold Red |
| Fix-it | Bold Green |

---

## Source Locations and Ranges

### Source_Location

Represents a single point in source code:

```c++
struct Source_Location {
    FileID file = 0;      // Which file
    Offset offset = 0;    // Byte offset from start of file
    
    Source_Location() = default;
    Source_Location(FileID f, Offset o);
    
    bool isValid() const;
    bool isInvalid() const;
};
```

### CharSourceRange

Represents a range of source code for highlighting:

```c++
struct CharSourceRange {
    Source_Location begin;
    Source_Location end;
    bool is_token_range = true;
    
    // Factory methods
    static CharSourceRange getTokenRange(Source_Location b, Source_Location e);
    static CharSourceRange getCharRange(Source_Location b, Source_Location e);
    
    bool isValid() const;
    bool isTokenRange() const;
    bool isCharRange() const;
};
```

#### Token Range vs Char Range

- **Token Range**: The end location points to the *start* of the last token
- **Char Range**: The end location points to the character *after* the last character

```c++
// For highlighting "foo" in "let foo = 5"
//                   ^~~

// Token range: end points to 'f'
auto tr = CharSourceRange::getTokenRange(foo_start, foo_start);

// Char range: end points to space after 'o'  
auto cr = CharSourceRange::getCharRange(foo_start, foo_end);
```

---

## Fix-It Hints

Fix-it hints suggest code modifications to fix diagnosed problems.

### FixItHint Class

```c++
class FixItHint {
public:
    CharSourceRange remove_range;      // Code to remove
    CharSourceRange insert_from_range; // Code to copy from elsewhere
    std::string code_to_insert;        // Literal code to insert
    bool before_previous_insertions = false;
    
    bool isNull() const;
    
    // Factory methods
    static FixItHint CreateInsertion(Source_Location loc, const std::string& code,
                                     bool before_previous = false);
    static FixItHint CreateRemoval(CharSourceRange range);
    static FixItHint CreateReplacement(CharSourceRange range, const std::string& code);
};
```

### Insertion

Insert code at a specific location:

```c++
// Insert missing semicolon
FixItHint::CreateInsertion(end_of_statement, ";");

// Insert before other insertions at same location
FixItHint::CreateInsertion(loc, "const ", true);
```

### Removal

Remove a range of code:

```c++
// Remove unnecessary parentheses
auto range = CharSourceRange::getTokenRange(lparen, rparen);
FixItHint::CreateRemoval(range);
```

### Replacement

Replace code with something else:

```c++
// Replace deprecated function
auto range = CharSourceRange::getTokenRange(func_start, func_end);
FixItHint::CreateReplacement(range, "new_function_name");

// Add cast
FixItHint::CreateReplacement(expr_range, "static_cast<int>(x)");
```

---

## Usage Examples

### Example 1: Basic Error Reporting

```c++
#include <error/error.hpp>

void Parser::parseStatement() {
    Token tok = peek();
    
    if (tok.type != TokenType::Semicolon) {
        diag_.Report(tok.location, diag::parse::err_expected_semicolon)
            << ";";
        return;
    }
}
```

Output:
```
file.aion:10:5: error: expected ';'
    return x
    ^
```

### Example 2: Error with Note

```c++
void Sema::checkRedefinition(const std::string& name, Source_Location new_loc) {
    if (auto* existing = lookup(name)) {
        // Report the error
        diag_.Report(new_loc, diag::sema::err_redefinition)
            << name;
        
        // Add a note pointing to the previous definition
        diag_.Report(existing->location, diag::sema::note_previous_declaration)
            << name;
    }
}
```

Output:
```
file.aion:25:5: error: redefinition of 'x'
    let x = 10;
        ^
file.aion:10:5: note: previous declaration of 'x' was here
    let x = 5;
        ^
```

### Example 3: Warning with Fix-It

```c++
void Sema::checkUnusedVariable(Variable* var) {
    if (!var->isUsed()) {
        auto range = CharSourceRange::getTokenRange(var->getDeclStart(), 
                                                     var->getDeclEnd());
        
        diag_.Report(var->getLocation(), diag::common::warn_unused_variable)
            << var->getName()
            << FixItHint::CreateRemoval(range);
    }
}
```

Output:
```
file.aion:5:5: warning: unused variable 'temp'
    let temp = 42;
        ^~~~
  fix-it: remove "let temp = 42;"
```

### Example 4: Type Mismatch with Suggestion

```c++
void Sema::checkAssignment(Expr* lhs, Expr* rhs) {
    Type* lhsType = lhs->getType();
    Type* rhsType = rhs->getType();
    
    if (!lhsType->isAssignableFrom(rhsType)) {
        auto range = CharSourceRange::getTokenRange(rhs->getBeginLoc(), 
                                                     rhs->getEndLoc());
        
        std::string cast = "static_cast<" + lhsType->getName() + ">(" + 
                           rhs->getSourceText() + ")";
        
        diag_.Report(rhs->getBeginLoc(), diag::sema::err_cannot_convert)
            << rhsType->getName()
            << lhsType->getName()
            << range
            << FixItHint::CreateReplacement(range, cast);
    }
}
```

Output:
```
file.aion:15:13: error: cannot convert 'float' to 'int'
    let x: int = 3.14;
                 ^~~~
  fix-it: replace with "static_cast<int>(3.14)"
```

### Example 5: Lexer Error

```c++
void Lexer::lexStringLiteral() {
    Source_Location start = currentLocation();
    advance(); // Skip opening quote
    
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') {
            diag_.Report(start, diag::lex::err_unterminated_string)
                << FixItHint::CreateInsertion(currentLocation(), "\"");
            return;
        }
        advance();
    }
    
    if (isAtEnd()) {
        diag_.Report(start, diag::lex::err_unterminated_string);
    }
}
```

### Example 6: Multiple Ranges

```c++
void Sema::checkBinaryOp(BinaryExpr* expr) {
    if (!areTypesCompatible(expr->getLHS()->getType(), 
                            expr->getRHS()->getType())) {
        
        auto lhsRange = CharSourceRange::getTokenRange(
            expr->getLHS()->getBeginLoc(), 
            expr->getLHS()->getEndLoc()
        );
        auto rhsRange = CharSourceRange::getTokenRange(
            expr->getRHS()->getBeginLoc(), 
            expr->getRHS()->getEndLoc()
        );
        
        diag_.Report(expr->getOpLoc(), diag::sema::err_invalid_operands)
            << expr->getLHS()->getType()->getName()
            << expr->getRHS()->getType()->getName()
            << lhsRange
            << rhsRange;
    }
}
```

Output:
```
file.aion:20:10: error: invalid operands to binary expression ('string' and 'int')
    let x = "hello" + 5;
            ^~~~~~~   ^
```

---

## Configuration Options

### Command-Line Style Options

```c++
// Simulate common compiler flags
void configureFromFlags(DiagnosticsEngine& diag, const CompilerFlags& flags) {
    // -Werror: Treat warnings as errors
    if (flags.werror) {
        diag.setWarningsAsErrors(true);
    }
    
    // -ferror-limit=N: Stop after N errors
    if (flags.error_limit > 0) {
        diag.setErrorLimit(flags.error_limit);
    }
    
    // -fno-color-diagnostics: Disable colors
    if (flags.no_color) {
        diag.setShowColors(false);
    }
    
    // -w: Suppress all warnings
    if (flags.suppress_warnings) {
        // Set all warning diagnostics to Ignored
        for (DiagID id = DIAG_START_COMMON; id < DIAG_UPPER_LIMIT; ++id) {
            if (diag.getSeverity(id) == Severity::Warning) {
                diag.setSeverity(id, Severity::Ignored);
            }
        }
    }
    
    // -Wno-unused-variable: Disable specific warning
    if (flags.no_unused_warning) {
        diag.setSeverity(diag::common::warn_unused_variable, Severity::Ignored);
    }
}
```

### Per-Diagnostic Configuration

```c++
// Enable a normally-disabled warning
diag.setSeverity(diag::sema::warn_implicit_conversion, Severity::Warning);

// Make specific warning an error
diag.setSeverity(diag::common::warn_unused_variable, Severity::Error);

// Downgrade an error to warning (for gradual migration)
diag.setSeverity(diag::sema::err_type_mismatch, Severity::Warning);
```

---

## Extending the System

### Adding a New Diagnostic Category

1. **Update `diagid.hpp`**:

```c++
enum DiagnosticSizes : unsigned {
    // ... existing ...
    DIAG_SIZE_CODEGEN   = 200,
    DIAG_SIZE_LINKER    = 100,  // NEW
};

enum DiagnosticStart : unsigned {
    // ... existing ...
    DIAG_START_CODEGEN  = DIAG_START_SEMA    + DIAG_SIZE_SEMA,
    DIAG_START_LINKER   = DIAG_START_CODEGEN + DIAG_SIZE_CODEGEN,  // NEW
    DIAG_UPPER_LIMIT    = DIAG_START_LINKER  + DIAG_SIZE_LINKER,   // UPDATED
};

// Add new namespace
namespace linker {
enum : DiagID {
    err_undefined_symbol = DIAG_START_LINKER,
    err_duplicate_symbol,
    err_library_not_found,
    warn_unused_library,
};
}
```

2. **Update severity mapping in `error.c++`**:

```c++
Severity DiagnosticsEngine::getDefaultSeverity(DiagID id) const {
    // ... existing code ...
    
    if (id >= DIAG_START_LINKER && id < DIAG_UPPER_LIMIT) {
        if (id >= linker::warn_unused_library) {
            return Severity::Warning;
        }
        return Severity::Error;
    }
    
    return Severity::Warning;
}
```

### Creating a Custom Consumer

```c++
class IDEDiagnosticConsumer : public DiagnosticConsumer {
    std::function<void(const DiagnosticInfo&)> callback_;
    
public:
    struct DiagnosticInfo {
        std::string file;
        unsigned line;
        unsigned column;
        std::string severity;
        std::string message;
        std::vector<std::string> fixits;
    };
    
    explicit IDEDiagnosticConsumer(std::function<void(const DiagnosticInfo&)> cb)
        : callback_(std::move(cb)) {}
    
    void HandleDiagnostic(Severity severity, const Diagnostic& diag) override {
        DiagnosticInfo info;
        info.severity = getSeverityName(severity);
        info.message = diag.message;
        
        // Extract location info...
        
        for (const auto& fixit : diag.fixits) {
            info.fixits.push_back(fixit.code_to_insert);
        }
        
        callback_(info);
        
        if (isErrorOrFatal(severity)) ++num_errors_;
        else if (severity == Severity::Warning) ++num_warnings_;
    }
};

// Usage
IDEDiagnosticConsumer consumer([](const auto& info) {
    // Send to IDE via LSP, IPC, etc.
    sendToIDE(info);
});
diag.setClient(&consumer);
```

### Diagnostic Multiplexer

Forward diagnostics to multiple consumers:

```c++
class MultiplexConsumer : public DiagnosticConsumer {
    std::vector<DiagnosticConsumer*> consumers_;
    
public:
    void addConsumer(DiagnosticConsumer* c) {
        consumers_.push_back(c);
    }
    
    void HandleDiagnostic(Severity severity, const Diagnostic& diag) override {
        for (auto* consumer : consumers_) {
            consumer->HandleDiagnostic(severity, diag);
        }
        
        if (isErrorOrFatal(severity)) ++num_errors_;
        else if (severity == Severity::Warning) ++num_warnings_;
    }
    
    void BeginSourceFile() override {
        for (auto* consumer : consumers_) {
            consumer->BeginSourceFile();
        }
    }
    
    void EndSourceFile() override {
        for (auto* consumer : consumers_) {
            consumer->EndSourceFile();
        }
    }
};
```

---

## Best Practices

### 1. Always Use Diagnostic IDs

```c++
// ✅ Good: Use defined diagnostic ID
diag.Report(loc, diag::sema::err_undeclared_identifier) << name;

// ❌ Bad: Magic numbers
diag.Report(loc, 1001) << name;
```

### 2. Provide Context with Notes

```c++
// ✅ Good: Error with helpful note
diag.Report(use_loc, diag::sema::err_undeclared_identifier) << name;
if (auto* similar = findSimilar(name)) {
    diag.Report(similar->loc, diag::common::note_declared_here) 
        << similar->name;
}

// ❌ Bad: Error without context
diag.Report(loc, diag::sema::err_undeclared_identifier) << name;
```

### 3. Include Fix-It Hints When Possible

```c++
// ✅ Good: Actionable error
diag.Report(loc, diag::parse::err_expected_semicolon)
    << FixItHint::CreateInsertion(loc, ";");

// ❌ Bad: Error without solution
diag.Report(loc, diag::parse::err_expected_semicolon);
```

### 4. Use Appropriate Severity

```c++
// ✅ Good: Unused variable is a warning (code still valid)
diag.Report(loc, diag::common::warn_unused_variable) << name;

// ❌ Bad: Making non-critical issues errors
diag.Report(loc, diag::sema::err_unused_variable) << name;
```

### 5. Check Error Count Before Continuing

```c++
void Compiler::compile() {
    lex();
    if (diag.hasErrorOccurred()) return;
    
    parse();
    if (diag.hasErrorOccurred()) return;
    
    sema();
    if (diag.hasErrorOccurred()) return;
    
    codegen();
}
```

### 6. Limit Error Cascades

```c++
// ✅ Good: Stop after reasonable number of errors
diag.setErrorLimit(20);

// ✅ Good: Skip related diagnostics after first error
if (diag.hasErrorOccurred()) {
    // Skip semantic analysis that would produce cascading errors
    return;
}
```

### 7. Use Source Ranges for Clarity

```c++
// ✅ Good: Highlight the problematic expression
auto range = CharSourceRange::getTokenRange(expr->begin(), expr->end());
diag.Report(expr->begin(), diag::sema::err_type_mismatch)
    << expected << got << range;

// ❌ Bad: Only point location, no range
diag.Report(expr->begin(), diag::sema::err_type_mismatch)
    << expected << got;
```

---

## API Reference

### DiagnosticSeverity.hpp

```c++
namespace aion::diag {
    enum class Severity : uint8_t;
    enum class Flavor : uint8_t;
    bool isErrorOrFatal(Severity S);
    const char* getSeverityName(Severity S);
}
```

### diagid.hpp

```c++
namespace aion::diag {
    // Types
    using DiagID = unsigned;
    
    // Enums
    enum DiagnosticSizes : unsigned;
    enum DiagnosticStart : unsigned;
    enum class DiagClass : uint8_t;
    
    // Functions
    bool isCustomDiag(DiagID ID);
    
    // Diagnostic ID namespaces
    namespace common { enum : DiagID; }
    namespace lex { enum : DiagID; }
    namespace parse { enum : DiagID; }
    namespace sema { enum : DiagID; }
    
    // Classes
    struct DiagnosticInfo;
    class DiagnosticMapping;
}
```

### error.hpp

```c++
namespace aion::diag {
    // Source ranges
    struct CharSourceRange;
    
    // Fix-it hints
    class FixItHint;
    
    // Diagnostic representation
    struct Diagnostic;
    class StoredDiagnostic;
    
    // Consumers
    class DiagnosticConsumer;
    class TextDiagnosticPrinter;
    
    // Builder
    class DiagnosticBuilder;
    
    // Engine
    class DiagnosticsEngine;
    
    // Factory
    std::unique_ptr<DiagnosticsEngine> createDiagnosticsEngine(Source_Manager* sm);
}
```

---

## Conclusion

The Aion Diagnostics Engine provides a robust, extensible system for error and warning reporting. By following the patterns established by LLVM/Clang, it offers:

- **Consistency**: All compiler phases use the same diagnostic infrastructure
- **Clarity**: Colored output with source snippets helps developers understand issues
- **Actionability**: Fix-it hints provide immediate solutions
- **Flexibility**: Severity mappings and custom consumers adapt to any use case

For questions or contributions, please refer to the project repository.
