#ifndef FILE_H
#define FILE_H

#include <stdio.h>

#include "arena.h"

char *file_readline(FILE *input, struct arena *arena);

/* copies until the "%%" sequence in `input` */
int file_copysection(FILE *input, FILE *output);

#endif
