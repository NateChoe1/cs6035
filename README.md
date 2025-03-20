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
    regex 1
    puts("result 1");
    (regex 2)*
    puts("result 2");

This is extremely stupid, and I will absolutely improve this later.

The outputted C code is guaranteed to define this function:

    int YEX_NAME(struct yex_buffer *buffer);

This function may modify the contents of `buffer->parsed`.
