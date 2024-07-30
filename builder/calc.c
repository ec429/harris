#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "calc.h"

void init_bomber(struct bomber *b, struct manf *m, struct engine *e)
{
	memset(b, 0, sizeof(*b));

	b->manf = m;
	b->engines.number = 1;
	b->engines.typ = e;
	b->engines.mou = e;
	b->wing.area = 240;
	b->wing.art = 70;
	b->crew.n = 2;
	b->crew.men[0] = (struct crewman){.pos = CCLASS_P};
	b->crew.men[1] = (struct crewman){.pos = CCLASS_N};
	b->bay.load = b->bay.cap = 1000;
	b->tanks.hlb = 19;
	b->tanks.pct = 80;
	b->par_idx = -1;
}

static void design_error(struct bomber *b, const char *format, ...)
{
	va_list ap;

	/* If this is the only error, make sure to include it even if
	 * that means overwriting one of the existing warnings.
	 */
	if (b->new < MAX_EW || !b->error) {
		if (b->new < MAX_EW)
			b->new++;
		va_start(ap, format);
		vsnprintf(b->ew[b->new - 1], EW_LEN, format, ap);
		va_end(ap);
	}
	b->error = true;
}

static void design_warning(struct bomber *b, const char *format, ...)
{
	va_list ap;

	if (b->new < MAX_EW) {
		va_start(ap, format);
		vsnprintf(b->ew[b->new++], EW_LEN, format, ap);
		va_end(ap);
	}
}

const struct bomber *mod_ancestor(const struct bomber *b)
{
	if (!b->parent || b->refit < REFIT_MOD || b->parent == b)
		return b;
	return mod_ancestor(b->parent);
}

static int calc_engines(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	struct engines *e = &b->engines;
	float ees = 1, eet = 1, eec = 1;
	float mounts = 0;

	if (!e->mou) {
		design_error(b, "Mount type not specified!");
		return -EINVAL;
	}
	if (e->mou != e->typ && e->mou != e->typ->u)
		design_error(b, "Mounts are for wrong engine type %s!",
			     e->mou->name);
	if (!e->typ->unlocked)
		design_error(b, "%s not developed yet!", e->typ->name);
	if (b->refit >= REFIT_MOD && e->typ != b->parent->engines.typ &&
	    e->typ != b->parent->engines.mou)
		design_error(b, "Engines changed in %s refit!",
			     describe_refit(b->refit));
	if (b->refit && e->number != b->parent->engines.number)
		design_error(b, "Engine count changed in %s refit!",
			     describe_refit(b->refit));
	if (b->refit >= REFIT_MOD && e->egg != b->parent->engines.egg)
		design_error(b, "Power Egg changed in %s refit!",
			     describe_refit(b->refit));
	e->odd = e->number & 1;
	e->manumatch = b->manf->eman && !strcmp(e->typ->manu, b->manf->eman);
	if (e->egg) {
		if (!tn->ees || !tn->eet || !tn->eec)
			design_error(b, "Power Egg mounts not developed yet!");
		ees = tn->ees / 100.0f;
		eet = tn->eet / 100.0f;
		eec = tn->eec / 100.0f;
	}
	e->power_factor = e->number - (e->odd ? 0.1f : 0.0f);
	e->vuln = e->typ->vul / 100.0f;
	e->rely1 = 1.0f - powf(1.0f - e->typ->fai / 1000.0f, e->number);
	if (e->number > 1)
		e->rely2 = e->rely1 - e->number * (e->typ->fai / 1000.0f) *
				      powf(1.0f - (e->typ->fai / 1000.0f),
					   e->number - 1);
	else
		e->rely2 = e->rely1;
	e->serv = 1.0f - powf(1.0f - (e->typ->svc / 1000.0f) * ees,
			      e->number);
	e->cost = e->number * e->typ->cos * eec +
		  e->number * max(e->typ->cos, e->mou->cos) * eec * 0.5f * tn->emc / 100.0f;
	if (e->number > 3) {
		e->cost *= tn->g4c / 100.0f;
		if (!tn->g4c || !tn->g4t)
			design_error(b, "Four-engined bombers not developed yet!");
	}
	e->scl = e->typ->scl;
	e->fuelrate = e->number * e->typ->bhp * 0.4f;
	mounts = e->number;
	if (e->odd)
		mounts -= 0.25f;
	mounts *= max(e->typ->twt, e->mou->twt) / 6.6f;
	if (e->number > 3)
		mounts += tn->g4t * (e->number / 2 - 1);
	if (e->manumatch)
		mounts *= 0.8f;
	e->tare = e->number * e->typ->twt * eet + mounts;
	e->drag = e->number * max(e->typ->drg, e->mou->drg) * tn->edf / 100.0f;
	return 0;
}

static int gcr[GC_COUNT] = {
	[GC_FRONT] = 1,
	[GC_BEAM_HIGH] = 2,
	[GC_BEAM_LOW] = 1,
	[GC_TAIL_HIGH] = 2,
	[GC_TAIL_LOW] = 3,
	[GC_BENEATH] = 3,
};

