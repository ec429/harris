#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include "data.h"
#include "parse.h"

#define INT_KEY(obj, kn, vn)						\
	if (!strcmp(key, kn)) {						\
		if (sscanf(value, "%u", &obj->vn) != 1)			\
			return -EINVAL;					\
		return 0;						\
	}

static int load_gun_word(const char *key, const char *value, void *data)
{
	struct turret *gun = data;

	INT_KEY(gun, "SRV", srv);
	INT_KEY(gun, "TWT", twt);
	INT_KEY(gun, "DRG", drg);
	INT_KEY(gun, "LXN", lxn);
	INT_KEY(gun, "GUN", gun);
	INT_KEY(gun, "GCF", gc[GC_FRONT]);
	INT_KEY(gun, "GCD", gc[GC_BEAM_HIGH]);
	INT_KEY(gun, "GCV", gc[GC_BEAM_LOW]);
	INT_KEY(gun, "GCH", gc[GC_TAIL_HIGH]);
	INT_KEY(gun, "GCL", gc[GC_TAIL_LOW]);
	INT_KEY(gun, "GCB", gc[GC_BENEATH]);
	INT_KEY(gun, "OCP", ocp);
	INT_KEY(gun, "OCN", ocn);
	INT_KEY(gun, "OCB", ocb);
	INT_KEY(gun, "SLB", slb);
	INT_KEY(gun, "ESL", esl);
	if (!strcmp(key, "n")) {
		gun->name = strdup(value);
		if (!gun->name)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "d")) {
		gun->desc = strdup(value);
		if (!gun->desc)
			return -ENOMEM;
		return 0;
	}

	fprintf(stderr, "load_gun_word: unrecognised key '%s'\n", key);
	return -EINVAL;
}

static int load_gun(const char *line, void *data)
{
	struct turret *gun = calloc(1, sizeof(*gun));
	struct list_head *head = data;
	int rc;

	if (strcspn(line, ":") != 4) {
		fprintf(stderr, "load_gun: ident is not 4 chars long\n");
		rc = -EINVAL;
		goto out;
	}
	memcpy(gun->ident, line, 4);
	gun->ident[4] = 0;
	rc = for_each_word(line + 5, load_gun_word, gun);
out:
	if (rc) {
		fprintf(stderr, "load_gun: failed to load %s\n", gun->ident);
		free(gun->name);
		free(gun->desc);
		free(gun);
	} else {
		list_add_tail(head, &gun->list);
	}
	return rc;
}

int load_guns(struct list_head *head)
{
	int fd = open("guns", O_RDONLY), rc;

	if (fd < 0)
		return -errno;
	rc = for_each_line(fd, load_gun, head);
	close(fd);
	return rc;
}

int free_guns(struct list_head *head)
{
	struct turret *gun;

	while (!list_empty(head)) {
		gun = list_first_entry(head, struct turret);
		free(gun->name);
		free(gun->desc);
		list_del(&gun->list);
		free(gun);
	}
	return 0;
}

struct engine_loader {
	struct engine *eng;
	struct list_head *engines;
};

static int load_engine_word(const char *key, const char *value, void *data)
{
	struct engine_loader *loader = data;
	struct engine *eng = loader->eng;

	INT_KEY(eng, "BHP", bhp);
	INT_KEY(eng, "VUL", vul);
	INT_KEY(eng, "FAI", fai);
	INT_KEY(eng, "SVC", svc);
	INT_KEY(eng, "COS", cos);
	INT_KEY(eng, "SCL", scl);
	INT_KEY(eng, "TWT", twt);
	INT_KEY(eng, "DRG", drg);
	if (!strcmp(key, "m")) {
		eng->manu = strdup(value);
		if (!eng->manu)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "n")) {
		eng->name = strdup(value);
		if (!eng->name)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "d")) {
		eng->desc = strdup(value);
		if (!eng->desc)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "u")) {
		struct engine *ueng;

		list_for_each_entry(ueng, loader->engines) {
			if (!strcmp(value, ueng->ident)) {
				eng->u = ueng;
				return 0;
			}
		}
		fprintf(stderr, "load_engine_word: No such u-engine '%s'\n", value);
		return -EINVAL;
	}

	fprintf(stderr, "load_engine_word: unrecognised key '%s'\n", key);
	return -EINVAL;
}

