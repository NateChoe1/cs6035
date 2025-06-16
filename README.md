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

> **AUTHOR'S NOTE**: I'll be adding in some extra commentary in block quotes to
> try and justify all of my decisions.

The default type of `yytext` is `char[]`

> Whether `yytext` is `char[]` or `char *` is implementation defined behavior,
> so I'm required to define it here.

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


> A large part of the philosophy of Stone is that POSIX Lex sucks, but a lot of
> its problems can be fixed with the C preprocessor. For example, if the
> `yymore` function is defined as globally visibile, it could create a namespace
> collision in multi-file C programs. On the other hand, if `yymore` is declared
> as `static`, you might not be able to access it externally when you want to.
>
> On an extreme level, if you want to have two different tokenizers to parse two
> different types of data in the same program, you're basically guaranteed to
> have a namespace collision at link time.
>
> The obvious solution is to allow the programmer to choose how they want these
> functions defined with the C preprocessor.

The `%p`, `%n`, `%a`, and `%k` directives are ignored. The `%o` directive is
used to set `YYTEXT_SIZE`.

> I really hate these directives, they ought to be implementation defined
> extensions.

The extension function `yyreset()` will reset the lexer for further use with a
different file by throwing away any internal buffers and resetting all state.
The file pointer that was previously stored in `yyin` SHOULD NOT be reused.
`yyreset` contains this snippet:

```c
#if YYRESET1_DEFINED
	return yyreset1();
#else
	return 0;
#endif
```

> This feels like such an obvious feature that POSIX Lex just, like, doesn't
> have. It's intended that the programmer defines `yyreset1` and uses it to hook
> on lexer resets.

The extension function `yyerror()` will take as input an error code returned by
some other function and return a string providing an explanation of what error
occurred.

> For example, `yyerror(1)` returns the string "Input buffer is full". I didn't
> realize that this has a conflict with Yacc's `yyerror` function when I wrote
> it. I might change its name later.

Defining a visibility macro as anything other than `static` or the empty string
is undefined behavior.

> No `const int yyleng`, for example.

`BEGIN newstate;`, `ECHO;`, and `REJECT;`, are valid C statements.

> This allows you to do fancy stuff like this (nested multiline comments with
> some stored state):
>
> ```lex
> %x COMMENT
> 
>  static int level = 0;
>  static int yywrap(void);
> 
> %%
> 
> \/\\* level = 1; BEGIN COMMENT;
> 
> <COMMENT>\/\\* ++level;
> 
> <COMMENT>\\*\/ if (--level == 0) BEGIN INITIAL;
> 
> \\*\/ return 0;
> 
> . ECHO;
> 
> <COMMENT>. ;
> 
> %%
> 
> static int yywrap(void) {
> 	return 1;
> }
> 
> int main(void) {
> 	yyin = stdin;
> 	for (;;) {
> 		if (!yylex()) {
> 			return 0;
> 		}
> 	}
> }
> ```

## Moyo

Moyo is currently unimplemented, but it will eventually become a parser
generator which is hopefully compatible with POSIX yacc.

## Naming

Stone and Moyo are named after terms from the game of
[go](https://en.wikipedia.org/wiki/Go_(game)). Stone, the lexer, is named
after the pieces on the board because they form the building blocks of a game,
just like how tokens form the building blocks of a language. Moyo, the parser
generator, is named after the large frameworks that you sometimes build in go.
The pun is that parser generators work with shift-reduce tables, and in go when
your opponent builds a moyo you're supposed to either reduce or invade it.

## Standard references

* [POSIX regular
  expressions](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap09.html)
* [POSIX character
  classes](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html)
* [lex](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/lex.html)
* [yacc](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/yacc.html)
* [getopt](https://pubs.opengroup.org/onlinepubs/9799919799/functions/getopt.html)
* [Command argument syntax](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap12.html#tag_12_02)