static int calc_turrets(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	struct turrets *t = &b->turrets;
	unsigned int fixed_gunners = 0;
	unsigned int i, j;

	t->need_gunners = 0;
	t->drag = 0;
	t->tare = 0;
	t->mtare = 0;
	t->ammo = 0;
	t->serv = 1.0f;
	t->cost = 0;
	if (t->typ[LXN_NOSE] && b->engines.odd)
		design_error(b, "Turret in nose position conflicts with engine!");
	for (j = 0; j < GC_COUNT; j++)
		t->gc[j] = 0;
	for (i = LXN_NOSE; i < LXN_COUNT; i++) {
		struct turret *g = t->typ[i];
		struct turret *m = t->mou[i];
		float tare;

		if (m)
			t->mtare += m->twt * (1.0f + tn->gtf / 100.0f);
		if (!g)
			continue;
		if (!m) {
			design_error(b, "%s without a mount!", g->name);
			return -EINVAL;
		}
		if (m != mod_ancestor(b)->turrets.mou[i])
			design_error(b, "%s mount added in %s refit!", m->name,
				     describe_refit(b->refit));
		if (g->twt > m->twt)
			design_error(b, "%s too heavy for mounts!",
				     g->name);
		if (!g->unlocked)
			design_error(b, "%s not developed yet!", g->name);
		if (g->slb && b->fuse.typ != FT_SLABBY)
			design_error(b, "%s requires slab-sided fuselage!",
				     g->name);
		t->need_gunners++;
		if (i == LXN_FIXED)
			fixed_gunners++;
		t->drag += g->drg * tn->gdf;
		tare = g->twt + m->twt * tn->gtf / 100.0f;
		t->tare += tare;
		/* Fixed guns generally carry far fewer rounds */
		t->ammo += g->gun * tn->gam * (i == LXN_FIXED ? 0.25 : 1.0);
		t->serv *= 1.0f - g->srv / 1000.0f;
		t->cost += 3.0f * tare + g->gun * tn->gcf / 10.0f + g->gun * tn->gac / 10.0f;
		for (j = 0; j < GC_COUNT; j++)
			t->gc[j] += g->gc[j] / 10.0f;
	}
	if (t->need_gunners <= fixed_gunners && b->engines.number > tn->ubl)
		design_error(b, "The Air Ministry will not allow an unarmed bomber of this size!");
	t->serv = 1.0f - t->serv;
	t->rate[0] = t->rate[1] = 0;
	for (j = 0; j < GC_COUNT; j++) {
		float x = gcr[j] * 3.0f / (3.0f + t->gc[j] * t->gc[j]);

		if (j != GC_BENEATH)
			t->rate[0] += x;
		t->rate[1] += x;
	}
	return 0;
}

static int calc_wing(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	struct wing *w = &b->wing;
	float arpen, epen;

	if (b->refit && w->area != b->parent->wing.area)
		design_error(b, "Wing area changed in %s refit!",
			     describe_refit(b->refit));
	if (b->refit && w->art != b->parent->wing.art)
		design_error(b, "Wing aspect ratio changed in %s refit!",
			     describe_refit(b->refit));
	w->ar = w->art / 10.0f;
	/* Make sure there's no risk of calculations blowing up */
	if (w->ar < 1.0) {
		design_error(b, "Wing aspect ratio too low!");
		return -EINVAL;
	}
	w->span = sqrt(w->area * w->ar);
	w->chord = sqrt(w->area / w->ar);
	w->cl = M_PI * M_PI / 6.0f / (1.0f + 2.0f / w->ar);
	w->ld = M_PI * sqrt(w->ar) * (tn->wld / 100.0f) *
				     (b->manf->wld / 100.0f);
	arpen = sqrt(max(w->ar, b->manf->wap)) / 6.0f;
	epen = b->engines.number > 3 ? b->manf->wt4 / 100.0f : 1.0f;
	w->tare = powf(w->span, tn->wts / 100.0f) *
		  powf(w->chord, tn->wtc / 100.0f) *
		  (tn->wtf / 100.0f) * arpen * epen;
	arpen = max(w->ar, 6.0f) / 6.0f;
	epen = b->engines.number > 3 ? b->manf->wc4 / 100.0f : 1.0f;
	w->cost = powf(w->tare, b->manf->wcp / 100.0f) *
		  (tn->wcf / 100.0f) * (b->manf->wcf / 100.0f) *
		  arpen * epen / 12.0f;
	/* wl and drag need b->gross, fill in later */
	return 0;
}

const char *crew_name(enum cclass c)
{
	if (c >= 0 && c < CREW_CLASSES)
		return cclasses[c].name;
	return "Unknown crew";
}

void count_crew(const struct crew *c, unsigned int *v)
{
	memset(v, 0, sizeof(*v) * CREW_CLASSES);
	unsigned int i;

	for (i = 0; i < c->n; i++)
		v[c->men[i].pos]++;
}

