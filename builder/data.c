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
/* need to poke at 'event[]' */
#include "../globals.h"

const char *describe_bbg(enum bb_girth girth)
{
	switch (girth) {
	case BB_SMALL:
		return "small bombs";
	case BB_MEDIUM:
		return "medium bombs";
	case BB_COOKIE:
		return "large bombs";
	default:
		return "error!  unknown girth";
	}
}

const char *ident_bbg(enum bb_girth girth)
{
	switch (girth) {
	case BB_SMALL:
		return "SMA";
	case BB_MEDIUM:
		return "MED";
	case BB_COOKIE:
		return "COO";
	default:
		return "err";
	}
}

const char *describe_bbg_long(enum bb_girth girth)
{
	switch (girth) {
	case BB_SMALL:
		return "Sized to carry a multitude of small bombs internally.";
	case BB_MEDIUM:
		return "Can enclose small bombs, or carry cookies externally.";
	case BB_COOKIE:
		return "Fully-enclosed carriage of large 'cookie' bombs.";
	default:
		return "error!  unknown girth";
	}
}

const char *ident_ft(enum fuse_type ft)
{
	switch (ft) {
	case FT_NORMAL:
		return "NOR";
	case FT_SLENDER:
		return "SLE";
	case FT_SLABBY:
		return "SLA";
	case FT_GEODETIC:
		return "GEO";
	default:
		return "err";
	}
}

const char *describe_ft(enum fuse_type ft)
{
	switch (ft) {
	case FT_NORMAL:
		return "Normal fuselage design balances weight, drag and space.";
	case FT_SLENDER:
		return "A slender fuselage reduces drag, but is cramped and weak.";
	case FT_SLABBY:
		return "Slab-sided fuselages are draggy but cheap to build.";
	case FT_GEODETIC:
		return "Geodetic fuselage structure is strong and light.";
	default:
		return "error!  unknown fuselage type";
	}
}

const char *describe_esl(enum elec_level esl)
{
	switch (esl) {
	case ESL_LOW:
		return "low power";
	case ESL_HIGH:
		return "high power";
	case ESL_STABLE:
		return "high power, stable voltage";
	default:
		return "error!  unknown electric supply level";
	}
}

const char *ident_esl(enum elec_level esl)
{
	switch (esl) {
	case ESL_LOW:
		return "LO";
	case ESL_HIGH:
		return "HI";
	case ESL_STABLE:
		return "SV";
	default:
		return "er";
	}
}

const char *describe_esl_long(enum elec_level esl)
{
	switch (esl) {
	case ESL_LOW:
		return "Low power: basic instruments only.";
	case ESL_HIGH:
		return "High power: can use beams and beacons, supports GEE.";
	case ESL_STABLE:
		return "Stable voltage supply: supports advanced navaids.";
	default:
		return "error!  unknown electric supply level";
	}
}

const char *describe_navaid(enum nav_aid na)
{
	switch (na) {
	case NAV_GEE:
		return "GEE";
	case NAV_H2S:
		return "H2S";
	case NAV_OBOE:
		return "OBOE";
	case NAV_GH:
		return "GH";
	default:
		return "error!  unknown navaid";
	}
}

