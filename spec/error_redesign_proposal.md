# Formal Proposal: Aion Exceptional Type System (AETS)

## Abstract
This paper proposes a fundamental redesign of error handling in the Aion programming language. The current model, which treats exceptions as alternate return values (e.g., `T or E`), is replaced with a dedicated exceptional channel. This change decouples success-path logic from failure-path tracking, ensuring that functions have a singular primary return value while maintaining full compile-time visibility into potential error states. We introduce "Ex-wrapped" types for unified variable storage and define formal semantics for error propagation and handling.

## 1. Motivation
The legacy `T or E` syntax presents several challenges:
1. **Type Ambiguity**: It treats errors as valid data returns, making it difficult to distinguish between a function returning a union and a function that might fail.
2. **System Complexity**: Supporting multiple return values (or union returns) complicates code generation and optimizations.
3. **Ergonomic Friction**: Explicitly handling every union return becomes verbose, leading to either "panic-everywhere" or excessive boilerplate.

The AETS aims to provide the safety of checked exceptions with the ergonomics of modern error types, without the overhead of complex multiple-return-value logic in the core type system.

## 2. Syntax Specification

### 2.1 Function Declarations
The function signature is updated to separate the return type from the exception set using the `!` operator.

**EBNF Grammar:**
```ebnf
func_decl         = "fn" identifier "(" [param_list] ")" [ "->" return_type ] [ "!" exception_set ] block ;
exception_set     = exception_type { "," exception_type } ;
exception_type    = identifier ;
```

**Example:**
```aion
fn load_config(path: string) -> Config ! FileError, ParseError {
    let raw = read_file(path)!; // Propagates FileError if unhandled
    return parse_yaml(raw)!;    // Propagates ParseError if unhandled
}
```

### 2.2 Ex-wrapped Variable Types
Variables can explicitly store the result of a fallible operation using the `!` type modifier.

**Grammar:**
```ebnf
ex_wrapped_type   = base_type "!" "{" exception_set "}" ;
```

**Example:**
```aion
let result: i32 ! {HttpError, Timeout} = risky_call();
```

## 3. The Exceptional Channel Semantics

### 3.1 Ex-Wrapped Values
An ex-wrapped value of type `T ! {E...}` is a compiler-managed sum type. 
- **Storage**: Internally represented as a tagged union. The tag (discriminant) identifies whether the value is a valid `T` or one of the exceptions in the set.
- **Memory Layout**: `[Discriminant (u32)] [Payload (Max(sizeof(T), max(sizeof(E...))))]`.

### 3.2 Implicit Bubbling (The Happy Path)
When a fallible function is called or an ex-wrapped value is used:
1. If the value is coerced to a non-ex-wrapped type `T` (e.g., assignment to a non-ex variable, passed as a non-ex function argument, or used in a non-propagating context), the compiler inserts an implicit "bubble" check.
2. If the value contains an exception, the current function immediately returns (bubbles) that exception.
3. This requires the calling function to include the exception in its own `!` set.

```aion
fn outer() ! MyError {
    let x: i32 = inner(); // inner() returns i32 ! {MyError}
    // Coercion to i32 triggers implicit bubbling.
}
```

### 3.3 Explicit Capture
If the result is assigned to an ex-wrapped variable or the expression remains in an ex-wrapped state, no bubbling occurs. The variable or expression result captures the state.

```aion
fn outer() {
    let x = inner(); // x is i32 ! {MyError}
    if x.is_ex() {
        // handle it
    }
}
```

### 3.4 Propagation and Short-circuiting
Exceptions propagate upward through the expression tree. An expression containing one or more fallible sub-expressions evaluates to an ex-wrapped type.

- **Short-circuiting**: Evaluation follows a strict left-to-right order. If a sub-expression evaluates to an exception, all subsequent parts of the larger expression are skipped.
- **Type Union**: The exception set of an expression is the union of the exception sets of all its sub-expressions.

**Example: `f() + 1`**
If `f()` returns `i32 ! {Error}`, the expression `f() + 1` has type `i32 ! {Error}`.
1. `f()` is evaluated.
2. If `f()` returns an exception, the `+ 1` operation is never performed.
3. The exception is returned as the result of the entire expression.
4. If this result is then assigned to an `i32`, it bubbles (see 3.2).

## 4. Operators and Built-ins

### 4.1 The Unwrap Operator (`!`)
Applied to an ex-wrapped value `v`, `v!` extracts the value of type `T`.
- **Semantics**: If `v` contains an exception, the program panics with a stack trace. Otherwise, it evaluates to `T`.

### 4.2 The `catch` Mechanism
The `catch` operator allows for branching based on the exceptional state.

**Expression Catch:**
Evaluates to a default value if an exception is present. This effectively "flattens" the ex-wrapped type into the base type.
```aion
let val: i32 = risky() catch 42;
```