void count_dcrew(const struct crew *c, unsigned int *v)
{
	memset(v, 0, sizeof(*v) * CREW_CLASSES);
	unsigned int i;

	for (i = 0; i < c->n; i++)
		if (c->men[i].gun)
			v[c->men[i].pos]++;
}

static int calc_crew(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	unsigned int pcount[CREW_CLASSES];
	unsigned int count[CREW_CLASSES];
	const struct crew *pc = NULL;
	struct crew *c = &b->crew;
	unsigned int i, j;

	count_crew(c, count);
	pc = &mod_ancestor(b)->crew;
	if (b->refit >= REFIT_MOD) {
		count_crew(pc, pcount);
		for (i = 0; i < CREW_CLASSES; i++)
			if (count[i] > pcount[i])
				design_error(b, "%s%s added in %s refit!",
					     crew_name(i),
					     count[i] > pcount[i] + 1 ? "s" : "",
					     describe_refit(b->refit));
	}
	c->gunners = 0;
	c->dc = 0;
	c->bn = 0;
	memset(b->turrets.gas, 0, sizeof(b->turrets.gas));
	for (i = 0; i < c->n; i++) {
		struct crewman *m = c->men + i;

		if (m->pos == CCLASS_N)
			c->bn += m->gun ? 0.75f : 1.0f;
		if (m->pos == CCLASS_B)
			c->bn += m->gun ? 0.45f : 0.6f;
		if (m->pos == CCLASS_P && b->turrets.typ[LXN_FIXED] && !b->turrets.gas[LXN_FIXED]) {
			if (b->turrets.typ[LXN_FIXED]->ocp != 2) {
				design_warning(b, "Bad turret %s, LXN_FIXED but OCP=%d",
					       b->turrets.typ[LXN_FIXED]->ident,
					       b->turrets.typ[LXN_FIXED]->ocp);
			}
			c->gunners++;
			b->turrets.gas[LXN_FIXED] = true;
		} else if (m->pos == CCLASS_G) {
			c->gunners++;
		} else if (m->gun) {
			if (m->pos == CCLASS_W) {
				c->gunners++;
			} else if (m->pos == CCLASS_E) {
				design_error(b, "Engineer cannot dual-role as gunner");
			} else {
				for (j = LXN_NOSE; j < LXN_COUNT; j++) {
					struct turret *t = b->turrets.typ[j];

					if (!t)
						continue;
					if (b->turrets.gas[j])
						continue;
					switch (m->pos) {
					case CCLASS_P:
						if (t->ocp != 1)
							continue;
						break;
					case CCLASS_N:
						if (!t->ocn)
							continue;
						break;
					case CCLASS_B:
						if (!t->ocb)
							continue;
						break;
					default: /* can't happen */
						design_error(b, "Unknown cclass %d at %d",
							     m->pos, i + 1);
						return -EINVAL;
					}
					c->gunners++;
					b->turrets.gas[j] = true;
					break;
				}
				if (j >= LXN_COUNT)
					design_warning(b, "No turrets found for %s to dual-role operate",
						       crew_name(m->pos));
			}
		}
		if (m->pos == CCLASS_E)
			c->dc += m->gun ? 0.75f : 1.0f;
		if (m->pos == CCLASS_W) {
			c->dc += m->gun ? 0.45f : 0.6f;
			/* Radionavigation by pre-GEE methods,
			 * such as beams (Jay, Elektra Sonne)
			 */
			if (b->elec.esl >= ESL_HIGH)
				c->bn += m->gun ? 0.15f : 0.2f;
		}
	}
	if (!count[CCLASS_P])
		design_error(b, "Crew must include a pilot!");
	if (!count[CCLASS_N])
		design_error(b, "Crew must include a navigator!");
	c->engineers = count[CCLASS_E];
	/* Removing crew in a mod doesn't save their cmi */
	c->tare = pc->n * tn->cmi;
	c->gross = c->n * 168;
	if (c->gunners < b->turrets.need_gunners)
		design_warning(b, "Fewer gunners than turrets, defence will be weakened.");
	c->cct = c->tare * (tn->ccc / 100.0f - 1.0f);
	c->es = tn->ces / 100.0f;
	return 0;
}

