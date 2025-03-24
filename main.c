#include <stdio.h>
#include <yex.h>
#include <string.h>

extern int yylex(struct yex_buffer *buff);
extern int yyparse(int (*yylex)(struct yex_buffer *), struct yex_buffer *);

int main() {
	struct yex_buffer buff;
	buff.input = "3*2 + 1*5";
	buff.parsed = 0;
	buff.length = (unsigned long) strlen(buff.input);
	printf("%d\n", yyparse(yylex, &buff));
	return 0;
}
