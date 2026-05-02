# Proposal: "ex" — a separate, statically-typed exception channel for Aion

Status: Draft

Author: (proposal by you)

Date: 2026-04-28

Summary
--------
This proposal introduces a first-class, separate exception channel called `ex` for Aion. Rather than folding errors into function return types, functions declare a list of exception types they may raise. Exceptions are not ordinary return values; they are a distinct channel that can be propagated, caught, or captured as a value. This keeps ordinary returns simple (exactly one return value), supports multiple possible exception types, and lets callers opt into treating exceptions as values when needed.

Goals
-----
- Keep the normal return type of a function singular and simple.
- Allow functions to declare zero or more exception types they may raise.
- Make exception values first-class and storable in variables.
- Provide ergonomic syntax for propagation, capture (value + exception), and handling (catch).
- Maintain static checks so callers must either handle, capture, or propagate exceptions explicitly.
- Allow multiple exceptions from a single function while keeping typechecking tractable.

Non-goals / constraints
----------------------
- This does not force a particular runtime implementation (unwind vs. longjmp vs. explicit check). The design permits multiple backends.
- This is not a full rewrite of `Result<T,E>`-style APIs: those remain available for library code but are not required.

Design overview
---------------
1. Function declarations include an optional `throws` list naming exception types. These are the only types the function may statically `throw`.
2. Throwing is a control-flow action separate from returns. A `throw` does not produce a normal return value.
3. Call sites can do three things:
   - `try` the call, allowing exceptions to propagate (the caller must declare compatible `throws`), or
   - `capture` the call, producing an `Outcome<T>` value that contains either the value `T` or the exception object, or
   - declare the called function's exceptions in its own `throws` list and call normally (implicit propagation).
4. `ex` is the first-class exception handle type. `Outcome<T>` is a simple struct containing either a `T` or an `ex`.

Concrete syntax
---------------
- Function: declare thrown exception types using a `throws` clause.

```aion
fn parse(src: Str) -> AST throws (ParseError, IoError) {
    // body
}

fn read_file(path: Str) -> Bytes throws (IoError) {
    throw IoError { code: 2, msg: "open failed" };
}
```

- Throwing:

```aion
throw ParseError { pos: p, msg: "unexpected token" };
```

- Call site options:

```aion
// 1) propagate with try; caller must declare throws covering callee's throws
fn caller() throws (ParseError, IoError) {
    let ast = try parse(src); // if parse throws, exception propagates out of caller
}

// 2) capture into Outcome<T>
fn caller2() {
    let out = capture parse(src); // out : Outcome<AST>
    if out.is_err() {
        let e: ex = out.exception();
        // handle or store e
    } else {
        let ast = out.value();
    }
}

// 3) declare callee's throws in caller signature and call normally
fn caller3() throws (ParseError) {
    let ast = parse(src); // allowed because caller declares ParseError in throws
}
```

- Catching:

```aion
try {
    let ast = parse(src);
} catch (e: ParseError) {
    // e is statically a ParseError
} catch (e) {
    // e is any ex from ParseError | IoError (must be one of declared throws)
}
```

Type system
-----------
- Function type: `(Args) -> R  throws {E1,...,En}`. `R` is the normal return type. `throws` is metadata that must be checked by callers and the compiler.
- `throw T` is only allowed when `T` is in the current function's `throws` set or is a subtype of a declared exception.
- `try call()` requires the caller function to either declare a `throws` set that is a superset of the callee's `throws` set or to be at top-level/REPL where propagation is permitted.
- `capture call()` yields `Outcome<R>`: a concrete type representing `{ has_exception: bool; value: R; exception: ex }`.
- `ex` also supports dynamic interrogation (downcast to one of declared exception types) and pattern-match-like checking via `catch` arms.

Semantics
---------
- Throwing: `throw E` immediately transfers control to the nearest enclosing `try` block or propagates to the caller. If no handler exists up the chain and the runtime reaches a top-level that doesn't accept propagation, the program terminates with an unhandled exception diagnostic.
- Capture: `capture call()` evaluates the call and returns an `Outcome<R>` that is always a normal value (no propagation). If the call throws, the `Outcome` holds the `ex` and no `R` value.
- Try-propagate: `let v = try call()` is equivalent to calling `call()` and allowing exceptions to bubble upward. The caller must be declared to `throws` the same set or a superset.

**Expression-level exception semantics (critical for arithmetic/binary ops):**
- When a function call that can throw appears in an expression (e.g., `f() + 1`), the type of the entire expression **accumulates** the exception set from all called functions in that expression.
- Example:
  ```aion
  fn f() -> i32 throws (E1) { ... }
  fn g() -> i32 throws (E2) { ... }
  let x = f() + g();  // expr type: i32 throws {E1, E2}
  ```
