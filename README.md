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

Yex is a lexer. It's pretty dumb, to invoke it just run

    ./dist/yex input.yex output.c

The generated C code relies on `<yex.h>`, as defined in `dist/yex.h`. The input
file format is like this:

    #include <stdio.h>
    #define YEX_NAME yylex
    /* other header stuff */
    %%
    !regex 1
    puts("result 1");
    !(regex 2)*
    puts("result 2");

This is extremely stupid, and I will absolutely improve this later.

The exclamation marks indicate that these regular expression matches are greedy.

The outputted C code is guaranteed to define this function:

    int YEX_NAME(struct yex_buffer *buffer);

This function may modify the contents of `buffer->parsed`.

If `YEX_NAME` isn't defined, we default to the name `yex_read`

# Lacc

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

Lines 3 gives metadata about the grammar

    4 7

indicates that this grammar contains 5 tokens in total, 3 of which are
terminals. This means that tokens 0, 1, and 2 are terminals, and tokens 3 and 4
are nonterminals. This specific grammar is an expression parser which can handle
addition and multiplication with operator precedence. Some more helpful names
for the tokens might be these:

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