**Block Catch:**
Provides access to the exception object and allows for partial handling.
```aion
let res = risky() catch e {
    match e {
        HttpError => { 
            print("Handled HttpError"); 
            yield 0; // Provide a fallback value
        },
        _ => throw e; // Re-throw unhandled exceptions
    }
};
```
*Note: If a `catch` block handles all possible exceptions in the set, the resulting type is the base type `T`. If it handles only a subset, the resulting type is `T ! {Remaining_E...}`.*

### 4.3 Type Reflection
- `v.is_ex()`: Returns `bool`.
- `v.get_ex()`: Returns the exception payload as an `ex` base type (or `none` if valid).

## 5. Edge Cases and Advanced Semantics

### 5.1 Nested and Compound Expressions
The system handles nesting by lifting the exceptional state to the outermost level of the expression that is not explicitly handled or captured.

**Nested Calls:**
In `f(g())`, where both `g` and `f` can throw:
- If `g()` throws `E1`, `f` is never called.
- If `g()` succeeds but `f()` throws `E2`, the result is `E2`.
- Result type: `ReturnOfF ! {E_g \cup E_f}`.

**Arithmetic/Logic Expressions:**
In `a() + b() * c()`:
- If `a()` throws, `b()` and `c()` are not called.
- If `a()` succeeds, `b()` is called. If `b()` throws, `c()` is not called.
- Exceptions are "first-come, first-propagated".

**function evaluation order:**
- The order of function evaluation (value substitution) is from left to right, with each succeeding function call enabling the next to be evaluated. 
If any function call throws, the later calls are skipped and the exception is propagated.
- that is, given an expression as `let x = a() + b() * c()`, if `a()` throws, `b()` and `c()` are not evaluated, and the type of x is `i32 ! {E_a}`.
- this evaluation order and type deduction is also true for when `b()` or `c()`

### 5.2 Void Functions
Functions returning nothing (`void` or `unit`) can still throw.
```aion
fn cleanup() ! PermissionError { ... }
let res = cleanup(); // res is void ! {PermissionError}
```

### 5.3 Higher-Order Functions and Closures
Exception sets are part of the function type.
```aion
fn map(f: fn(i32) -> i32 ! E, list: [i32]) -> [i32] ! E { ... }
```
The generic `E` represents the union of all exceptions thrown by the closure.

### 5.4 Variance and Subtyping
- `T ! {E1}` is a subtype of `T ! {E1, E2}`. An ex-wrapped value with fewer potential errors can be safely assigned to one expecting more.
- `T` is a subtype of `T ! {E}`. A guaranteed value can be "promoted" to an ex-wrapped state.

### 5.5 Recursive Functions
Recursive functions must declare the union of all exceptions thrown in any branch. Since the set of exceptions is finite and defined at compile-time, the exception set does not "grow" infinitely during recursion; it simply converges to the set of all `throw` statements reachable in the call graph.

### 5.6 Control Flow Integration
Control flow structures that require a specific base type (e.g., `bool` for `if`, `iterable` for `for`) trigger implicit bubbling if provided with an ex-wrapped value.

- **`if` statements**: `if (fallible_bool())` bubbles if an exception occurs.
- **`for` loops**: `for (x in fallible_list())` bubbles if the list retrieval fails.
- **`match` expressions**: `match fallible_val()` bubbles before entering any branch if `fallible_val()` is an exception. To handle the exception within the match context, the value must be explicitly caught or assigned to an ex-wrapped variable first.

## 6. Comparison with Existing Models

| Feature | Aion (Proposed) | Swift | Rust | Zig |
| :--- | :--- | :--- | :--- | :--- |
| **Primary Channel** | `ReturnType` | `ReturnType` | `Result<T, E>` | `E!T` |
| **Error Channel** | Checked Exception Set | Checked Exceptions | Part of Return Type | Error Sets |
| **Implicit Bubbling** | Yes (via type match) | Yes (`try`) | No (`?` operator) | Yes (`try`) |
| **Storage** | Integrated Sum Type | Opaque | Manual Enum | Payload Union |
| **Multiple Errors** | Yes (Set-based) | No (Typed `Throws` is new) | No (Union needed) | Yes (Sets) |

## 7. AST Representation (Implementation Strategy)
The `FuncDecl` node in the AST will be restructured:
1. `ReturnType`: A single `MutableType` pointer.
2. `ExceptionSet`: A `SmallVector` or trailing array of `MutableType` pointers representing the checked exceptions.
3. `Flags`: A bit to indicate if the function is fallible (even if the set is currently empty, e.g., for future-proofing).

The `CallExpr` will now be responsible for generating "Bubble" or "Store" instructions depending on the expected type of the assignment.

## 8. Conclusion
The Aion Exceptional Type System provides a robust, compile-time verifiable method for error handling. By separating the return channel from the exception channel, we simplify the core type system while providing developers with powerful tools for explicit error management and implicit propagation.