static int calc_bombbay(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	struct bombbay *a = &b->bay;
	unsigned int bbb;

	if (b->refit && a->cap != b->parent->bay.cap)
		design_error(b, "Bombbay capacity changed in %s refit!",
			     describe_refit(b->refit));
	if (a->load > a->cap) /* Is this plane a TARDIS? */
		design_error(b, "Bomb load exceeds bombbay capacity!");
	if (b->refit && a->girth != b->parent->bay.girth)
		design_error(b, "Bombbay girth changed in %s refit!",
			     describe_refit(b->refit));
	if (a->girth < 0 || a->girth >= BB_COUNT) { /* can't happen */
		design_error(b, "Nonexistent bombbay girth!");
		return -EINVAL;
	}
	if (!tn->bt[a->girth])
		design_error(b, "Bay for %s not developed yet!",
			     describe_bbg(a->girth));
	if (a->csbs && !tn->csb)
		design_error(b, "Course-Setting Bomb Sight not developed yet!");
	a->factor = (tn->bt[a->girth] / 1000.0f) *
		    (b->manf->bt[a->girth] / 100.0f);
	bbb = (tn->bbb + b->manf->bbb) * 1000;
	if (a->cap > bbb)
		a->bigfactor = (a->cap - bbb) / (tn->bbf * 1e5f);
	else
		a->bigfactor = 0.0f;
	a->tare = a->cap * (a->factor + a->bigfactor) +
		  (a->csbs ? 90.0f : 20.0f);
	a->cost = a->csbs ? 1200.0f : 0.0f;
	a->cookie = a->girth == BB_COOKIE ||
		    (a->girth == BB_MEDIUM && tn->bmc &&
		     b->fuse.typ != FT_SLENDER);
	return 0;
}

static int calc_fuselage(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	struct fuselage *f = &b->fuse;

	if (b->refit && f->typ != b->parent->fuse.typ)
		design_error(b, "Fuselage type changed in %s refit!",
			     describe_refit(b->refit));
	/* First need core_mtare, as input to fuse_tare */
	b->core_tare = (b->turrets.tare + b->crew.tare + b->bay.tare) *
		       b->manf->act / 100.0f;
	b->core_mtare = (b->turrets.mtare + b->crew.tare + b->bay.tare) *
			b->manf->act / 100.0f;
	if (f->typ < 0 || f->typ >= FT_COUNT) { /* can't happen */
		design_error(b, "Nonexistent fuselage type!");
		return -EINVAL;
	}
	if (f->typ == FT_GEODETIC && !b->manf->geo)
		design_error(b, "This manufacturer cannot design geodetics!");
	f->tare = b->core_mtare * (tn->ft[f->typ] / 100.0f) *
		  b->manf->ft[f->typ] / 100.0f;
	f->serv = tn->fs[f->typ] / 1000.0f;
	f->fail = tn->ff[f->typ] / 1000.0f;
	f->cost = f->tare * 1.2f * (b->manf->acc / 100.0f) *
		  (tn->fc[f->typ] / 100.0f);
	f->vuln = tn->fv[f->typ] / 100.0f;
	/* drag needs b->tare, fill in later */
	return 0;
}

static unsigned int nacost[NNAVAIDS] = {
	[NAV_GEE] = 500,
	[NAV_H2S] = 2500,
	[NAV_OBOE] = 3000,
	[NAV_GH] = 0, // XXX it doesn't do anything yet, so don't charge for it
};

static int calc_electrics(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	struct electrics *e = &b->elec;
	unsigned int i;

	if (b->refit >= REFIT_MOD && e->esl != b->parent->elec.esl)
		design_error(b, "Electric supply changed in %s refit!",
			     describe_refit(b->refit));
	if (e->esl > tn->esl)
		design_error(b, "Electrics %s not developed yet!",
			     describe_esl(e->esl));
	e->ncost = 0.0f;
	for (i = 0; i < NNAVAIDS; i++)
		if (e->navaid[i]) {
			if (!tn->na[i])
				design_error(b, "Navaid %s not developed yet!",
					     describe_navaid(i));
			else if (e->esl < (i == NAV_GEE ? ESL_HIGH : ESL_STABLE))
				design_error(b, "Navaid %s requires better electrics!",
					     describe_navaid(i));
			e->ncost += nacost[i];
		}
	if (b->turrets.typ[LXN_VENTRAL] && e->navaid[NAV_H2S])
		design_error(b, "H₂S conflicts with turret in ventral position!");
	/* This is all hard-coded; datafiles / techlevels don't get to
	 * change these coefficients.
	 */
	switch (e->esl) {
	case ESL_LOW:
		e->cost = 90.0f;
		break;
	case ESL_HIGH:
		e->cost = 600.0f / max(b->engines.number, 1);
		break;
	case ESL_STABLE:
		e->cost = (b->fuse.typ == FT_SLENDER ? 1200.0f : 900.0f) +
			  500.0f / max(b->engines.number, 1);
		break;
	default: /* can't happen */
		design_error(b, "Nonexistent electric supply level!");
		return -EINVAL;
	}
	return 0;
}

