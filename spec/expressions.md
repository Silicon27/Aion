# Expressions

## [expr.valcat.named]

Expressions are split into 2 distinct value categories: **named** and **unnamed**.
Any expression that does not name (implying impermanence) nor address a value in either persistent, heap, or static storage is qualified as an unnamed expression. 
```aion
1 + 1; // computed but unused; an expression, although unnamed
x + 1; // unnamed 
```

Any expression that names (implying permanence) or addresses a value in either persistent, heap, or static storage is qualified as a named expression
```aion
let x = 2; // x is addressable, and a memory location is given to it.
let comp y = 4; // y is also addressable, even if the computation for said variable is completed at compile time
```

## [expr.prim]

Primary expressions