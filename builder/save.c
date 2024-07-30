#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "save.h"
#include "parse.h"

char crew_to_letter(enum cclass c)
{
	if (c >= 0 && c < CREW_CLASSES)
		return cclasses[c].letter;
	return '?';
}

static void save_tn(FILE *f, const struct tech_numbers *tn)
{
	fprintf(f, "TN=1:FWT=%u:WTS=%u:WTC=%u:WTF=%u:WCF=%u:ETF=%u\n",
		tn->fwt, tn->wts, tn->wtc, tn->wtf, tn->wcf, tn->etf);
	fprintf(f, "TN=1:BTS=%u:BTM=%u:BTC=%u:BBB=%u:BBF=%u:UBL=%u\n",
		tn->bt[BB_SMALL], tn->bt[BB_MEDIUM], tn->bt[BB_COOKIE],
		tn->bbb, tn->bbf, tn->ubl);
	fprintf(f, "TN=1:FTN=%u:FTT=%u:FTS=%u:FTG=%u\n",
		tn->ft[FT_NORMAL], tn->ft[FT_SLENDER], tn->ft[FT_SLABBY],
		tn->ft[FT_GEODETIC]);
	fprintf(f, "TN=1:FDN=%u:FDT=%u:FDS=%u:FDG=%u\n",
		tn->fd[FT_NORMAL], tn->fd[FT_SLENDER], tn->fd[FT_SLABBY],
		tn->fd[FT_GEODETIC]);
	fprintf(f, "TN=1:FSN=%u:FST=%u:FSS=%u:FSG=%u\n",
		tn->fs[FT_NORMAL], tn->fs[FT_SLENDER], tn->fs[FT_SLABBY],
		tn->fs[FT_GEODETIC]);
	fprintf(f, "TN=1:FFN=%u:FFT=%u:FFS=%u:FFG=%u\n",
		tn->ff[FT_NORMAL], tn->ff[FT_SLENDER], tn->ff[FT_SLABBY],
		tn->ff[FT_GEODETIC]);
	fprintf(f, "TN=1:FVN=%u:FVT=%u:FVS=%u:FVG=%u\n",
		tn->fv[FT_NORMAL], tn->fv[FT_SLENDER], tn->fv[FT_SLABBY],
		tn->fv[FT_GEODETIC]);
	fprintf(f, "TN=1:CCN=%u:CCT=%u:CCS=%u:CCG=%u\n",
		tn->cc[FT_NORMAL], tn->cc[FT_SLENDER], tn->cc[FT_SLABBY],
		tn->cc[FT_GEODETIC]);
	fprintf(f, "TN=1:FCN=%u:FCT=%u:FCS=%u:FCG=%u\n",
		tn->fc[FT_NORMAL], tn->fc[FT_SLENDER], tn->fc[FT_SLABBY],
		tn->fc[FT_GEODETIC]);
	fprintf(f, "TN=1:WLD=%u:G4T=%u:G4C=%u:FUT=%u:FUV=%u:FUC=%u:FGV=%u\n",
		tn->wld, tn->g4t, tn->g4c, tn->fut, tn->fuv, tn->fuc, tn->fgv);
	fprintf(f, "TN=1:EDF=%u:EMC=%u:EES=%u:EET=%u:EEC=%u\n",
		tn->edf, tn->emc, tn->ees, tn->eet, tn->eec);
	fprintf(f, "TN=1:GTF=%u:GDF=%u:GCF=%u:ESL=%u:SFT=%u:SFV=%u:SFC=%u\n",
		tn->gtf, tn->gdf, tn->gcf, tn->esl, tn->sft, tn->sfv, tn->sfc);
	fprintf(f, "TN=1:CMI=%u:CES=%u:CCC=%u:GAM=%u:GAC=%u:CSB=%u\n",
		tn->cmi, tn->ces, tn->ccc, tn->gam, tn->gac, tn->csb);
	fprintf(f, "TN=1:NAG=%u:NAH=%u:NAO=%u:NAJ=%u:CLT=%u:BMC=%u\n",
		tn->na[NAV_GEE], tn->na[NAV_H2S], tn->na[NAV_OBOE],
		tn->na[NAV_GH], tn->clt, tn->bmc);
	fprintf(f, "TN=2:RGS=%u:RGG=%u:RCS=%u:RCG=%u\n",
		tn->rgs, tn->rgg, tn->rcs, tn->rcg);
}

