#ifndef _ART_H
#define _ART_H

#include <atg.h>
#include "calc.h"
#include "data.h"

int init_camos(struct builder_data *builder);
void bomber_art_mini(SDL_Surface *s, const struct bomber *b);

#endif /* _ART_H */
