#pragma once
#include <stdbool.h>

double linedist(int ldx, int ldy, int pdx, int pdy); // min. distance from the line segment (0,0)-(ldx,ldy) to the point (pdx, pdy)
bool xyr(double x, double y, double r); // is |(x,y)|<r
