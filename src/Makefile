HEADERS=$(wildcard *.h)
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
CFLAGS=-Wall -Wextra -Wpedantic -Werror -g -ansi

cc: $(OBJ)
	$(CC) $(OBJ) -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