static int calc_tanks(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	struct tanks *t = &b->tanks;

	if (b->refit >= REFIT_MOD && t->hlb != b->parent->tanks.hlb)
		design_error(b, "Fuel capacity changed in %s refit!",
			     describe_refit(b->refit));
	if (t->pct > 100) /* Is this plane a TARDIS? */
		design_error(b, "Fuel tanks more than 100%% full!");
	t->cap = t->hlb * 100.0f;
	t->mass = t->hlb * t->pct;
	t->hours = t->mass / b->engines.fuelrate;
	t->tare = t->cap * (tn->fut / 1000.0f);
	t->cost = t->tare * (tn->fuc / 100.0f);
	/* Wing thickness scales linearly with chord, so volume (which
	 * is what matters for fuel storage) scales with area * chord
	 * Tank ullage volume is full of petrol fumes so contributes to
	 * vulnerability too
	 */
	t->ratio = t->cap * 1.45f / max(b->wing.area * b->wing.chord, 1.0f);
	if (t->ratio > (t->sst ? 2.5f : 2.0f))
		design_warning(b, "Wing is crammed with fuel, vulnerability high.");
	t->vuln = t->ratio * tn->fuv / 400.0f;
	if (t->sst) {
		if (!tn->sft || !tn->sfc || !tn->sfv)
			design_error(b, "Self sealing tanks not developed yet!");
		t->tare *= tn->sft / 100.0f;
		t->cost *= tn->sfc / 100.0f; /* note ignores SFT */
		t->vuln *= tn->sfv / 100.0f;
	}
	if (b->fuse.typ == FT_GEODETIC)
		t->vuln *= tn->fgv / 100.0f;
	return 0;
}

/* altitude in thousands ft */
static float engine_power(const struct engines *e, float alt)
{
	float ffth = 16.0f, mfth = 10.25, scp = 0.02f;
	float msp, fsp = 0;

	switch (e->scl) {
	case 3:
		ffth = 21.0f;
		mfth = 12.0f;
		scp = 0.06f;
		/* fallthrough */
	case 2:
		fsp = expf(min(ffth - alt, 0.0f) / 25.1f) - scp;
		/* fallthrough */
	case 1:
		msp = expf(min(mfth - alt, 0.0f) / 25.1f);
		break;
	default: /* bad data */
		return 0.0f;
	}

	return e->power_factor * e->typ->bhp * max(msp, fsp);
}

float wing_lift(const struct wing *w, float v)
{
	float u = v * 22.0f / 15.0f; /* convert mph to ft/s */

	return 0.5f * w->cl * 0.0075f * u * u * w->area;
}

/* lift in lbf, altitude in thousands ft */
static float wing_minv(const struct wing *w, float lift, float alt)
{
	float rho = 0.0075f * expf(-alt / 25.1f);

	return sqrt(lift * 2.0f / (w->cl * rho * w->area)) * 15.0f / 22.0f;
}

/* altitude in thousands ft */
static float climb_rate(const struct bomber *b, float alt)
{
	float v = wing_minv(&b->wing, b->gross, alt);
	float lpwr = b->drag * v / 375.0f, pwr, cpwr;

	pwr = engine_power(&b->engines, alt);
	cpwr = (pwr - lpwr) * 0.52f;
	return cpwr * 33e3f / b->gross;
}

/* altitude in thousands ft */
static float airspeed(const struct bomber *b, float alt)
{
	float pwr = engine_power(&b->engines, alt);
	float nwd, a, c, m;

	/* drag (other than wing) scales with v², and is normalised
	 * to 200mph.  So we should have:
	 * v * WD + v³ * NWD / 200² = pwr * 375
	 * But this appears to make Mosquitos impossible, and in any
	 * case doesn't allow for AoA dependence of NWD.  So let's
	 * instead scale NWD to v, yielding:
	 * v * WD + v² * NWD / 200 = pwr * 375
	 * This is a quadratic, solve by the well-known formula.
	 */
	nwd = b->drag - b->wing.drag;
	a = nwd / 200.0f;
	c = -pwr * 375.0f;
	m = sqrt(b->wing.drag * b->wing.drag - 4.0f * a * c);
	return (m - b->wing.drag) / (2.0f * a);
}

#define ALTITUDE_STEP	200	// feet

static int calc_ceiling(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	unsigned int alt; // units of ALTITUDE_STEP ft
	float tim = 0; // minutes

	for (alt = 0; alt * ALTITUDE_STEP < 35000; alt++) {
		float c = climb_rate(b, alt * (ALTITUDE_STEP * 1e-3));

		if (c < 480.0f)
			break;
		if (tim > tn->clt)
			break;
		tim += ALTITUDE_STEP / c;
	}
	b->ceiling = alt * (ALTITUDE_STEP * 1e-3);
	return 0;
}