static int load_engine(const char *line, void *data)
{
	struct engine *eng = calloc(1, sizeof(*eng));
	struct list_head *head = data;
	struct engine_loader loader;
	int rc;

	if (strcspn(line, ":") != 4) {
		fprintf(stderr, "load_engine: ident is not 4 chars long\n");
		rc = -EINVAL;
		goto out;
	}
	memcpy(eng->ident, line, 4);
	eng->ident[4] = 0;
	loader.eng = eng;
	loader.engines = head;
	rc = for_each_word(line + 5, load_engine_word, &loader);
out:
	if (rc) {
		fprintf(stderr, "load_engine: failed to load %s\n", eng->ident);
		free(eng->manu);
		free(eng->name);
		free(eng->desc);
		free(eng);
	} else {
		list_add_tail(head, &eng->list);
	}
	return rc;
}

int load_engines(struct list_head *head)
{
	int fd = open("eng", O_RDONLY), rc;

	if (fd < 0)
		return -errno;
	rc = for_each_line(fd, load_engine, head);
	close(fd);
	return rc;
}

int free_engines(struct list_head *head)
{
	struct engine *eng;

	while (!list_empty(head)) {
		eng = list_first_entry(head, struct engine);
		free(eng->manu);
		free(eng->name);
		free(eng->desc);
		list_del(&eng->list);
		free(eng);
	}
	return 0;
}

static int load_manf_word(const char *key, const char *value, void *data)
{
	struct manf *man = data;

	INT_KEY(man, "WAP", wap);
	INT_KEY(man, "WLD", wld);
	INT_KEY(man, "BTS", bt[BB_SMALL]);
	INT_KEY(man, "BTM", bt[BB_MEDIUM]);
	INT_KEY(man, "BTC", bt[BB_COOKIE]);
	INT_KEY(man, "BBB", bbb);
	INT_KEY(man, "WCF", wcf);
	INT_KEY(man, "WCP", wcp);
	INT_KEY(man, "WC4", wc4);
	INT_KEY(man, "WT4", wt4);
	INT_KEY(man, "ACC", acc);
	INT_KEY(man, "ACT", act);
	INT_KEY(man, "GEO", geo);
	INT_KEY(man, "TPL", tpl);
	INT_KEY(man, "FDN", fd[FT_NORMAL]);
	INT_KEY(man, "FDT", fd[FT_SLENDER]);
	INT_KEY(man, "FDS", fd[FT_SLABBY]);
	INT_KEY(man, "FDG", fd[FT_GEODETIC]);
	INT_KEY(man, "FTN", ft[FT_NORMAL]);
	INT_KEY(man, "FTT", ft[FT_SLENDER]);
	INT_KEY(man, "FTS", ft[FT_SLABBY]);
	INT_KEY(man, "FTG", ft[FT_GEODETIC]);
	INT_KEY(man, "SVP", svp);
	INT_KEY(man, "BOF", bof);
	if (!strcmp(key, "e")) {
		man->eman = strdup(value);
		if (!man->eman)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "n")) {
		man->name = strdup(value);
		if (!man->name)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "d")) {
		man->desc = strdup(value);
		if (!man->desc)
			return -ENOMEM;
		return 0;
	}

	fprintf(stderr, "load_manf_word: unrecognised key '%s'\n", key);
	return -EINVAL;
}

struct manf_loader {
	struct list_head *head;
	struct manf *starman;
};

static int load_manf(const char *line, void *data)
{
	struct manf *man = calloc(1, sizeof(*man));
	struct manf_loader *loader = data;
	bool star;
	int rc;

	if (strcspn(line, ":") != 2) {
		fprintf(stderr, "load_manf: ident is not 2 chars long\n");
		rc = -EINVAL;
		goto out;
	}
	star = line[0] == '*' && line[1] == '*';
	if (!star) {
		if (!loader->starman) {
			fprintf(stderr, "load_manf: entry ** must be first\n");
			rc = -EINVAL;
			goto out;
		}
		// copy all the stars
		*man = *loader->starman;
		// except pointery things
		INIT_LIST_HEAD(&man->list);
		man->eman = NULL;
		man->name = NULL;
		man->desc = NULL;
	}
	memcpy(man->ident, line, 2);
	man->ident[2] = 0;
	rc = for_each_word(line + 3, load_manf_word, man);
out:
	if (rc) {
		fprintf(stderr, "load_manf: failed to load %s\n", man->ident);
		free(man->eman);
		free(man->name);
		free(man->desc);
		free(man);
	} else if (star) {
		loader->starman = man;
	} else {
		list_add_tail(loader->head, &man->list);
	}
	return rc;
}

