// to cast objects upstream and downstream, we use compile time type conversion.
// the compiler inherently stores the layout and type information of all the types,
// so we can use that to convert between types when needed, at compile time.

let x = 2; // inferred as i32

// to cast x to f32, a widening conversion, we can do:
let y = x as f32; // y is now 2.0 as f32

// this is performed at compile time, and types where widening conversions are applicable,
// can be cast via the user defining how internal members of the type are meant to be
// default initialized (for downcasting) or in the case of an upcast, the compiler
// would remove the members that are not present in the target type, and keep the ones that are.