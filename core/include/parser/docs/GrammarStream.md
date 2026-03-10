# `GrammarStream`


```c++
std::string id;
std::vector<std::string> args;

auto rule = 
    identifier(id) 
    >> ch("(")
    >> optional(
        seq(
            ch(",") 
            >> 
            identifier(args)
        ))
    >> ch(")")
```

This parses a simple expression similar to the following 
```c++
my_id(arg1, arg2)
```
where `id = "my_id"` and `args = ["arg1", "arg2"]`

This is thereafter easily traversable and recoverable upon parsing fault.

Given some input `this(,)` to our `rule`:
```c++
Recovery r(diag_engine); // pre-defined DiagnosticEngine instance, any errors spotted on recovery and subsequent hints are stored to this engine
std::vector<Token> toks = {"this", "(", ",", ")"};

rule.parse(toks, 0) // this faults! 

if (rule.is_faulty()) {
    r.recover(rule);
}
```
