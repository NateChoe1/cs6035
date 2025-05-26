#ifndef MAIN_TEST_H
#define MAIN_TEST_H

extern void t_assert(int test, char *exp, char *file, int line);
#define assert(v) t_assert((v) != 0, #v, __FILE__, __LINE__)
#define LEN(arr) (sizeof(arr) / sizeof(*arr))

#endif
