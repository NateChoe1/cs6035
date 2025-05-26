#ifndef GETOPT_H
#define GETOPT_H

int getopt(int argc, char **argv, const char *optstring);
extern char *optarg;
extern int opterr, optind, optopt;

#endif
