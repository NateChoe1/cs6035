#include <stdio.h>

#include "arena.h"

#include "main_test.h"

#include "lr_test.h"
#include "regex_test.h"

static void summarize(void);

int main() {
	test_regex();
	test_lr();

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
	fprintf(stderr, "[FAIL]\tAssertion failed: %s (%s:%d)\n",
			exp, file, line);
}

static void summarize() {
	fprintf(stderr, "[INFO]\tFinal results: %d/%d tests passed\n",
			passed, total);
	if (passed < total) {
		fputs("[FAIL]\tSome tests failed\n", stderr);
	} else {
		fputs("[SUCCESS]\tCongratulations, you don't suck at coding\n",
				stderr);
	}
	fputs("[INFO]\tRemember to run this test with Valgrind\n", stderr);
}
