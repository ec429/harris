/*
	harris - a strategy game
	Copyright (C) 2012-2024 Edward Cree

	licensed under GPLv2 - see top of harris.c for details

	research: screen for research priorities
*/

#include <atg.h>
#include "date.h"

extern atg_element *research_box;

bool tech_future(date now, const struct tech *tech);