const char *describe_refit(enum refit_level refit)
{
	switch (refit) {
	case REFIT_FRESH:
		return "Clean-sheet";
	case REFIT_MARK:
		return "Mark";
	case REFIT_MOD:
		return "Mod";
	case REFIT_DOCTRINE:
		return "Doctrine";
	default:
		return "error!  unknown refit";
	}
}

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
	INT_KEY(gun, "UAB", uab);
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
	FILE *f = fopen("builder/dat/guns", "r");
	int rc;

	if (!f)
		return -errno;
	rc = for_each_line(f, load_gun, head);
	fclose(f);
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
	INT_KEY(eng, "HVY", hvy);
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
				if (ueng->u) {
					fprintf(stderr, "load_engine_word: u-engine '%s' already upgrades to '%s'\n", ueng->ident, ueng->u->ident);
					return -EEXIST;
				}
				ueng->u = eng;
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
	eng->hvy = 4;
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
	FILE *f = fopen("builder/dat/eng", "r");
	int rc;

	if (!f)
		return -errno;
	rc = for_each_line(f, load_engine, head);
	fclose(f);
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
	INT_KEY(man, "FAF", faf);
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
	if (!strcmp(key, "r")) {
		char *buf, *p;

		if (man->rand_names >= RN_MAX)
			return -ENOSPC;
		buf = man->rand_name[man->rand_names++];
		p = stpncpy(buf, value, RN_LEN - 1);
		*p = 0;
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
	FILE *f = fopen("builder/dat/manu", "r");
	struct manf_loader loader;
	int rc;

	if (!f)
		return -errno;
	loader.head = head;
	loader.starman = NULL;
	rc = for_each_line(f, load_manf, &loader);
	if (loader.starman) {
		free(loader.starman->eman);
		free(loader.starman->name);
		free(loader.starman->desc);
	}
	free(loader.starman);
	fclose(f);
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

struct tn_entry {
	char ident[4];
	unsigned int offset;
	const char *desc;
} tn_meta[] = {
#define	TNE(_id, _memb, _desc)						       \
	(struct tn_entry) {.ident = _id,				       \
			   .offset = offsetof(struct tech_numbers, _memb),     \
			   .desc = _desc }
	TNE("G4T", g4t, "Tare weight penalty for 4+ engines"),
	TNE("G4C", g4c, "Cost penalty for 4+ engines"),
	TNE("G4G", g4g, "4+ engines allowed with geodetic fuselage"),
	TNE("CMI", cmi, "Tare weight of per-crewman incidentals"),
	TNE("CES", ces, "Crewman effective skill scaling %"),
	TNE("CCC", ccc, "Crewman core cost scaling %"),
	TNE("CLT", clt, "Climb time"),
	TNE("FTN", ft[FT_NORMAL], "Tare weight of normal fuselage"),
	TNE("FTT", ft[FT_SLENDER], "Tare weight of slender fuselage"),
	TNE("FTS", ft[FT_SLABBY], "Tare weight of slab-sided fuselage"),
	TNE("FTG", ft[FT_GEODETIC], "Tare weight of geodetic fuselage"),
	TNE("FDN", fd[FT_NORMAL], "Drag of normal fuselage"),
	TNE("FDT", fd[FT_SLENDER], "Drag of slender fuselage"),
	TNE("FDS", fd[FT_SLABBY], "Drag of slab-sided fuselage"),
	TNE("FDG", fd[FT_GEODETIC], "Drag of geodetic fuselage"),
	TNE("FSN", fs[FT_NORMAL], "Serviceability of normal fuselage"),
	TNE("FST", fs[FT_SLENDER], "Serviceability of slender fuselage"),
	TNE("FSS", fs[FT_SLABBY], "Serviceability of slab-sided fuselage"),
	TNE("FSG", fs[FT_GEODETIC], "Serviceability of geodetic fuselage"),
	TNE("FFN", ff[FT_NORMAL], "Failure rate due to normal fuselage"),
	TNE("FFT", ff[FT_SLENDER], "Failure rate due to slender fuselage"),
	TNE("FFS", ff[FT_SLABBY], "Failure rate due to slab-sided fuselage"),
	TNE("FFG", ff[FT_GEODETIC], "Failure rate due to geodetic fuselage"),
	TNE("FVN", fv[FT_NORMAL], "Vulnerability of normal fuselage"),
	TNE("FVT", fv[FT_SLENDER], "Vulnerability of slender fuselage"),
	TNE("FVS", fv[FT_SLABBY], "Vulnerability of slab-sided fuselage"),
	TNE("FVG", fv[FT_GEODETIC], "Vulnerability of geodetic fuselage"),
	TNE("FWT", fwt, "Fuselage wing tare weight scaling %"),
	TNE("CCN", cc[FT_NORMAL], "Core cost of normal fuselage"),
	TNE("CCT", cc[FT_SLENDER], "Core cost of slender fuselage"),
	TNE("CCS", cc[FT_SLABBY], "Core cost of slab-sided fuselage"),
	TNE("CCG", cc[FT_GEODETIC], "Core cost of geodetic fuselage"),
	TNE("FCN", fc[FT_NORMAL], "Structure cost of normal fuselage"),
	TNE("FCT", fc[FT_SLENDER], "Structure cost of slender fuselage"),
	TNE("FCS", fc[FT_SLABBY], "Structure cost of slab-sided fuselage"),
	TNE("FCG", fc[FT_GEODETIC], "Structure cost of geodetic fuselage"),
	TNE("WTS", wts, "Wing tare weight span exponent %"),
	TNE("WTC", wtc, "Wing tare weight chord exponent %"),
	TNE("WTF", wtf, "Wing tare weight scaling factor %"),
	TNE("WLD", wld, "Wing lift/drag scaling factor %"),
	TNE("WCF", wcf, "Wing cost scaling factor %"),
	TNE("FUT", fut, "Fuel tanks tare weight scaling factor %"),
	TNE("FUV", fuv, "Fuel tanks vulnerability scaling factor %"),
	TNE("FGV", fgv, "Fuel tanks for geodetics vulnerability %"),
	TNE("SFT", sft, "Self-sealing tank tare weight scaling %"),
	TNE("SFV", sfv, "Self-sealing tank vulnerability scaling %"),
	TNE("SFC", sfc, "Self-sealing fuel tank cost scaling %"),
	TNE("FUC", fuc, "Fuel tank cost scaling factor %"),
	TNE("ETF", etf, "Engine mount tare weight scaling factor %"),
	TNE("EDF", edf, "Engine mount drag scaling factor %"),
	TNE("EES", ees, "Power Egg mount serviceability factor %"),
	TNE("EET", eet, "Power Egg mount tare weight factor %"),
	TNE("EEC", eec, "Power Egg mount cost scaling factor %"),
	TNE("EMC", emc, "Engine mount cost scaling factor %"),
	TNE("GTF", gtf, "Gun/turret tare weight scaling factor"),
	TNE("GDF", gdf, "Gun/turret drag scaling factor"),
	TNE("GCF", gcf, "Gun/turret cost scaling factor %"),
	TNE("GAC", gac, "Ammunition track cost scaling"),
	TNE("GAM", gam, "Ammunition tare weight per gun"),
	TNE("BTS", bt[BB_SMALL], "Tare weight of small-cell bomb bays"),
	TNE("BTM", bt[BB_MEDIUM], "Tare weight of medium-cell bomb bays"),
	TNE("BTC", bt[BB_COOKIE], "Tare weight of unobstructed bomb bays"),
	TNE("BMC", bmc, "Can medium-cell bomb bays carry cookies"),
	TNE("BBB", bbb, "Starting size for big bomb bay penalty"),
	TNE("BBF", bbf, "Inverse scaling of big bomb bay penalty"),
	TNE("ESL", esl, "Electrical supply level"),
	TNE("CSB", csb, "Course-Setting Bomb Sight available"),
	TNE("NAG", na[NAV_GEE], "Navaid 'GEE' available"),
	TNE("NAH", na[NAV_H2S], "Navaid 'H2S' available"),
	TNE("NAO", na[NAV_OBOE], "Navaid 'OBOE' available"),
	TNE("NAJ", na[NAV_GH], "Navaid 'Gee-H' available"),
	TNE("RGS", rgs, "Max take-off speed, grass runways, mph"),
	TNE("RGG", rgg, "Max gross take-off weight, grass, 000lb"),
	TNE("RCS", rcs, "Max take-off speed, concrete runways, mph"),
	TNE("RCG", rcg, "Max gross take-off weight, concrete, 000lb"),
	TNE("UBL", ubl, "Maximum engine count for unarmed bomber"),
#undef TNE
};

const char *ident_tn(unsigned int offset)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(tn_meta); i++)
		if (tn_meta[i].offset == offset)
			return tn_meta[i].ident;
	return "???";
}

