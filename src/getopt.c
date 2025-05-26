#include <stdio.h>
#include <string.h>

#include "getopt.h"

char *optarg;
int opterr = 1;
int optind = 1;
int optopt = 0;

static int argidx = 1;

int getopt(int argc, char **argv, const char *optstring) {
	char rep_noarg, flag, match;
	int i, has_arg;

	if (argv[optind] == NULL ||
	    argv[optind][0] != '-' ||
	    strcmp(argv[optind], "-") == 0) {
		return -1;
	}
	if (strcmp(argv[optind], "--") == 0) {
		++optind;
		return -1;
	}

	if (optstring[0] == '+') {
		++optstring;
	}

	if (optstring[0] == ':') {
		++optstring;
		rep_noarg = ':';
	} else {
		rep_noarg = '?';
	}

	flag = argv[optind][argidx];

	for (i = 0; optstring[i]; ++i) {
		match = optstring[i];

		if (match == ':') {
			goto error;
		}

		if (optstring[i+1] == ':') {
			++i;
			has_arg = 1;
		} else {
			has_arg = 0;
		}

		if (match == flag) {
			break;
		}
	}

	if (!has_arg) {
		++argidx;
		if (argv[optind][argidx] == '\0') {
			++optind;
			argidx = 1;
		}
		return match;
	}

	/* handle things like -a'value'
	 *
	 * XXX: this is technically not posix compliant */
	if (argv[optind][argidx+1] != '\0') {
		optarg = argv[optind] + argidx + 1;
		++optind;
		argidx = 1;
		return match;
	}

	if (optind+1 >= argc) {
		return rep_noarg;
	}

	optarg = argv[optind+1];
	optind += 2;
	argidx = 1;
	return match;

error:
	return '?';
}
