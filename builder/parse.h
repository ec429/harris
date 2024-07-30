#ifndef _PARSE_H
#define _PARSE_H
#include <stdio.h>

int for_each_line(FILE *f, int (*cb)(const char *line, void *data), void *data);
int for_each_word(const char *line,
		  int (*cb)(const char *key, const char *value, void *data),
		  void *data);

#endif // _PARSE_H
