#include <stdio.h>
#include <string.h>

#include "yex.h"
#include "lacc.h"

static char *basename(char *path);

int main(int argc, char **argv) {
	char *prog;

	if (argc < 1) {
		fputs("Your system doesn't provide command line arguments\n",
				stderr);
		return 1;
	}

	prog = basename(argv[0]);
	if (strcmp(prog, "yex") == 0) {
		return yex_main(argc, argv);
	}
	if (strcmp(prog, "lacc") == 0) {
		return lacc_main(argc, argv);
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
