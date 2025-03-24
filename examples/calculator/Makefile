calculator: lacc.c yex.c main.c atoin.h
	$(CC) -ansi -Wall -Wextra -Wpedantic -Werror lacc.c yex.c main.c -I../../dist -o $@

lacc.c: input.lacc ../../dist/lacc
	../../dist/lacc $< $@

yex.c: input.yex ../../dist/yex
	../../dist/yex $< $@
