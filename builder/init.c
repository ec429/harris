#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data.h"
#include "calc.h"

static void error(const char *msg, int rc)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(-rc));
}

static void empty_guns(struct list_head *guns)
{
	struct turret *gun;

	list_for_each_entry(gun, guns)
		gun->unlocked = false;
}

static void empty_engines(struct list_head *engines)
{
	struct engine *eng;

	list_for_each_entry(eng, engines)
		eng->unlocked = false;
}

static void empty_techs(struct list_head *techs)
{
	struct tech *tech;

	list_for_each_entry(tech, techs)
		tech->unlocked = !tech->year;
}

/* This isn't used by anything yet, and loading into locals obviously
 * doesn't make sense; when we integrate a bit further the entities,
 * tech_numbers etc. will live in Harris globals and we'll split the
 * freeing at the end of this function into a separate function.
 */
int load_builder(void)
{
	struct list_head guns, engines, manfs, techs;
	struct entities entities;
	struct tech_numbers tn;
	int rc;

	INIT_LIST_HEAD(&guns);
	INIT_LIST_HEAD(&engines);
	INIT_LIST_HEAD(&manfs);
	INIT_LIST_HEAD(&techs);

	rc = load_guns(&guns);
	if (rc < 0) {
		error("Failed to load guns", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d guns\n", rc);

	rc = load_engines(&engines);
	if (rc < 0) {
		error("Failed to load engines", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d engines\n", rc);

	rc = load_manfs(&manfs);
	if (rc < 0) {
		error("Failed to load manfs", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d manfs\n", rc);

	rc = load_techs(&techs, &engines, &guns);
	if (rc < 0) {
		error("Failed to load techs", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d techs\n", rc);

	rc = populate_entities(&entities, &guns, &engines, &manfs, &techs);
	if (rc < 0) {
		error("Failed to create entity arrays", rc);
		return 1;
	}

	empty_guns(&guns);
	empty_engines(&engines);
	empty_techs(&techs);
	rc = apply_techs(&entities, &tn);
	if (rc < 0) {
		error("Failed to init techs", rc);
		return 1;
	}
	fprintf(stderr, "Initialised tech state\n");

	fprintf(stderr, "Cleaning up...\n");
	free_techs(&techs);
	free_guns(&guns);
	free_engines(&engines);
	free_manfs(&manfs);
	return 0;
}
