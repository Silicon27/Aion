# Ownership and lifetime semantics in Aion

Object lifetimes typically follow a stack-like structure, where objects are created and destroyed in a last-in-first-out (LIFO) manner. When an object is created, it is allocated on the stack, and when it goes out of scope, it is automatically deallocated. This means that the lifetime of an object is determined by its scope in the code.

Also typically, lifetime is inferred by the scope of declaration. However, what if you return a reference to a stack-allocated object who's lifetime is tied to the current scope (say, some function `f`)? Would the caller of `f` be able to use that reference after `f` has returned? No, because the object would have been deallocated once `f` returns, leading to undefined behavior if the caller tries to access it.

However, we can prevent this exact scenario by instead returning the lifetime of the reference to the stack-allocated object, and then changing the lifetime of that reference to be that of the caller.
```rust
callee() -> ref i32 {
    let x: ref i32 = 42; // x is allocated on the stack
    return lifetime ref x; // returns the lifetime of the reference to x
}

caller() -> i32 {
    let y:kref i32 = callee(); // x's lifetime is changed to be that of y
    return *y; // safe to dereference y, as it has the same lifetime as x
}
```

The lifetime of `x` is automatically extended to match the lifetime of `y`, allowing `caller` to safely use the reference returned by `callee`. This approach ensures that the reference remains valid as long as it is used within the scope of `caller`, preventing any undefined behavior that would arise from accessing a deallocated object.

`lifetime` is a hint to the compiler to prospectively look-forward to the call site for potential to extend the lifetime of the reference being returned.

> [!NOTE]\
> Dev note: during sema, when assignment is done on a function that returns a `ref` of any sorts, attach that variable name to a **call site reference move variable** and thereafter when `return` is seen, we move the returned object's lifetime from the callee's list to the caller's. This is done to remove the responsibility of specifying returning of `lifetime ref` explicitly.  

### Ownership semantics

Take this example:

```rust
let i: i32 = 2; // `i` owns the value 2 allocated on the stack
let x: ref i32 = i; // `x` owns nothing, an alias
let own_i: i32 [=] i; // owns `i`, `i` is transformed into a `ref i32`
// let wrong: i32 = i; // ownership transferal if disallowed unless `[=]` is used
```

Line 1 and 2 are pretty clear:
1. `i` is allocated on the stack with the value 2. 
2. `x` is a `ref i32` that aliases `i`.
3. declares a variable `own_i` that owns `i`'s resources (via resource transferal operator `[=]`), `i` is thereafter implicitly casted to a `ref i32`, aliasing `own_i`.
4. this line is wrong, as `i` may not be directly addressed whilst not being a [sub-expression of a larger expression](../spec/expressions.md#sub-expressions).
