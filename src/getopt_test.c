#include <string.h>

#include "getopt.h"

#include "main_test.h"

#include "getopt_test.h"

/* 0 on failure, 1 on success */
static int test_set(char *optstring, char **argv, char **expected,
		char *next);

void test_getopt(void) {
	char *argv[] = {
		"main",
		"-o",
		"option_val",
		"-al",
		"-a",
		"-l",
		"-oval",
		"arg",
		NULL,
	};
	char *expected[] = {
		"ooption_val",
		"a",
		"l",
		"a",
		"l",
		"oval",
		NULL,
	};
	assert(test_set("alo:", argv, expected, "arg"));
}

static int test_set(char *optstring, char **argv, char **expected,
		char *next) {
	int i, argc, ret;

	for (argc = 0; argv[argc]; ++argc) ;

	getopt_reset();
	for (i = 0; expected[i]; ++i) {
		int v = getopt(argc, argv, optstring);

		if (v != expected[i][0]) {
			assert(0);
			return 0;
		}

		if (expected[i][1] && strcmp(optarg, expected[i]+1) != 0) {
			assert(0);
			return 0;
		}
	}

	if (getopt(argc, argv, optstring) != -1) {
		assert(0);
		return 0;
	}

	ret = strcmp(argv[optind], next) == 0;
	assert(ret);
	return ret;
}
