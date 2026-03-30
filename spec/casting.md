# Casting

## [cst.com]
Aion does not accept implicit casting. 
Any conversion is required to be cast explicitly, and if static analysis of type coherency in a given expression fail, compilation fails. 

```
let x = 5; // i32
let y = x as f32;

let z: f32 = x; // error: declared variable is not initialized with an expression of the same type 
```

