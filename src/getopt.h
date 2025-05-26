#ifndef GETOPT_H
#define GETOPT_H

int getopt(int argc, char **argv, const char *optstring);
void getopt_reset(void);
extern char *optarg;
extern int opterr, optind, optopt;

#endif
