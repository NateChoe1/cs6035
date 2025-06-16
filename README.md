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

## Stone

Stone is a lexer. It tries to be compatible with POSIX Lex.

### Implementation-specific behavior:

The default type of `yytext` is `char[]`

The symbols in `lex.yy.c` are defined as follows:

```c
YYLEX_V int yylex(void);    /* #define YYLEX_V */
YYMORE_V int yymore(void);  /* #define YYMORE_V static */
YYLESS_V int yyless(int n); /* #define YYLESS_V static */
YYINPUT_V int input(void);  /* #define YYINPUT_V static */
YYUNPUT_V int unput(int c); /* #define YYUNPUT_V static */

/* if using %pointer */
static char yytext_data[YYTEXT_SIZE];
YYTEXT_V char *yytext = yytext_data; /* #define YYTEXT_V */

/* if using %array */
YYTEXT_V char yytext[YYTEXT_SIZE];   /* #define YYTEXT_V */

YYLENG_V int yyleng; /* #define YYLENG_V */
YYIN_V FILE *yyin; /* #define YYIN_V */

YYRESET_V int yyreset(void); /* #define YYRESET_V static */

YYERROR_V char *yyerror(int code); /* #define YYERROR_V static */
```

The visibility macros macros (i.e. `YYLEX_V`, `YYMORE_V`, etc.) are defined with
a redefinition guard, as in this snippet:

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

The extension function `yyreset()` will reset the lexer for further use with a
different file by throwing away any internal buffers and resetting all state.
The file pointer that was previously stored in `yyin` SHOULD NOT be reused. This
function returns 0 on success and an error code greater than zero on failure.

The extension function `yyerror()` will take as input an error code returned by
some other function and return a string providing an explanation of what error
occurred.

Defining a visibility macro as anything other than `static` or the empty string
is undefined behavior. No `const int yyleng`, for example.

## Moyo

Moyo is currently unimplemented, but it will eventually become a parser
generator which is hopefully compatible with POSIX yacc.

## Standard references

* [POSIX regular
  expressions](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap09.html)
* [POSIX character
  classes](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html)
* [lex](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/lex.html)
* [yacc](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/yacc.html)
* [getopt](https://pubs.opengroup.org/onlinepubs/9799919799/functions/getopt.html)
* [Command argument syntax](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap12.html#tag_12_02)
