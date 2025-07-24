# VMake

## Grammar

```ebnf
program = declaration* EOF;

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
unary = ( ( "!" | "-" ) unary ) | call;
call = primary ( ( "(" arguments? ")" ) | ( "." identifier ) )*;
primary = number | string | literal | grouping | identifier ;

arguments = assignment ( "," assignment )* ;
grouping = "(" expression ")" ;
number = digit_excluding_zero digit* ;
string = """ ascii_character_excluding_zero """ ;
literal = "true" | "false" | "null" ;
identifier = ( alphabetical | "_" ) ( alphanumerical | "_" )* ;

digit_excluding_zero = "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
digit = "0" | digit_excluding_zero ;
```

## Basic C program

Consider a C program with the following structure:

```tree
├── build
├── VMake.txt
├── include
│   └── myprog.h
├── private
│   └── myprog-priv.h
└── src
    └── myprog.c
```
