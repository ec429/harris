#include <stdbool.h>

typedef struct
{
	double push, slant;
	double p[256][128];
	double t[256][128];
}
w_state;

void w_init(w_state * buf, unsigned int prep, bool lorw[128][128]);
void w_iter(w_state * ptr, bool lorw[128][128]);