const char *describe_tn(unsigned int offset)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(tn_meta); i++)
		if (tn_meta[i].offset == offset)
			return tn_meta[i].desc;
	return "error!  unknown tech_numbers entry";
}

enum refit_level rfl_tn(unsigned int offset)
{
	if (offset < offsetof(struct tech_numbers, mark_block))
		return REFIT_FRESH;
	if (offset < offsetof(struct tech_numbers, mod_block))
		return REFIT_MARK;
	if (offset < offsetof(struct tech_numbers, doctrine_block))
		return REFIT_MOD;
	return REFIT_DOCTRINE;
}

void doc_tech(struct tech_numbers *tn, const struct tech_numbers *dtn)
{
	memcpy((char *)tn->doctrine_block,
	       (char *)dtn->doctrine_block,
	       sizeof(*tn) - offsetof(struct tech_numbers, doctrine_block));
}

struct tech supporting = {
	.ident="sup",
	.name="Supporting Research",
	.desc="Allows one other slot to hold a cutting-edge tech.",
};
struct tech spec_four = {
	.ident="sp4",
	.name="Special req: 4+ Engines",
	.desc="Must have flown a prototype with 4 or more engines.",
};
struct tech spec_geo = {
	.ident="spG",
	.name="Special req: Geodetics",
	.desc="Must have flown a prototype with geodetic fuselage.",
};

