# [var.decl]

Variable declarations are initiated with `let`, with attributes specified after `let`.
Every variable is by default immutable.

### Grammar:
```bnf
declaration ::= "let" ["mut" | "comp"] identifier [":" type] ["=" expression] ";"
```

## [var.decl.mut]

To enable mutability, the `mut` keyword must be specified. 
```aion
let mut x = 2;
x++; // allowed

let y = 2;
y++; // error: mutation attempted on an immutable declared variable
```

## [var.decl.comp]
Any variable that is declared `comp` will have their result be compile-time computed. 
A `comp` declared variable restrains its operant to be strictly **compile time computable**. 
Any given operant, be it a [primary expression](expressions.md#exprprim) or a function invocation, would require that the said expression to be equally `comp` declared. 
```
a() comp :: i32 {
    return 2;
}

let comp x = a() + 1; // legal

let y = 2;

let comp z = y + 1; error: operant not comp declared 
```


## [var.decl.dedc]

Type deduction bases off the following rules:
1. if the deduced type imposes type errors after substitution, the compiler is guaranteed to retry type deduction by appling applicable qualifiers/sub-type