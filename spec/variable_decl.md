# [var.decl]

Variable declarations are initiated with `let`, with attributes specified after `let`.
Every variable is by default immutable.

### Grammar:
```bnf
declaration ::= "let" ["mut" | "comp"] identifier [":" type] ["=" expression] ";"
```

## [var.decl.mut]

To enable mutability, the `mut` keyword must be specified. 

## [var.decl.comp]



## [var.decl.dedc]

Type deduction bases off the following rules:
1. if the deduced type imposes type errors after substitution, the compiler is guaranteed to retry type deduction by appling applicable qualifiers/sub-type