int load_manfs(struct list_head *head)
{
	int fd = open("manu", O_RDONLY), rc;
	struct manf_loader loader;

	if (fd < 0)
		return -errno;
	loader.head = head;
	loader.starman = NULL;
	rc = for_each_line(fd, load_manf, &loader);
	if (loader.starman) {
		free(loader.starman->eman);
		free(loader.starman->name);
		free(loader.starman->desc);
	}
	free(loader.starman);
	close(fd);
	if (rc > 0)
		rc--; /* starman doesn't count */
	return rc;
}

int free_manfs(struct list_head *head)
{
	struct manf *man;

	while (!list_empty(head)) {
		man = list_first_entry(head, struct manf);
		free(man->eman);
		free(man->name);
		free(man->desc);
		list_del(&man->list);
		free(man);
	}
	return 0;
}

struct tech_loader {
	struct list_head *head;
	struct tech *tech;
	struct list_head *engines;
	struct list_head *guns;
};

int try_load_tn_word(const char *key, const char *value,
		     struct tech_numbers *tn)
{
	INT_KEY(tn, "G4T", g4t);
	INT_KEY(tn, "G4C", g4c);
	INT_KEY(tn, "CMI", cmi);
	INT_KEY(tn, "CES", ces);
	INT_KEY(tn, "CCC", ccc);
	INT_KEY(tn, "CLT", clt);
	INT_KEY(tn, "FTN", ft[FT_NORMAL]);
	INT_KEY(tn, "FTT", ft[FT_SLENDER]);
	INT_KEY(tn, "FTS", ft[FT_SLABBY]);
	INT_KEY(tn, "FTG", ft[FT_GEODETIC]);
	INT_KEY(tn, "FDN", fd[FT_NORMAL]);
	INT_KEY(tn, "FDT", fd[FT_SLENDER]);
	INT_KEY(tn, "FDS", fd[FT_SLABBY]);
	INT_KEY(tn, "FDG", fd[FT_GEODETIC]);
	INT_KEY(tn, "FSN", fs[FT_NORMAL]);
	INT_KEY(tn, "FST", fs[FT_SLENDER]);
	INT_KEY(tn, "FSS", fs[FT_SLABBY]);
	INT_KEY(tn, "FSG", fs[FT_GEODETIC]);
	INT_KEY(tn, "FFN", ff[FT_NORMAL]);
	INT_KEY(tn, "FFT", ff[FT_SLENDER]);
	INT_KEY(tn, "FFS", ff[FT_SLABBY]);
	INT_KEY(tn, "FFG", ff[FT_GEODETIC]);
	INT_KEY(tn, "FVN", fv[FT_NORMAL]);
	INT_KEY(tn, "FVT", fv[FT_SLENDER]);
	INT_KEY(tn, "FVS", fv[FT_SLABBY]);
	INT_KEY(tn, "FVG", fv[FT_GEODETIC]);
	INT_KEY(tn, "FWT", fwt);
	INT_KEY(tn, "CCN", cc[FT_NORMAL]);
	INT_KEY(tn, "CCT", cc[FT_SLENDER]);
	INT_KEY(tn, "CCS", cc[FT_SLABBY]);
	INT_KEY(tn, "CCG", cc[FT_GEODETIC]);
	INT_KEY(tn, "FCN", fc[FT_NORMAL]);
	INT_KEY(tn, "FCT", fc[FT_SLENDER]);
	INT_KEY(tn, "FCS", fc[FT_SLABBY]);
	INT_KEY(tn, "FCG", fc[FT_GEODETIC]);
	INT_KEY(tn, "WTS", wts);
	INT_KEY(tn, "WTC", wtc);
	INT_KEY(tn, "WTF", wtf);
	INT_KEY(tn, "WLD", wld);
	INT_KEY(tn, "WCF", wcf);
	INT_KEY(tn, "FUT", fut);
	INT_KEY(tn, "FUV", fuv);
	INT_KEY(tn, "FGV", fgv);
	INT_KEY(tn, "SFT", sft);
	INT_KEY(tn, "SFV", sfv);
	INT_KEY(tn, "SFC", sfc);
	INT_KEY(tn, "FUC", fuc);
	INT_KEY(tn, "ETF", etf);
	INT_KEY(tn, "EDF", edf);
	INT_KEY(tn, "EES", ees);
	INT_KEY(tn, "EET", eet);
	INT_KEY(tn, "EEC", eec);
	INT_KEY(tn, "EMC", emc);
	INT_KEY(tn, "GTF", gtf);
	INT_KEY(tn, "GDF", gdf);
	INT_KEY(tn, "GCF", gcf);
	INT_KEY(tn, "GAC", gac);
	INT_KEY(tn, "GAM", gam);
	INT_KEY(tn, "BTS", bt[BB_SMALL]);
	INT_KEY(tn, "BTM", bt[BB_MEDIUM]);
	INT_KEY(tn, "BTC", bt[BB_COOKIE]);
	INT_KEY(tn, "BMC", bmc);
	INT_KEY(tn, "BBB", bbb);
	INT_KEY(tn, "BBF", bbf);
	INT_KEY(tn, "ESL", esl);
	INT_KEY(tn, "CSB", csb);
	INT_KEY(tn, "NAG", na[NA_GEE]);
	INT_KEY(tn, "NAH", na[NA_H2S]);
	INT_KEY(tn, "NAO", na[NA_OBOE]);
	INT_KEY(tn, "RGS", rgs);
	INT_KEY(tn, "RGG", rgg);
	INT_KEY(tn, "RCS", rcs);
	INT_KEY(tn, "RCG", rcg);
	INT_KEY(tn, "UBL", ubl);
	return -EINVAL;
}

