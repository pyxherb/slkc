# About The Compiler

The compiler has 3 main modules:

* The AST framework
* The compilation framework (including semantic analysis)
* The language server (depends on the compilation framework) (not implemented yet.)

## Project Structure

* `ast/` - The AST framework
  * `ast/nodedefs/` - AST node definitions
  * `ast/parser/` - Implementation codes of parser that generates red-green tree nodes from source codes
  * `ast/rgtree` - Red and green tree definitions
* `comp/` - The compilation framework
  * `comp/rg2ast` - Module that lowers red-green trees into ASTs.

## Issues

### Path-based Null Checker

Implement the path-based null checker.

```slake
let a : i32?;

a = 123;

// a is i32.

let b : i32?;

if (b === null) {
    // b is null.
} else {
    // optional, b is i32.
}
// b is perhaps i32?,

let b : i32?;

if (b === null) {
    // b is null.
    return;
}
// b is i32,
```

We have to be carefully to deal with following cases:

```slake
let a : i32?;

if (true) {
    a = 123;
} else {
    a = null;
}
// Then a should be i32.
```

```slake
let a : i32?;

while (true) {
    a = 123;
    if (true)
        break;
    a = null;
}
// Then a should be i32.
```

### Constructor Initialization Checker

Implement the constructor initialization checker.

Mostly in the same way of the path-based null checker.

Take notice of the initialization completion point, and switch the object type
using `DCMT` instruction on that time point.

### Variable Nullity/Estimated Value Validity Restriction

We should restrict the variable's nullity and estimated value validity to local
variables or synchronized variables.

## Dumped Object Management

Currently we use `.release()` for pushing and inserting subobjects in JSON.
Use `.get()` then `.release()` instead.
