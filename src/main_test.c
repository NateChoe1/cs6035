#include <stdio.h>
#include <stdarg.h>

#include "arena.h"

#include "main_test.h"

#include "lr_test.h"
#include "regex_test.h"
#include "getopt_test.h"

#define FAIL "[\x1b[41;30;1mFAIL\x1b[0m]\t"
#define PASS "[\x1b[32;1mPASS\x1b[0m]\t"
#define INFO "[\x1b[33;1mINFO\x1b[0m]\t"

static void summarize(void);
static void report(char *level, char *template, ...);

int main() {
	test_lr();
	test_regex();
	test_getopt();

	summarize();
	return 0;
}

static int passed = 0, total = 0;

void t_assert(int test, char *exp, char *file, int line) {
	++total;
	if (test) {
		++passed;
		return;
	}
	report(FAIL, "Assertion failed: %s (%s:%d)", exp, file, line);
}

static void summarize() {
	report(INFO, "Final results: %d/%d tests passed", passed, total);
	if (passed < total) {
		report(FAIL, "Some tests failed");
	} else {
		report(PASS, "Congratulations, you don't suck at coding");
	}
	report(INFO, "Remember to run this test with Valgrind");
}

static void report(char *level, char *template, ...) {
	va_list ap;

	fputs(level, stderr);
	va_start(ap, template);
	vfprintf(stderr, template, ap);
	fputc('\n', stderr);
}