static int load_tech_word(const char *key, const char *value, void *data)
{
	struct tech_loader *loader = data;

	INT_KEY(loader->tech, "y", year);
	INT_KEY(loader->tech, "m", month);
	if (!try_load_tn_word(key, value, &loader->tech->num))
		return 0;
	if (!strcmp(key, "e")) {
		struct engine *eng;
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(loader->tech->eng); i++)
			if (!loader->tech->eng[i])
				break;
		if (i == ARRAY_SIZE(loader->tech->eng))
			return -ENOBUFS;
		list_for_each_entry(eng, loader->engines) {
			if (!strcmp(value, eng->ident)) {
				loader->tech->eng[i] = eng;
				return 0;
			}
		}
		fprintf(stderr, "load_tech_word: No such engine '%s'\n", value);
		return -ENOENT;
	}
	if (!strcmp(key, "t")) {
		struct turret *gun;
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(loader->tech->gun); i++)
			if (!loader->tech->gun[i])
				break;
		if (i == ARRAY_SIZE(loader->tech->gun))
			return -ENOBUFS;
		list_for_each_entry(gun, loader->guns) {
			if (!strcmp(value, gun->ident)) {
				loader->tech->gun[i] = gun;
				return 0;
			}
		}
		fprintf(stderr, "load_tech_word: No such turret '%s'\n", value);
		return -ENOENT;
	}
	if (!strcmp(key, "r")) {
		struct tech *req;
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(loader->tech->req); i++)
			if (!loader->tech->req[i])
				break;
		if (i == ARRAY_SIZE(loader->tech->req))
			return -ENOBUFS;
		list_for_each_entry(req, loader->head) {
			if (!strcmp(value, req->ident)) {
				loader->tech->req[i] = req;
				return 0;
			}
		}
		fprintf(stderr, "load_tech_word: No such tech '%s'\n", value);
		return -ENOENT;
	}
	if (!strcmp(key, "n")) {
		loader->tech->name = strdup(value);
		if (!loader->tech->name)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "d")) {
		loader->tech->desc = strdup(value);
		if (!loader->tech->desc)
			return -ENOMEM;
		return 0;
	}

	fprintf(stderr, "load_tech_word: unrecognised key '%s'\n", key);
	return -EINVAL;
}