static int calc_perf(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	bool concrete = tn->rcs;
	int rc;

	b->tare = b->core_tare + b->fuse.tare + b->tanks.tare +
		  b->wing.tare * tn->fwt / 100.0f +
		  b->engines.tare * tn->etf / 100.0f;
	b->gross = b->tare + b->tanks.mass + b->turrets.ammo +
		   b->crew.gross + b->bay.load;
	if (b->refit < REFIT_MOD) {
		if (!b->user_mtow)
			b->mtow = ceil(b->gross);
	} else {
		/* Cannot change structure in MOD or DOCTRINE refit */
		b->mtow = b->parent->mtow;
	}
	if (floor(b->gross) > b->mtow)
		design_error(b, "Exceeded MTOW of %ulb", b->mtow);
	/* Gross weight with everything filled up to maximum.
	 * Used for development time calculations.
	 */
	b->overgross = b->tare + b->tanks.cap + b->turrets.ammo +
		       b->crew.gross + b->bay.cap;
	b->wing.wl = b->gross / max(b->wing.area, 1.0f);
	b->wing.drag = b->gross / max(b->wing.ld, 1.0f);
	b->fuse.drag = sqrt(b->tare - b->wing.tare) *
		       (b->manf->fd[b->fuse.typ] / 100.0f) *
		       (tn->fd[b->fuse.typ] / 10.0f);
	b->drag = b->wing.drag + b->fuse.drag + b->engines.drag +
		  b->turrets.drag;
	b->drag *= (100 + b->dice.drag) / 100.0f;
	b->takeoff_spd = wing_minv(&b->wing, b->gross, 0.0f) * 1.6f;
	if (concrete && floor(b->gross) > tn->rcg * 1000)
		design_warning(b, "Gross weight too high for concrete runways, load will be reduced in service.");
	else if (concrete && b->takeoff_spd - 0.1 > tn->rcs)
		design_warning(b, "Take-off speed too high for concrete runways, load will be reduced in service.");
	else if (floor(b->gross) > tn->rgg * 1000)
		design_warning(b, "Gross weight too high for grass runways.");
	else if (b->takeoff_spd - 0.1 > tn->rgs)
		design_warning(b, "Take-off speed too high for grass runways.");
	rc = calc_ceiling(b);
	if (rc)
		return rc;
	b->cruise_alt = min(b->ceiling, 10.0f) +
			max(b->ceiling - 10.0f, 0) / 2.0f;
	b->cruise_spd = airspeed(b, b->cruise_alt);
	b->init_climb = climb_rate(b, 0.0f);
	if (b->init_climb < 540.0f)
		design_error(b, "Design can barely take off!");
	else if (b->init_climb < 640.0f)
		design_warning(b, "Climb rate is very slow.");
	b->deck_spd = airspeed(b, 0.0f);
	b->range = max(b->tanks.hours * 0.45f * b->cruise_spd - 20.0f, 0.0f);
	if (b->range < 200.0f)
		design_error(b, "Range is far too low!");
	else if (b->range < 320.0f)
		design_warning(b, "Range is on the low side.");
	return 0;
}

static int calc_rely(struct bomber *b)
{
	b->serv = 1.0f - (b->engines.serv * 6.0f + b->turrets.serv +
			  b->fuse.serv + b->manf->svp / 100.0f);
	b->serv += b->dice.serv / 100.0f;
	b->serv = min(max(b->serv, 0.0f), 1.0f);
	b->fail = b->engines.rely1 * 2.0f + b->engines.rely2 * 30.0f +
		  b->fuse.fail;
	if (b->crew.engineers)
		b->fail *= 0.9f / b->crew.es;
	return 0;
}

static int calc_combat(struct bomber *b)
{
	unsigned int sch;
	float sgf, lbb;

	b->roll_pen = powf(b->wing.ar, 0.8f) * 0.7f;
	b->turn_pen = sqrt(max(b->wing.wl - b->manf->tpl, 0.0f));
	b->manu_pen = b->roll_pen + b->turn_pen;
	b->manu_pen *= (100 + b->dice.manu) / 100.0f;
	b->evade_factor = (max(33.0f - b->ceiling, 2.0f) / 13.0f) *
			  powf(max(350.0f - b->cruise_spd, 30.0f) / 1.2f,
			       0.45) *
			  (1.0f - 0.3f / max(b->manu_pen - 4.5f, 0.5f));
	b->vuln = (b->engines.vuln + b->fuse.vuln) *
		  max(3.8f - sqrt(b->crew.dc * b->crew.es), 1.0f) +
		  b->tanks.vuln;
	b->vuln *= (100 + b->dice.vuln) / 100.0f;
	b->flak_factor = b->vuln * 6.0f *
			 sqrt(max(35.0f - b->ceiling, 2.0f) / 1.5f);
	/* Looks backwards?  Lower is better for gunrate */
	sgf = max((b->turrets.need_gunners + 1.0f) / (b->crew.gunners + 1.0f),
		  1.0f);
	for (sch = 0; sch < 2; sch++)
		b->defn[sch] = powf(b->evade_factor, 0.8f) *
			       (b->vuln * 4.0f +
				b->turrets.rate[sch] * sgf) /
			       1.8f;
	/* accu contribs bn, speed, esl */
	/* bn of 1.45 -> .24
	 * speed of 210 -> .124; 150 -> .101; 340 -> .165.
	 * esl of High -> .17; Stable -> .208.
	 * light bomber bonus: 15k -> .04; 7k5 -> .09
	 */
	lbb = max(21000 - b->gross, 0.0f) / 150000.0f;
	b->accu = sqrt(b->crew.bn) * b->crew.es * 0.2f +
		  powf(b->cruise_spd, 0.6f) / 200.0f +
		  sqrt(1.0f + b->elec.esl) * 0.12f +
		  (b->bay.csbs ? 0.12f : 0.0f) + lbb;
	b->accu += b->dice.accu / 100.0f;
	return 0;
}

