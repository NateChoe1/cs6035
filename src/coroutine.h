#ifndef COROUTINE_H
#define COROUTINE_H

/* https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 *
 * COROUTINE_GETC gets a character and puts it in `c`
 * COROUTINE_GETL reads a line to state->line, and puts the line length into
 * state->line_len
 * COROUTINE_UNGETC turns the next GET_CHAR into a no-op
 *
 * before parsing, you should call each function with c = RESET_STATE
 *
 * a return value of 0 indicates that execution hasn't finished
 * a return value of 1 indicates that there was an error
 *
 * negative return values are used for useful information
 *
 * note that this code assumes c is always positive except on controls like
 * COROUTINE_EOF and COROUTINE_RESET
 * */

#define COROUTINE_EOF -1
#define COROUTINE_RESET -2

#define COROUTINE_START(progress) \
	int *p = &progress; \
	int u = 0; \
	if (c == COROUTINE_RESET) { \
		*p = 0; \
	} \
	switch(*p) { \
	case -1: \
		return 1; \
	case 0: \
	((void) u)
#define COROUTINE_END } do { return 1; } while (0)
#define COROUTINE_GETC do { \
	if (!u) { \
		*p = __LINE__; \
		return 0; \
		case __LINE__: \
		(void) 0; \
	} \
	u = 0; \
} while (0)
#define COROUTINE_UNGETC u = 1
#define COROUTINE_RET(value) do { *p = -1; return value; } while (0)

#define COROUTINE_GETL do { \
	state->line_len = 0; \
	for (;;) { \
		if (state->line_len > sizeof(state->line)) { \
			COROUTINE_RET(1); \
		} \
		COROUTINE_GETC; \
		if (c == '\n') { \
			state->eof = 0; \
			state->line[state->line_len] = '\0'; \
			break; \
		} \
		if (c == COROUTINE_EOF) { \
			state->eof = 1; \
			state->line[state->line_len] = '\0'; \
			break; \
		} \
		state->line[state->line_len++] = c; \
	} \
} while (0)

#endif