- The expression type becomes `T throws {E1 ∪ E2 ∪ ...}` (union of all throws in the expr).
- At the point of assignment or use:
  - If assigning to a non-ex-wrapped type `T`, the compiler verifies the caller declares a compatible `throws` set (implicit propagation).
  - If assigning to an ex-wrapped type `T ! {E...}`, the call is implicitly wrapped in `capture` semantics and stored.
  - If the expression is a standalone statement (e.g., `f()`), the compiler requires the caller to declare the throws or it's a compile error.

Data model: `ex` and `Outcome<T>`
---------------------------------
- ex: an opaque, first-class handle to an exception value. Internally it contains a runtime tag indicating the concrete exception type and a pointer/reference to its payload. `ex` is small (pointer-sized) and copyable.
- Outcome<T> (in pseudocode):

```
struct Outcome<T> {
    bool has_exception;
    alignas(T) unsigned char value_storage[sizeof(T)]; // contains T when has_exception==false
    ex exception; // contains the exception when has_exception==true
}
```

- API on Outcome<T>:
  - is_err(): bool
  - is_ok(): bool
  - value(): T (UB unless is_ok())
  - exception(): ex (UB unless is_err())
  - into_result(): Result<T, ex>  // convenience

Expression typing and propagation
----------------------------------
This section addresses the critical case of exceptions in composite expressions.

**Principle**: Every expression that contains a call to a throwing function has a computed exception set. This set is the union of all exception sets from all function calls in that expression.

**Examples**:
```aion
fn f() -> i32 throws (A) { ... }
fn g() -> i32 throws (B) { ... }
fn h(x: i32) -> i32 throws (C) { ... }

// Example 1: arithmetic expression with two throwing calls
let x = f() + g();
// Type of (f() + g()): i32 throws {A, B}
// Assignment to x: i32 requires implicit propagation
// Caller must declare throws {A, B} or superset

// Example 2: nested/chained calls
let y = f() + h(g());
// Type: i32 throws {A, B, C}
// All three exception sets are unioned

// Example 3: assignment to ex-wrapped var (capture semantics)
let z: i32 ! {A, B} = f() + g();
// Implicitly wraps: capture (f() + g())
// z is an Outcome-like value; no implicit propagation
```

**Binary and unary operators**: When operands are expressions with throws, the operator result inherits the throws set. For example:
- `(f() throws {A}) + (g() throws {B})` yielding `i32` has type `i32 throws {A, B}`.
- `-(f() throws {A})` yielding `i32` has type `i32 throws {A}`.

**Function arguments**: When a throwing expression is passed as an argument, the caller must handle the union of throws:
```aion
fn process(val: i32) -> string throws (ParseError) { ... }

let result = process(f() + g());  // f() throws {A}, g() throws {B}
// combined type of process(...): string throws {A, B, ParseError}
```

**Standalone statements**: A standalone call or expression `f();` where `f()` throws {A} is a compile error unless the caller declares `throws {A}` (or superset).

**Try and catch at expression level**: To avoid propagation in an expression context, wrap explicitly:
```aion
let x = try (f() + g());  // try applies to the whole expression
// If any throw occurs, it propagates (caller must handle)

let out = capture (f() + g());  // out: Outcome<i32>
// Captures value and exceptions together
```

AST changes
-----------
- `FuncDecl` additions:
  - store static list of exception types (either as a `ShortVec<MutableType*>` or trailing array like other variable-length data).
  - `num_throws` and pointer/list storage for the types.
- New AST nodes:
  - `ThrowExpr` (or `ThrowStmt` depending on grammar): carries an exception construction expression and source location.
  - `CaptureExpr`: node representing `capture <expr>` and typed to `Outcome<T>`.
  - `TryStmt` / `TryExpr`: a node that can contain a block and list of `CatchClause`s.
  - `CatchClause` node with exception pattern/type and handler body.

Parser changes
--------------
- Extend function-declaration grammar to accept an optional `throws` clause.
- Add grammar for `throw` statements, `capture` expressions, `try/catch` constructs.
- Optional sugar: allow `!` shorthand in function headers (see variants below).

Typechecker changes
-------------------
- Track the current function's declared `throws` set during typechecking. When seeing `throw` ensure the thrown type is allowed.
- **Expression typing**: Compute the exception set for each expression by collecting all `throws` sets from all function calls within that expression. The expression type becomes `T throws {E1 ∪ E2 ∪ ...}`.
- When typechecking assignments and uses:
  - Assignment to non-ex-wrapped type `T`: verify the caller's `throws` set is a superset of the expression's throws set, or emit a compile error (unhandled exception).
  - Assignment to ex-wrapped type `T ! {E...}` or call to `capture(expr)`: the throws are implicitly captured; no propagation check needed.
  - Standalone statements like `f();`: verify caller declares compatible throws or error.