static int calc_cost(struct bomber *b)
{
	const struct tech_numbers *tn = &b->tn;
	float structure_cost;

	b->core_cost = (b->core_tare + b->crew.cct) *
		       (b->manf->acc / 100.0f) *
		       (tn->cc[b->fuse.typ] / 100.0f) *
		       (b->engines.number > 2 ? 2.0f : 1.0f);
	structure_cost = b->core_cost + b->bay.cost + b->fuse.cost +
			 b->wing.cost;
	b->stress_factor = powf(b->mtow * 0.5f / b->tare, 2);
	structure_cost *= b->stress_factor;
	b->cost = b->engines.cost + b->turrets.cost + structure_cost +
		  b->elec.cost + b->elec.ncost + b->tanks.cost;
	return 0;
}

static int calc_dev(struct bomber *b)
{
	float base_tproto = powf(b->overgross, 0.3f) * powf(b->cost, 0.2f);
	float base_tprod = powf(b->overgross, 0.4f) * powf(b->cost, 0.2f);
	float add_tproto = 0.0f, add_tprod = 0.0f;
	unsigned int pcount[CREW_CLASSES];
	unsigned int count[CREW_CLASSES];
	float bof = max(b->manf->bof, 1);
	unsigned int i;

	if (b->manf->proto_idx >= 0)
		design_warning(b, "Manufacturer is already building another prototype");
	else if (b->manf->prod_idx >= 0)
		design_warning(b, "Manufacturer is already tooling another design");
	count_crew(&b->crew, count);
	switch (b->refit) {
	case REFIT_FRESH:
		b->tproto = base_tproto * 90.0f / bof;
		b->tprod = base_tprod * 40.0f / bof;
		b->cproto = b->cost * 15.0f;
		b->cprod = b->cost * 30.0f;
		break;
	case REFIT_MARK:
		b->tproto = base_tproto * 10.0f / bof;
		b->tprod = base_tprod * 6.0f / bof;
		b->cproto = b->cost * 1.5f;
		b->cprod = b->cost * 3.0f;
		if (b->engines.typ != b->parent->engines.typ) {
			add_tproto += powf(b->engines.tare, 0.6f) *
				      powf(b->engines.cost, 0.4f) * 0.6f;
			add_tprod += powf(b->engines.tare, 0.8f) *
				    powf(b->engines.cost, 0.4f) * 0.32f;
			if (b->engines.typ == b->parent->engines.mou) {
				add_tproto *= 0.1f;
				add_tprod *= 0.1f;
			} else if (b->engines.egg && b->parent->engines.egg) {
				add_tproto *= 0.6f;
				add_tprod *= 0.6f;
			}
		}
		for (i = LXN_NOSE; i < LXN_COUNT; i++)
			if (b->turrets.typ[i] != b->parent->turrets.typ[i]) {
				const struct turret *t = b->turrets.typ[i];
				const struct turret *p = b->parent->turrets.typ[i];

				if (t) {
					/* turret added or replaced */
					add_tproto += powf(t->twt, 0.6f);
					add_tprod += powf(t->twt, 0.8f) * 0.6f;
				} else {
					/* turret removed */
					add_tproto += powf(p->twt, 0.6f) * 0.2f;
					add_tprod += powf(p->twt, 0.8f) * 0.12f;
				}
			}
		count_crew(&b->parent->crew, pcount);
		for (i = 0; i < CREW_CLASSES; i++) {
			unsigned int delta = max((int)(count[i] - pcount[i]), 0);

			add_tproto += 8 * delta;
			add_tprod += 9 * delta;
		}
		if (b->bay.csbs && !b->parent->bay.csbs) {
			add_tproto += 4;
			add_tprod += 4;
		}
		if (b->elec.esl > b->parent->elec.esl) {
			add_tproto += b->elec.cost * 5.0f;
			add_tprod += b->elec.cost * 7.0f;
		}
		if (b->tanks.cap > b->parent->tanks.cap) {
			add_tproto += b->tanks.cost * 0.5f;
			add_tprod += b->tanks.cost * 0.8f;
		}
		if (b->tanks.sst && !b->parent->tanks.sst) {
			add_tproto += b->tanks.cost * 0.3f;
			add_tprod += b->tanks.cost * 0.5f;
		}
		b->tproto += sqrt(add_tproto) * 100.0f / bof;
		b->tprod += sqrt(add_tprod) * 100.0f / bof;
		b->cproto += add_tproto;
		b->cprod += add_tprod;
		break;
	case REFIT_MOD:
		if (b->engines.typ != b->parent->engines.typ) {
			add_tproto += powf(b->engines.tare, 0.6f) *
				      powf(b->engines.cost, 0.4f) * 0.06f;
			add_tprod += powf(b->engines.tare, 0.8f) *
				    powf(b->engines.cost, 0.4f) * 0.032f;
		}
		if (b->engines.egg && b->parent->engines.egg) {
			add_tproto *= 0.6f;
			add_tprod *= 0.6f;
		}
		for (i = LXN_NOSE; i < LXN_COUNT; i++)
			if (b->turrets.typ[i] != b->parent->turrets.typ[i]) {
				const struct turret *t = b->turrets.typ[i];
				const struct turret *p = b->parent->turrets.typ[i];

				if (t) {
					/* turret added or replaced */
					add_tproto += powf(t->twt, 0.6f);
					add_tprod += powf(t->twt, 0.8f) * 0.6f;
				} else {
					/* turret removed */
					add_tproto += powf(p->twt, 0.6f) * 0.2f;
					add_tprod += powf(p->twt, 0.8f) * 0.12f;
				}
			}
		if (b->bay.csbs && !b->parent->bay.csbs) {
			add_tproto += 4;
			add_tprod += 4;
		}
		if (b->tanks.sst && !b->parent->tanks.sst) {
			add_tproto += b->tanks.cost * 0.5f;
			add_tprod += b->tanks.cost * 0.8f;
		}
		b->tproto = base_tproto * 7.0f / bof +
			    sqrt(add_tproto) * 100.0f / bof;
		b->tprod = base_tprod * 7.0f / bof +
			   sqrt(add_tprod) * 100.0f / bof;
		b->cproto = b->cost * 0.9f;
		b->cprod = b->cost * 1.2f;
		break;
	case REFIT_DOCTRINE:
		/* It costs nothing to change your doctrine */
		b->tproto = b->tprod = 0.0f;
		b->cproto = b->cprod = 0.0f;
		break;
	default:
		design_error(b, "Unknown refit level %d", b->refit);
		return -EINVAL;
	}
	return 0;
}

