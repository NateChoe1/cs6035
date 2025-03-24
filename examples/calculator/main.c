#include <stdio.h>
#include <yex.h>
#include <string.h>

extern int yylex(struct yex_buffer *buff);
extern int yyparse(int (*yylex)(struct yex_buffer *), struct yex_buffer *);

int main(int argc, char *argv[]) {
	struct yex_buffer buff;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [expression]\n", argv[0]);
		return 1;
	}

	buff.input = argv[1];
	buff.parsed = 0;
	buff.length = (unsigned long) strlen(buff.input);
	printf("%d\n", yyparse(yylex, &buff));
	return 0;
}
