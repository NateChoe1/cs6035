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