- When typechecking calls:
  - `call()` in a body that does not `capture` or `try` must either be in a function that declares the callee's throws or be an error.
  - `try call()` does not change the type of `call()` (it yields `R`), but the typechecker must ensure the caller's `throws` covers the callee's `throws`.
  - `capture call()` yields `Outcome<R>`.
- `catch` arms narrow the `ex` type inside the arm as appropriate. Optionally enforce exhaustiveness: if all throws in the function are covered by catch arms, the remaining expression type is non-throwing.

Lowering / Codegen considerations
--------------------------------
Two implementation strategies are possible; the AST and type system are agnostic to them:

1) Explicit-outcome lowering (no stack unwind)
   - All functions that can throw return an extra hidden status (or set a thread-local exception handle) that codegen tests after every call. `capture` becomes a direct copy of that status into an `Outcome<T>` value. This strategy is lower-level but maps well to environments where stack unwinding is expensive.

2) Unwind-based lowering (exception-style)
   - Throw creates an exception object and initiates stack unwinding to the nearest handler. Capture is implemented by invoking the call inside a local `try` hand-crafted region that converts the thrown exception into an `Outcome<T>`.

Either approach is compatible with the type rules above. Implementation choice depends on desired runtime performance & target platform.

Examples and idioms
-------------------
- Basic propagation:

```aion
fn parse(s: Str) -> AST throws (ParseError) {
    // ... might throw
}

fn top() throws (ParseError) {
    let ast = try parse("..."); // no need to capture
}
```

- Capture and store exception:

```aion
fn caller_store() {
    let result = capture parse(src);
    if result.is_err() {
        let e: ex = result.exception();
        // store e in a variable for later handling
        saved_err = e;
    }
}
```

- Multiple throws and matching:

```aion
fn f() -> T throws (A, B) { ... }

try {
   let v = f();
} catch (e: A) {
   // handle A
} catch (e: B) {
   // handle B
}
```

- Expressions with multiple throws (arithmetic):

```aion
fn read_x() -> i32 throws (IoError) { ... }
fn read_y() -> i32 throws (IoError) { ... }

fn compute_sum() throws (IoError) {
    let sum = read_x() + read_y();  // expr: i32 throws {IoError}
    // at assignment, implicit propagation check
    // caller must declare throws (IoError)
    return sum;
}

fn compute_safe() {
    let result = capture (read_x() + read_y());  // result: Outcome<i32>
    if result.is_err() {
        // handle IoError
    } else {
        let sum = result.value();
    }
}
```

Migration and interoperability
------------------------------
- Existing `Result<T,E>`-style APIs remain usable. Libraries can provide conversion helpers between `Outcome<T>`/`ex` and `Result<T, E>`.
- For code that currently encodes errors inside returned unions/tuples, migrations can:
  - keep the existing type unmodified (backward compatibility), or
  - split to `throws` + single return, then update callers to use `try`/`capture` as needed.

Exhaustiveness and compatibility
--------------------------------
- The compiler can optionally check that `catch` arms exhaust the declared `throws` list for a `try` block.
- A function's `throws` clause is open to subtyping: a function may declare a general exception type while throwing subtypes.
- Allow `throws (any)` shorthand to disable checking (useful for FFI or quick prototyping).

Tests to add
------------
- Positive: `throw` a declared type and `try` propagate.
- Positive: `capture` collects exception into `Outcome<T>`.
- Negative: `throw` an undeclared type yields a compile-time error.
- Negative: calling a throwing function without handling/declaring throws is a compile-time error.
- Multiple exceptions from callee, ensure correct arm selection in `catch`.
- Storing `ex` into variables and later rethrowing / inspecting.

Open questions
--------------
- Exception subtyping rules (how to define and check subtypes for exceptions).
- Runtime representation: boxed vs. inline small-object optimization.
- Whether `throws` must be strictly declared (closed set) or can be open/opaque for easier FFI.
- Shortcut syntax choices (e.g. `!` as shorthand vs `throws(...)`).

Appendix: small grammar sketch
-----------------------------

```
function_decl : 'fn' IDENT '(' param_list ')' '->' type ('throws' '(' type_list ')')? block
throw_stmt    : 'throw' expr ';'
capture_expr  : 'capture' expression
try_stmt      : 'try' block ('catch' '(' pattern ')' block )*
```

Conclusion
----------
This design separates exceptions into an explicit, statically-typed channel that integrates with the type system without contaminating function return types. It is ergonomically flexible (propagate with `try`, capture with `capture`, or declare propagation at the caller), statically checkable, and maps to multiple runtime implementation strategies. It keeps the language's multi-return/multiple-value features intact while providing a clean mechanism for multiple error kinds.