int save_design(FILE *f, const struct bomber *b)
{
	unsigned int i;

	fprintf(f, "MAN=%s\n", b->manf->ident);
	fprintf(f, "ENG=%u:TYP=%s:MOU=%s:EGG=%d\n", b->engines.number,
		b->engines.typ->ident, b->engines.mou->ident,
		b->engines.egg ? 1 : 0);
	for (i = LXN_NOSE; i < LXN_COUNT; i++)
		fprintf(f, "TUR=%u:TYP=%s:MOU=%s\n", i,
			b->turrets.typ[i] ? b->turrets.typ[i]->ident : "null",
			b->turrets.mou[i] ? b->turrets.mou[i]->ident : "null");
	fprintf(f, "WIN=%u:ART=%u\n", b->wing.area, b->wing.art);
	fprintf(f, "CRW=");
	for (i = 0; i < b->crew.n; i++) {
		const struct crewman *m = b->crew.men + i;

		fputc(crew_to_letter(m->pos), f);
		if (m->gun)
			fputc('*', f);
	}
	fputc('\n', f);
	fprintf(f, "BOM=%u:CAP=%u:GIR=%d:CSB=%d\n", b->bay.load,
		b->bay.cap, (int)b->bay.girth,
		b->bay.csbs ? 1 : 0);
	fprintf(f, "FUS=%d\n", (int)b->fuse.typ);
	fprintf(f, "ESL=%d:NAV=", (int)b->elec.esl);
	for (i = 0; i < NNAVAIDS; i++)
		fputc(b->elec.navaid[i] ? '1' : '0', f);
	fputc('\n', f);
	fprintf(f, "TAN=%u:PCT=%u:SST=%d\n", b->tanks.hlb, b->tanks.pct,
		b->tanks.sst ? 1 : 0);
	fprintf(f, "MTW=%u:USR=%u\n", b->mtow, b->user_mtow ? 1 : 0);
	fprintf(f, "RFL=%u\n", b->refit);
	fprintf(f, "PAR=%d\n", b->par_idx);
	fprintf(f, "RND=%u:DRG=%d:SRV=%d:VUL=%d:MNU=%d:ACC=%d\n",
		b->dice.rolled ? 1 : 0, b->dice.drag, b->dice.serv,
		b->dice.vuln, b->dice.manu, b->dice.accu);
	save_tn(f, &b->tn);
	fprintf(f, "EOD\n");
	return 0;
}

struct loaddata {
	struct bomber *b;
	const struct entities *ent;
	unsigned int ei, ti, tn;
};

static void load_error(struct loaddata *l, const char *format, ...)
{
	va_list ap;

	/* only one error can occur during load */
	l->b->new = 1;
	va_start(ap, format);
	vsnprintf(l->b->ew[0], EW_LEN, format, ap);
	va_end(ap);
	l->b->error = true;
}

#define LOADER_UINT(_name, _field)					\
static int load_##_name(const char *value, struct loaddata *l)		\
{									\
	if (sscanf(value, "%u", &l->b->_field) != 1)			\
		return -EINVAL;						\
	return 0;							\
}

#define LOADER_INT(_name, _field)					\
static int load_##_name(const char *value, struct loaddata *l)		\
{									\
	if (sscanf(value, "%d", &l->b->_field) != 1)			\
		return -EINVAL;						\
	return 0;							\
}

#define LOADER_BOOL(_name, _field)					\
static int load_##_name(const char *value, struct loaddata *l)		\
{									\
	unsigned int v;							\
									\
	if (sscanf(value, "%u", &v) != 1)				\
		return -EINVAL;						\
	l->b->_field = !!v;						\
	return 0;							\
}

