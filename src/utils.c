#include <stdio.h>
#include <stdarg.h>

#include "utils.h"

int startswith(char *string, char *prefix) {
	long i;

	for (i = 0; prefix[i]; ++i) {
		if (string[i] != prefix[i]) {
			return 0;
		}
	}
	return 1;
}

void report(char *level, char *message, ...) {
	va_list ap;

	va_start(ap, message);
	fputs(level, stderr);
	vfprintf(stderr, message, ap);
}
