#ifndef _SAVE_H
#define _SAVE_H

#include <stdio.h>
#include "calc.h"

int save_design(FILE *f, const struct bomber *b);
int load_design(FILE *f, struct bomber *b, const struct entities *ent);

#endif // _SAVE_H
