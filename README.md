# Toy Compiler

A small compiler for a C-like language that parses source code and emits three-address intermediate representation (IR).

The compiler supports:

* Variable declarations
* Arithmetic and boolean expressions
* Arrays
* Blocks and nested scopes
* `if` / `else`
* `while`
* `do-while`
* `break`

The generated output is three-address code suitable for later optimization or code generation passes.

---

## Grammar

```text
program     -> block

block       -> { decls stmts }

decls       -> decls decl | e
decl        -> type id ;

type        -> type [ num ] | basic

stmts       -> stmts stmt | e

stmt        -> loc = expr ;
             | if ( expr ) stmt
             | if ( expr ) stmt else stmt
             | while ( expr ) stmt
             | do stmt while ( expr ) ;
             | break ;
             | block

loc         -> loc [ expr ] | id ;

expr        -> expr || join | join

join        -> join && equality | equality

equality    -> equality == rel
             | equality != rel
             | rel

rel         -> arith < arith
             | arith <= arith
             | arith > arith
             | arith >= arith
             | arith

arith       -> arith + term
             | arith - term
             | term

term        -> term * unary
             | term / unary
             | unary

unary       -> ! unary
             | - unary
             | factor

factor      -> ( expr )
             | loc
             | num
             | real
             | true
             | false
```

---

## Intermediate Representation

The compiler emits three-address code in the following formats:

### Arithmetic / Binary Operations

```text
t = a op b
```

Where `op` may be:

* `add`
* `sub`
* `mul`
* `div`
* `access`

Example:

```text
t1 = a add b
```

---

### Unary Operations

```text
t = op b
```

Where `op` may be:

* unary minus
* cast
* logical not

Example:

```text
t2 = not x
```

---

### Assignment

```text
s = a
```

Example:

```text
a = t1
```

---

### Array Assignment

```text
s[t] = a
```

Example:

```text
arr[i] = x
```

---

### Conditional Branch

```text
if a rel b goto L
```

or

```text
iffalse a rel b goto L
```

Example:

```text
if x lt y goto L1
```

---

### Unary Conditional Branch

```text
if unary b goto L
```

or

```text
iffalse unary b goto L
```

---

### Unconditional Branch

```text
goto L
```

---

### Labels

```text
L
```

Example:

```text
L1:
```

---

## Example

### Input

```c
{
    int a;
    a = 0;

    while (true) {
        a = a + 1;

        if (a == 10)
            break;
    }

    a = a - 2;
}
```

### Output

```text
a = 0

L2:
t1 = a add 1
a = t1

iffalse a eq 10 goto L5
goto L1

L5:
goto L2

L1:
t2 = a sub 2
a = t2
```

---

## Building

Compile with:

```bash
gcc *.c -o a.out
```

For debugging:

```bash
gcc -g -O0 *.c -o a.out
```

---

## Running

Run the compiler with:

```bash
./a.out <filepath>
```

Example:

```bash
./a.out examples/test.toy
```

---

## Notes

* The grammar is based on the appendix grammar from the second edition of the Dragon Book (Compilers: Principles, Techniques, and Tools).
* All compiler code, including lexer, parser, semantic analysis, and IR emission, is handwritten.
* No parser generators or compiler frameworks are used.
* This README was generated with assistance from AI.
