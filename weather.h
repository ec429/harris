#include <math.h>

typedef struct
{
	double p[128][128][3];
	double fx,fc,fdy;
}
w_state;

void w_init(w_state * buf, int prep);
void w_iter(w_state * ptr);
