#include <stdio.h>

static void t_assert(int test, char *exp, char *file, int line);
static void summarize();
#define assert(v) t_assert((v) != 0, #v, __FILE__, __LINE__)

int main() {
	assert(0*0);
	assert(1+1);
	summarize();
	return 0;
}

static int passed = 0, total = 0;

static void t_assert(int test, char *exp, char *file, int line) {
	++total;
	if (test) {
		++passed;
		return;
	}
	fprintf(stderr, "[FAIL]\tAssertion failed: %s:%d  %s\n",
			file, line, exp);
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
