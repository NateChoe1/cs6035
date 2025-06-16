#include <stdio.h>
#include <string.h>

#include "moyo.h"
#include "stone.h"

static char *basename(char *path);

int main(int argc, char **argv) {
	char *prog;

	if (argc < 1) {
		fputs("Your system doesn't provide command line arguments\n",
				stderr);
		return 1;
	}

	prog = basename(argv[0]);
	if (strcmp(prog, "stone") == 0 || strcmp(prog, "lex") == 0) {
		return stone_main(argc, argv);
	}
	if (strcmp(prog, "moyo") == 0 || strcmp(prog, "yacc") == 0) {
		return moyo_main(argc, argv);
	}
	fprintf(stderr, "Unknown program name %s\n", prog);
	return 1;
}

static char *basename(char *path) {
	char *ret;
	ret = path;
	while (*path) {
		if (*path == '/') {
			ret = path+1;
		}
		++path;
	}
	return ret;
}