static int load_man(const char *value, struct loaddata *l)
{
	unsigned int i;

	for (i = 0; i < l->ent->nmanf; i++)
		if (!strcmp(value, l->ent->manf[i]->ident)) {
			l->b->manf = l->ent->manf[i];
			return 0;
		}
	load_error(l, "Manufacturer %s not found!", value);
	return -ENOENT;
}

static int load_eng(const char *value, struct loaddata *l)
{
	if (sscanf(value, "%u", &l->b->engines.number) != 1)
		return -EINVAL;
	l->ei = 1;
	return 0;
}

static int load_ety(const char *value, struct loaddata *l)
{
	unsigned int i;

	for (i = 0; i < l->ent->neng; i++)
		if (!strcmp(value, l->ent->eng[i]->ident)) {
			l->b->engines.typ = l->ent->eng[i];
			return 0;
		}
	load_error(l, "Engine %s not found!", value);
	return -ENOENT;
}

static int load_emo(const char *value, struct loaddata *l)
{
	unsigned int i;

	for (i = 0; i < l->ent->neng; i++)
		if (!strcmp(value, l->ent->eng[i]->ident)) {
			l->b->engines.mou = l->ent->eng[i];
			return 0;
		}
	load_error(l, "Engine %s not found!", value);
	return -ENOENT;
}

static int load_tty(const char *value, struct loaddata *l)
{
	unsigned int i;

	if (!strcmp(value, "null")) {
		l->b->turrets.typ[l->ti] = NULL;
		return 0;
	}
	for (i = 0; i < l->ent->ngun; i++)
		if (!strcmp(value, l->ent->gun[i]->ident)) {
			l->b->turrets.typ[l->ti] = l->ent->gun[i];
			return 0;
		}
	load_error(l, "Turret %s not found!", value);
	return -ENOENT;
}

static int load_tmo(const char *value, struct loaddata *l)
{
	unsigned int i;

	if (!strcmp(value, "null")) {
		l->b->turrets.mou[l->ti] = NULL;
		return 0;
	}
	for (i = 0; i < l->ent->ngun; i++)
		if (!strcmp(value, l->ent->gun[i]->ident)) {
			l->b->turrets.mou[l->ti] = l->ent->gun[i];
			return 0;
		}
	load_error(l, "Turret %s not found!", value);
	return -ENOENT;
}

static int load_typ(const char *value, struct loaddata *l)
{
	if (l->ei)
		return load_ety(value, l);
	if (l->ti)
		return load_tty(value, l);
	load_error(l, "TYP (%s) not in ENG or TUR line!", value);
	return -EINVAL;
}

static int load_mou(const char *value, struct loaddata *l)
{
	if (l->ei)
		return load_emo(value, l);
	if (l->ti)
		return load_tmo(value, l);
	load_error(l, "MOU (%s) not in ENG or TUR line!", value);
	return -EINVAL;
}

static int load_egg(const char *value, struct loaddata *l)
{
	unsigned int e;

	if (!l->ei) {
		load_error(l, "EGG (%s) not in ENG line!", value);
		return -EINVAL;
	}
	if (sscanf(value, "%u", &e) != 1)
		return -EINVAL;
	l->b->engines.egg = !!e;
	return 0;
}

static int load_tur(const char *value, struct loaddata *l)
{
	if (sscanf(value, "%u", &l->ti) != 1)
		return -EINVAL;
	if (l->ti >= LXN_COUNT)
		return -EINVAL;
	return 0;
}

LOADER_UINT(win, wing.area);
LOADER_UINT(art, wing.art);

static int load_crw(const char *value, struct loaddata *l)
{
	struct crew *c = &l->b->crew;
	enum cclass p;
	char v;

	while ((v = *value++)) {
		if (v == '*') {
			if (!c->n)
				return -EINVAL;
			c->men[c->n - 1].gun = true;
			continue;
		}
		p = lookup_crew_letter(v);
		if (p >= CREW_CLASSES)
			return -EINVAL;
		if (c->n >= MAX_CREW)
			return -ENOSPC;
		c->men[c->n++].pos = p;
	}
	return 0;
}