static int load_tech(const char *line, void *data)
{
	struct tech *tech = calloc(1, sizeof(*tech));
	struct tech_loader *loader = data;
	int rc;

	if (strcspn(line, ":") != 3) {
		fprintf(stderr, "load_tech: ident is not 3 chars long\n");
		rc = -EINVAL;
		goto out;
	}
	memcpy(tech->ident, line, 3);
	tech->ident[3] = 0;
	loader->tech = tech;
	rc = for_each_word(line + 4, load_tech_word, loader);
out:
	if (rc) {
		fprintf(stderr, "load_tech: failed to load %s\n", tech->ident);
		free(tech->name);
		free(tech->desc);
		free(tech);
	} else {
		list_add_tail(loader->head, &tech->list);
	}
	return rc;
}

int load_techs(struct list_head *head, struct list_head *engines,
	       struct list_head *guns)
{
	int fd = open("tech", O_RDONLY), rc;
	struct tech_loader loader;

	loader.head = head;
	loader.engines = engines;
	loader.guns = guns;

	if (fd < 0)
		return -errno;
	rc = for_each_line(fd, load_tech, &loader);
	close(fd);
	return rc;
}

int free_techs(struct list_head *head)
{
	struct tech *tech;

	while (!list_empty(head)) {
		tech = list_first_entry(head, struct tech);
		free(tech->name);
		free(tech->desc);
		list_del(&tech->list);
		free(tech);
	}
	return 0;
}

/* While lists were handy for loading the data, in the editor we'd rather
 * have an array that we can quickly index into.
 */
int populate_entities(struct entities *ent, struct list_head *guns,
		      struct list_head *engines, struct list_head *manfs,
		      struct list_head *techs)
{
	struct turret *gun;
	struct engine *eng;
	struct manf *manf;
	struct tech *tech;
	unsigned int i;

	memset(ent, 0, sizeof(*ent));
	/* Count the entities, to size the arrays */
	list_for_each_entry(gun, guns)
		ent->ngun++;
	list_for_each_entry(eng, engines)
		ent->neng++;
	list_for_each_entry(manf, manfs)
		ent->nmanf++;
	list_for_each_entry(tech, techs)
		ent->ntech++;
	/* Allocate the arrays of pointers */
	ent->gun = calloc(ent->ngun, sizeof(gun));
	if (!ent->gun)
		return -errno;
	ent->eng = calloc(ent->neng, sizeof(eng));
	if (!ent->eng)
		return -errno;
	ent->manf = calloc(ent->nmanf, sizeof(manf));
	if (!ent->manf)
		return -errno;
	ent->tech = calloc(ent->ntech, sizeof(tech));
	if (!ent->tech)
		return -errno;
	/* Fill in the arrays */
	i = 0;
	list_for_each_entry(gun, guns)
		ent->gun[i++] = gun;
	i = 0;
	list_for_each_entry(eng, engines)
		ent->eng[i++] = eng;
	i = 0;
	list_for_each_entry(manf, manfs)
		ent->manf[i++] = manf;
	i = 0;
	list_for_each_entry(tech, techs)
		ent->tech[i++] = tech;
	return 0;
}

int apply_techs(const struct entities *ent, struct tech_numbers *tn)
{
	struct tech *tech;
	unsigned int j;

	for (j = 0; j < ent->neng; j++)
		ent->eng[j]->unlocked = false;
	for (j = 0; j < ent->ngun; j++)
		ent->gun[j]->unlocked = false;
	for (j = 0; j < ent->ntech; j++) {
		unsigned int i;

		tech = ent->tech[j];
		tech->have_reqs = true;
		for (i = 0; i < ARRAY_SIZE(tech->req); i++)
			if (tech->req[i] && !tech->req[i]->unlocked)
				tech->have_reqs = false;
	}
	memset(tn, 0, sizeof(*tn));
	for (j = 0; j < ent->ntech; j++) {
		unsigned int *p, *q, i;

		tech = ent->tech[j];
		if (!tech->unlocked)
			continue;
		p = (unsigned int *)&tech->num;
		q = (unsigned int *)tn;
		for (i = 0; i * sizeof(*p) < sizeof(*tn); i++)
			if (p[i])
				q[i] = p[i];
		for (i = 0; i < ARRAY_SIZE(tech->eng); i++)
			if (tech->eng[i])
				tech->eng[i]->unlocked = true;
		for (i = 0; i < ARRAY_SIZE(tech->gun); i++)
			if (tech->gun[i])
				tech->gun[i]->unlocked = true;
	}
	return 0;
}
