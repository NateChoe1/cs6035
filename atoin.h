#ifndef ATOIN_H
#define ATOIN_H

#include <ctype.h>

static int atoin(char *str, long n) {
	int i, ret;
	ret = 0;
	for (i = 0; isdigit(str[i]) && i < n; ++i) {
		ret *= 10;
		ret += str[i] - '0';
	}
	return ret;
}

#endif
