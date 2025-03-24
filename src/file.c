#include "file.h"

char *file_readline(FILE *input, struct arena *arena) {
	char *ret;
	size_t len, alloc;
	int c;

	c = fgetc(input);
	if (c == EOF) {
		return NULL;
	}
	ungetc(c, input);

	len = 0;
	alloc = 80;
	ret = arena_malloc(arena, alloc);
	for (;;) {
		if (len >= alloc) {
			alloc *= 2;
			ret = arena_realloc(ret, alloc);
		}

		c = fgetc(input);
		if (c != '\n' && c != EOF) {
			ret[len++] = c;
			continue;
		}
		ret[len] = '\0';
		return ret;
	}
}

int file_copysection(FILE *input, FILE *output) {
	int c;

	for (;;) {
		c = fgetc(input);
		if (c == EOF) {
			return 1;
		}
		if (c != '%') {
			goto norm;
		}
		c = fgetc(input);
		if (c != '%') {
			ungetc(c, input);
			goto norm;
		}
		break;
norm:
		fputc(c, output);
	}
	c = fgetc(input);
	if (c != '\n') {
		ungetc(c, input);
	}
	return 0;
}