LOADER_UINT(bom, bay.load);
LOADER_UINT(cap, bay.cap);
LOADER_UINT(gir, bay.girth);
LOADER_BOOL(csb, bay.csbs);
LOADER_UINT(fus, fuse.typ);
LOADER_UINT(esl, elec.esl);

static int load_nav(const char *value, struct loaddata *l)
{
	unsigned int i;

	for (i = 0; i < NNAVAIDS; i++) {
		switch (value[i]) {
		case '0':
			break;
		case '1':
			l->b->elec.navaid[i] = true;
			break;
		default:
			return -EINVAL;
		}
	}
	if (value[i])
		return -EINVAL;
	return 0;
}

LOADER_UINT(tan, tanks.hlb);
LOADER_UINT(pct, tanks.pct);
LOADER_BOOL(sst, tanks.sst);
LOADER_UINT(mtw, mtow);
LOADER_BOOL(usr, user_mtow);
LOADER_UINT(rfl, refit);

LOADER_BOOL(rnd, dice.rolled);
LOADER_INT(drg, dice.drag);
LOADER_INT(srv, dice.serv);
LOADER_INT(vul, dice.vuln);
LOADER_INT(mnu, dice.manu);
LOADER_INT(acc, dice.accu);
LOADER_INT(par, par_idx);

static int load_tn(const char *value, struct loaddata *l)
{
	if (sscanf(value, "%u", &l->tn) != 1)
		return -EINVAL;
	return 0;
}

static int load_eod(const char *value, __attribute__((unused)) struct loaddata *l)
{
	if (value)
		return -EINVAL;
	return 1;
}

struct loadkey {
	const char *key;
	int (*fn)(const char *value, struct loaddata *l);
} loaders[] = {
	{"MAN", load_man},
	{"ENG", load_eng},
	{"TYP", load_typ},
	{"MOU", load_mou},
	{"EGG", load_egg},
	{"TUR", load_tur},
	{"WIN", load_win},
	{"ART", load_art},
	{"CRW", load_crw},
	{"BOM", load_bom},
	{"CAP", load_cap},
	{"GIR", load_gir},
	{"CSB", load_csb},
	{"FUS", load_fus},
	{"ESL", load_esl},
	{"NAV", load_nav},
	{"TAN", load_tan},
	{"PCT", load_pct},
	{"SST", load_sst},
	{"MTW", load_mtw},
	{"USR", load_usr},
	{"RFL", load_rfl},
	{"RND", load_rnd},
	{"DRG", load_drg},
	{"SRV", load_srv},
	{"VUL", load_vul},
	{"MNU", load_mnu},
	{"ACC", load_acc},
	{"TN", load_tn},
	{"PAR", load_par},
	{"EOD", load_eod},
};

static int load_design_word(const char *key, const char *value, void *data)
{
	struct loaddata *l = data;
	unsigned int i;

	if (l->tn) {
		if (!try_load_tn_word(key, value, &l->b->tn))
			return 0;
		load_error(l, "TN key %s not found!", key);
		return -ENOENT;
	}
	for (i = 0; i < ARRAY_SIZE(loaders); i++)
		if (!strcmp(key, loaders[i].key))
			return loaders[i].fn(value, l);
	load_error(l, "Key %s not found!", key);
	return -ENOENT;
}

static int load_design_line(const char *line, void *data)
{
	struct loaddata *l = data;

	l->ei = l->ti = l->tn = 0;
	return for_each_word(line, load_design_word, l);
}

int load_design(FILE *f, struct bomber *b, const struct entities *ent)
{
	struct loaddata l = {b, ent};
	int fd = fileno(f), rc;

	memset(b, 0, sizeof(*b));
	b->manf = ent->manf[0];
	b->engines.typ = b->engines.mou = ent->eng[0];
	b->parent = b;

	if (fd < 0) {
		load_error(&l, "Invalid stream!");
		return -EBADF;
	}
	rc = for_each_line(fd, load_design_line, &l);
	return rc > 0 ? 0 : rc;
}