static int calc_refit(struct bomber *b, const struct tech_numbers *tn)
{
	size_t start;

	if (!b->refit) {
		b->tn = *tn;
		return 0;
	}
	if (!b->parent) {
		design_error(b, "Refit must have a parent design!");
		return -EINVAL;
	}
	b->tn = b->parent->tn;
	switch (b->refit) {
	case REFIT_MARK:
		start = offsetof(struct tech_numbers, mark_block);
		break;
	case REFIT_MOD:
		start = offsetof(struct tech_numbers, mod_block);
		break;
	case REFIT_DOCTRINE:
		start = offsetof(struct tech_numbers, doctrine_block);
		break;
	default:
		design_error(b, "Unknown refit level %d", b->refit);
		return -EINVAL;
	}
	memcpy(((char *)&b->tn) + start, ((char *)tn) + start,
	       sizeof(*tn) - start);
	return 0;
}

int calc_bomber(struct bomber *b, const struct tech_numbers *tn)
{
	int rc;

	/* clear old errors and warnings */
	b->error = false;
	b->new = 0;

	rc = calc_refit(b, tn);
	if (rc)
		return rc;

	rc = calc_engines(b);
	if (rc)
		return rc;

	rc = calc_turrets(b);
	if (rc)
		return rc;

	rc = calc_wing(b);
	if (rc)
		return rc;

	rc = calc_crew(b);
	if (rc)
		return rc;

	rc = calc_bombbay(b);
	if (rc)
		return rc;

	rc = calc_fuselage(b);
	if (rc)
		return rc;

	rc = calc_electrics(b);
	if (rc)
		return rc;

	rc = calc_tanks(b);
	if (rc)
		return rc;

	rc = calc_perf(b);
	if (rc)
		return rc;

	rc = calc_rely(b);
	if (rc)
		return rc;

	rc = calc_combat(b);
	if (rc)
		return rc;

	rc = calc_cost(b);
	if (rc)
		return rc;

	rc = calc_dev(b);
	if (rc)
		return rc;

	return 0;
}

/* Copied from Harris rand.c */
static int irandu(int n)
{
	if(!n) return(0);
	// This is poor quality randomness, but that doesn't really matter here
	return(rand()%n);
}

/* Sets the random factors, to be picked up by subsequent recalcs */
int do_randomise(struct bomber *b)
{
	switch (b->refit) {
	case REFIT_FRESH:
		b->dice.drag = irandu(11) - 5;
		b->dice.serv = irandu(9) - 4;
		b->dice.vuln = irandu(21) - 10;
		b->dice.manu = irandu(11) - 5;
		b->dice.accu = irandu(9) - 4;
		break;
	case REFIT_MARK:
		b->dice.drag += irandu(5) - 2;
		b->dice.serv += irandu(7) - 3;
		b->dice.vuln += irandu(11) - 5;
		b->dice.manu += irandu(5) - 2;
		b->dice.accu += irandu(3) - 1;
		break;
	case REFIT_MOD:
		b->dice.serv += irandu(5) - 2;
		b->dice.vuln += irandu(5) - 2;
		break;
	default:
		break;
	}
	#define CLAMP(f, v)	b->dice.f = min(max(b->dice.f, -v), v)
	CLAMP(drag, 5);
	CLAMP(serv, 4);
	CLAMP(vuln, 10);
	CLAMP(manu, 5);
	CLAMP(accu, 4);
	#undef CLAMP
	b->dice.rolled = true;
	return 0;
}
