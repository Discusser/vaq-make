# VMake

## Grammar

```ebnf
program = declaration* EOF ;

declaration = statement ;

statement = print_statement | include_statement | expression_statement ;
print_statement = print grouping ";" ;
include_statement = include string ";" ;
expression_statement = expression ";" ;

expression = assignment ;
assignment = ( identifier "=" assignment ) | equality ;
equality = comparison ( ( "==" | "!=" ) comparison )* ;
comparison = term ( ( "<" | "<=" | ">" | ">=" ) term )* ;
term = factor ( ( "+" | "-" ) factor )* ;
factor = unary ( ( "*" | "/" ) unary )* ;
unary = ( ( "!" | "-" ) unary ) | subscript;
subscript = call ( "[" expression "]" )* ;
call = primary ( ( "(" arguments? ")" ) | ( "." identifier ) )*;
primary = number | string | literal | array | grouping | identifier ;

arguments = assignment ( "," equality )* ( "," identifier "=" equality )* ;
grouping = "(" expression ")" ;
number = digit_excluding_zero digit* ;
string = """ ascii_character_excluding_zero """ ;
literal = "true" | "false" | "nil" ;
array = "[" assignment ( "," assignment )* "]"
identifier = ( alphabetical | "_" ) ( alphanumerical | "_" )* ;

digit_excluding_zero = "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
digit = "0" | digit_excluding_zero ;
```

Note that comments are also allowed in VMake, and can be started with a `#`. They only span on one line.

## Basic C program

Consider a C program with the following structure:

```tree
├── build
├── VMake.vmake
├── include
│   └── myprog.h
└── src
    └── myprog.c
```

An example of a VMake file for this program would be:

```vmake
executable(
  "myprog", 
  sources=[
    "src/myprog.c", 
  ],
  include_directories=["include"],
  link_libraries=["m"]); # if you want the math library for example, equivalent to -lm
```

The build directory can then be populated with `vaq-make VMake.vmake . build/`. Inside, a Makefile with different targets will be generated. Most useful to the user are the targets that have the same names as the ones defined in the VMake file (in this case, "myprog"). The generated Makefile is also capable of regenerating the build configuration whenever changes are made to the source VMake file.
