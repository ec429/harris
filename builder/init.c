#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "init.h"
#include "data.h"
#include "calc.h"
#include "../globals.h"

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

int load_builder(void)
{
	int rc;

	builder = calloc(1, sizeof(*builder));
	if (!builder) {
		error("Failed to allocate builder data", -ENOMEM);
		return 1;
	}
	INIT_LIST_HEAD(&builder->guns);
	INIT_LIST_HEAD(&builder->engines);
	INIT_LIST_HEAD(&builder->manfs);
	INIT_LIST_HEAD(&builder->techs);

	rc = load_guns(&builder->guns);
	if (rc < 0) {
		error("Failed to load guns", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d guns\n", rc);

	rc = load_engines(&builder->engines);
	if (rc < 0) {
		error("Failed to load engines", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d engines\n", rc);

	rc = load_manfs(&builder->manfs);
	if (rc < 0) {
		error("Failed to load manfs", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d manfs\n", rc);

	rc = load_techs(&builder->techs, &builder->engines, &builder->guns);
	if (rc < 0) {
		error("Failed to load techs", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d techs\n", rc);

	rc = populate_entities(&builder->entities, &builder->guns,
			       &builder->engines, &builder->manfs,
			       &builder->techs);
	if (rc < 0) {
		error("Failed to create entity arrays", rc);
		return 1;
	}

	empty_guns(&builder->guns);
	empty_engines(&builder->engines);
	empty_techs(&builder->techs);
	rc = apply_techs(&builder->entities, &builder->tn);
	if (rc < 0) {
		error("Failed to init techs", rc);
		return 1;
	}
	fprintf(stderr, "Initialised tech state\n");
	return 0;
}

void free_builder_data(void)
{
	free_techs(&builder->techs);
	free_guns(&builder->guns);
	free_engines(&builder->engines);
	free_manfs(&builder->manfs);
	free(builder);
	return;
}
