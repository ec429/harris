#pragma once
#include <stdbool.h>

void linedist(int ldx, int ldy, int pdx, int pdy, double *d, double *lambda); // min. distance from the line segment (0,0)-(ldx,ldy) to the point (pdx, pdy), as well as value of lambda
bool xyr(double x, double y, double r); // is |(x,y)|<r