int try_load_tn_word(const char *key, const char *value,
		     struct tech_numbers *tn)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(tn_meta); i++)
		if (!strcmp(key, tn_meta[i].ident)) {
			char *p = (char *)tn + tn_meta[i].offset;
			if (sscanf(value, "%u", (unsigned int *)p) != 1)
				return -EINVAL;
			if (!*(unsigned int *)p)
				*(unsigned int *)p = -1;
			return 0;
		}
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
		if (!strcmp(value, "sp4")) {
			loader->tech->req[i] = &spec_four;
			return 0;
		}
		if (!strcmp(value, "spG")) {
			loader->tech->req[i] = &spec_geo;
			return 0;
		}
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
		if (strlen(loader->tech->desc) >= 98)
			fprintf(stderr, "Warning: long desc for tech '%s'\n",
				loader->tech->ident);
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
	FILE *f = fopen("builder/dat/tech", "r");
	struct tech_loader loader;
	int rc;

	loader.head = head;
	loader.engines = engines;
	loader.guns = guns;

	if (!f)
		return -errno;
	rc = for_each_line(f, load_tech, &loader);
	fclose(f);
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

date tech_date(const struct tech *t)
{
	return (date){t->uy, t->um, 1};
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
			if (p[i] && p[i] != (unsigned int)-1)
				q[i] = p[i];
		for (i = 0; i < ARRAY_SIZE(tech->eng); i++)
			if (tech->eng[i])
				tech->eng[i]->unlocked = true;
		for (i = 0; i < ARRAY_SIZE(tech->gun); i++)
			if (tech->gun[i])
				tech->gun[i]->unlocked = true;
		/* Events triggered by techs */
		for(i=0;i<NNAVAIDS;i++)
			if((int)tech->num.na[i]>0)
				event[navevent[i]]=tech_date(tech);
		if(tech->num.na[NAV_GEE]==1)
		{
			date gj=tech_date(tech);
			gj.month+=6;
			gj.day+=20;
			if(gj.month>12)
			{
				gj.month-=12;
				gj.year++;
			}
			event[EVENT_GEEJAM]=gj;
		}
		if((int)tech->num.na[NAV_GEE]>1)
			event[EVENT_ALLGEE]=tech_date(tech);
	}
	return 0;
}
