#ifndef YEX_H
#define YEX_H

struct yex_buffer {
	char *input;
	unsigned long t_start;
	unsigned long parsed; /* should be initialized to 0 */
	unsigned long length;
};

#define YEX_EOF -1
#define YEX_ERROR -2

#endif
