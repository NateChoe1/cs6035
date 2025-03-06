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

    cc nob.c -o nob
    ./nob
