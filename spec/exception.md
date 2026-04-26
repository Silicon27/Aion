# Exceptions

Errors in Aion are handled and represented via exceptions.
Exceptions are declared via the `ex` keyword; the keyword also representing a special builtin type: `ex`
All later exceptions are therefore a derivative of `ex`.

```aion
ex SimpleException; // declares a simple exception that has no payload

ex ExceptionWithPayload => f"simple payload, this is printed when this exception is thrown";

// exceptions can also have fields
ex ExceptionWithFields {
    string details;
} => f"Exception was thrown with {details}"

// a function can throw an exception with the `throw` keyword
fn throw_fun() -> i32 or ExceptionWithFields {
    throw ExceptionWithFields { details: "A function threw me!" }; // this counts also as a return to the callee
}


```