#include <string.h>

#include "getopt.h"

#include "main_test.h"

#include "getopt_test.h"

static void test_case_1(void);
static void test_case_2(void);

static void test_set(char *optstring, char **argv, char **expected,
		char *next);

void test_getopt(void) {
	test_case_1();
	test_case_2();
}

static void test_case_1(void) {
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
	test_set("alo:", argv, expected, "arg");
}

static void test_case_2(void) {
	char *argv[] = {
		"main",
		"-a",
		NULL,
	};
	char *expected[] = {
		"?",
		NULL,
	};
	test_set("h", argv, expected, NULL);
}

static void test_set(char *optstring, char **argv, char **expected,
		char *next) {
	int i, argc, ret;

	for (argc = 0; argv[argc]; ++argc) ;

	getopt_reset();
	for (i = 0; expected[i]; ++i) {
		int v = getopt(argc, argv, optstring);

		if (v != expected[i][0]) {
			assert(0);
			return;
		}

		if (expected[i][1] && strcmp(optarg, expected[i]+1) != 0) {
			assert(0);
			return;
		}
	}

	if (expected[i-1][0] != '?' &&
			getopt(argc, argv, optstring) != -1) {
		assert(0);
		return;
	}

	ret = (next != NULL) ? (strcmp(argv[optind], next) == 0) : 1;
	assert(ret);
	return;
}
