# OpenCourseWare 6.035

This repository contains some snippets of me working through [OpenCourseWare
6.035](https://ocw.mit.edu/courses/6-035-computer-language-engineering-spring-2010/pages/lecture-notes/),
Computer Language Engineering.

Right now I'm just trying to wrap my head around LALR parsing, but eventually
this may turn into something much bigger.

Some of this code does depend on implementation defined behavior, but most of
the things it depends on are pretty universal. Files which contain
implementation defined behavior are tagged with the `IMPL_DEF` tag. They can be
found with this command:

    grep -r IMPL_DEF

## Building:

This project uses [https://github.com/tsoding/nob.h](nob.h) to build itself. To
compile this program, just run these commands:

    cc nob.c -o nob # you only have to run this once
    ./nob

## Testing:

    cc nob.c -o nob
    ./nob test
    ./build/cc

## Yex

Yex is a lexer. It tries to be compatible with POSIX Lex.

### Implementation-specific behavior:

The default type of `yytext` is `char *`

The symbols in `lex.yy.c` are defined as follows:

```c
YYLEX_V int yylex(void);    /* #define YYLEX_V */
YYMORE_V int yymore(void);  /* #define YYMORE_V static */
YYLESS_V int yyless(int n); /* #define YYLESS_V static */
YYINPUT_V int input(void);  /* #define YYINPUT_V static */
YYINPUT_V int unput(int c); /* #define YYUNPUT_V static */
```

The macros `YYLEX_V`, `YYMORE_V`, are defined with a redefinition guard, as in
this snippet:

```c
#ifndef YYINPUT_V
#define YYINPUT_V static
#endif
```

The default values of these macros were shown in the comments of an earlier
snippet.

You can change the names of the `yylex`, `yymore`, `yyless`, `input`, and
`unput` symbols with a macro definition in the definitions section, like so:

```lex
%{
 #define yylex get_token
 #define yymore append_next
 /* and so on */
%}

%%

/* some lex rules */

%%

/* some user subroutines */
```

The `%p`, `%n`, `%a`, and `%k` directives are ignored.

## Lacc

Lacc is a parser generator. It's also pretty dumb. To invoke it just run

    ./dist/lacc input.lacc output.c

The input file looks like this

    #include "atoin.h"
    %%
    4 7
    lacc_v = atoin(closure->input + closure->t_start, closure->parsed - closure->t_start);
    ;
    ;
    ;
    6 3
    5 0
    lacc_v = LACC_V(0);
    5 5 2 0
    lacc_v = LACC_V(0) * LACC_V(2);
    4 5
    lacc_v = LACC_V(0);
    4 4 1 5
    lacc_v = LACC_V(0) + LACC_V(2);
    6 4 3
    ;

Yikes! Let's break it down

The header is included at the top of the generated C file

    #include "atoin.h"
    %%

(note that the `%%` is not included, and simply indicates the end of the
header.)

Line 3 gives metadata about the grammar

    4 7

indicates that this grammar contains 7 tokens in total, 4 of which are
terminals. This means that tokens 0, 1, 2, and 3 are terminals, and tokens 4, 5,
and 6 are nonterminals. This specific grammar is an expression parser which can
handle addition and multiplication with operator precedence. Some more helpful
names for the tokens might be these:

    number     new name
    0          integer
    1          +
    2          *
    3          EOF
    4          statement
    5          statement_0
    6          START

The next 4 lines are run after we read in each of the terminals.

    lacc_v = atoin(closure->input + closure->t_start, closure->parsed - closure->t_start);
    ;
    ;
    ;

After we read in token 0 (integer), read the integer value from the closure and
set `lacc_v` (the value for the current token). Tokens 1, 2, and 3 do nothing,
since they have no data associated with them.

    6 3

indicates that token number 6 is the start token, and that token number 3 is the
EOF token

    5 0

indicates that the production `5 -> 0` is in the grammar

    lacc_v = LACC_V(0);

indicates that when we take this reduction, we should copy the value of the
token.

    5 5 2 0

indicates that the production `5 -> 5 2 0` is also in the grammar

    lacc_v = LACC_V(0) * LACC_V(2);

indicates that when we take this reduction, we should multiply the values of the
two operands and use that as the value.

The file continues with more productions until EOF.

In this case, we use two statement tokens because lacc doesn't handle operator
precedence yet.

The generated C code is guaranteed to define this function:

    LACC_TYPE LACC_NAME(int (*get_token)(LACC_CLOSURE closure), YACC_CLOSURE closure);

`LACC_TYPE` is sort of equivalent to Yacc's `%union` directive, but as a
preprocessor directive. By default it's `int`.

We also assume that `get_token` returns -1 on EOF, and `< -1` on error.

When we encounter a parse error, we return `LACC_ERROR`.

## Standard references

* [POSIX regular
  expressions](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap09.html)
* [POSIX character
  classes](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html)
* [lex](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/lex.html)
* [getopt](https://pubs.opengroup.org/onlinepubs/9799919799/functions/getopt.html)
* [Command argument syntax](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap12.html#tag_12_02)
