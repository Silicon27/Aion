# Aion 
Aion is a compiled and imperative language aimed at provisioning requested memory whilst allowing maximum control.

Memory provisioning is an operation the compiler provides to requested memory 

Example code snippet:

```rust
@use <core/io>

main() :: void { 
    let v = 1; // type inference
    io.print(v);
    return 0;
}

```


### Concepts
Aion introduces `contracts`, which is a form of allowance or constraint between the programmer and the compiler.

```rust
@use <core/contracts>

// form alias if needed
// contract nopvs = contracts.nopvs

// nopvs is a compiler promise to never provision memory used in my_ctr
<contract #contacts.nopvs> // no provisioning, dangerous
execution my_ctr(x: i32) :: i32 { // int is defined as an alias of int32
    return x;
}
```

> A `contract`defines an allowance or constraint between the programmer and the compiler. 
> 
> the `execution` of said contract is formed similar to that of a function
The cboard (contract board) keeps tab on contracts present at CET, or, Contract Enforcement Time.
> 
> idea: make a language that is for systems, but one that has a repl. The important aspect of this is that we are able to experiment with raw memory as if we were programming in a file