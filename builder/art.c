#include <math.h>

#include "art.h"
#include "../render.h"
#include "../rand.h"
#include "../globals.h"

static float clerp(float a, float b, float x)
{
	float t=(1-cos(x*M_PI))/2.0;
	return a*(1-t)+b*t;
}

/* Generate a bicolour camouflage pattern fill */
static void render_camo(SDL_Surface *s, unsigned int lamb, atg_colour fore, atg_colour back)
{
	/* start by creating a wobble grid of noise control points */
	unsigned int grw = s->w / lamb, grh = s->h / lamb;
	float xoff[grw+1][grh+1];
	float z[grw+1][grh+1];
	for (unsigned int x = 0; x < grw + 1; x++)
		for (unsigned int y = 0; y < grh + 1; y++) {
			xoff[x][y] = x ? drandu(lamb) : 0;
			z[x][y] = drandu(1);
		}
	/* now interpolate and use a threshold to choose a colour */
	for (int x = 0; x < s->w; x++)
		for (int y = 0; y < s->h; y++) {
			unsigned int grx = x/lamb;
			unsigned int gry = y/lamb;
			float dy = y / (float)lamb - gry;
			float xoffat = clerp(xoff[grx][gry], xoff[grx][gry+1], dy);
			float xoffatnext;
			if (grx && x - grx * lamb < xoffat) {
				grx--;
				xoffatnext = xoffat;
				xoffat = clerp(xoff[grx][gry], xoff[grx][gry + 1], dy);
			} else {
				xoffatnext = clerp(xoff[grx + 1][gry], xoff[grx + 1][gry + 1], dy);
			}
			/* Reverse lerp
			 * x = grx*lamb + xoffat*(1-dx) + (xoffatnext+lamb)*dx
			 * => x - grx*lamb - xoffat = (xoffatnext + lamb - xoffat)*dx
			 */
			float dx = (x - grx*lamb - xoffat)/(xoffatnext + lamb - xoffat);
			float u = clerp(z[grx][gry], z[grx + 1][gry], dx);
			float b = clerp(z[grx][gry + 1], z[grx + 1][gry + 1], dx);
			float v = clerp(u, b, dy);
			pset(s, x, y, (v < 0.5f)?fore:back);
		}
}

int init_camos(struct builder_data *builder)
{
	srand(0x44f); /* Stable seed for camo generation */
	builder->camo_small = SDL_CreateRGBSurface(SDL_SWSURFACE,
						   36 * 16, 36, 32, 0xff000000,
						   0xff0000, 0xff00, 0xff);
	if (!builder->camo_small) {
		fprintf(stderr, "Failed to init camos: %s\n", SDL_GetError());
		return 1;
	}
	render_camo(builder->camo_small, 3,
		    (atg_colour){31, 94, 12, ATG_ALPHA_OPAQUE},
		    (atg_colour){80, 50, 10, ATG_ALPHA_OPAQUE});
	return 0;
}

static float fuse_width_at(const struct bomber *b, float t)
{
	if (t < 0 || t > 1)
		return 0;
	switch (b->fuse.typ) {
	case FT_NORMAL:
		if (t > 0.9f)
			return (1 - t) * 20.0f;
		if (t > 0.5f)
			return 2;
		if (t > 0.05f)
			return t + 1.5f;
		return t * 32.0f;
	case FT_SLENDER:
		if (t > 0.9f)
			return (1 - t) * 27.0f;
		if (t > 0.56f)
			return 2.7;
		if (t > 0.44f)
			return t * 8.0f - 1.78f;
		if (t > 0.1f)
			return t + 1.3f;
		return t * 4.0f;
	case FT_SLABBY:
		if (t > 0.93f)
			return (1 - t) * 30.0f;
		if (t > 0.05f)
			return 2.1f;
		return (t * 42.0f);
	case FT_GEODETIC:
		return powf(t, 0.4f) * powf(1.0f - t, 0.3f) * 4;
	default:
		return 2;
	}
}

static void render_mini_fuse(SDL_Surface *s, const struct bomber *b, SDL_Surface *feat)
{
	/* Fuse length depends on core_tare, about 660lb for a Blen (18px),
	 * about 3900lb for a Stir (25px) */
	float flen = 13 + sqrt(b->core_tare / 26.0f);
	float f0 = s->w - flen / 0.75f;
	float f1 = s->w + flen / 1.5f;
	for (int x = 0; x < s->w; x++)
		for (int y = 0; y < s->h; y++) {
			unsigned int sx = x + (s->h - y - 1);
			unsigned int sy = abs(x + y + 1 - s->h);
			float t = (sx - f0) / (f1 - f0);
			float fw = fuse_width_at(b, t) * 1.2f;
			unsigned char v = 255;
			if (sy < fw - 1.5f)
				v = 0;
			else if (sy < fw)
				v = min(170 * (sy + 1.5f - fw), 255);
			pset(s, x, y, (atg_colour){0, 0, 0, v});
		}
	/* Render cockpit, turrets in appropriate locations */
	unsigned int cx0 = (s->w + flen / 3.6f) / 2;
	bool daft = b->turrets.typ[LXN_DORSAL] && b->turrets.typ[LXN_DORSAL]->art;
	if (daft)
		cx0 -= 3;
	unsigned int cy0 = s->h - cx0 - 1;
	unsigned int cx1 = f1/2 - 1;
	unsigned int cy1 = s->h - cx1 - 1;
	if (daft) {
		line(feat, cx0, cy0, cx0 + 2, cy0 - 2, (atg_colour){160, 255, 255, 80});
		line(feat, cx0 + 3, cy0 - 3, cx1, cy1, (atg_colour){160, 255, 255, 96});
	} else {
		line(feat, cx0, cy0, cx1, cy1, (atg_colour){160, 255, 255, 96});
	}
	line(feat, cx0, cy0-1, cx1-1, cy1, (atg_colour){160, 255, 255, 48});
	line(feat, cx0+1, cy0, cx1, cy1+1, (atg_colour){160, 255, 255, 48});
	if (b->turrets.typ[LXN_NOSE]) {
		unsigned int x = f1/2, y = s->h - x - 1;
		pset(feat, x, y, (atg_colour){160, 255, 255, 64});
		pset(feat, x-1, y, (atg_colour){160, 255, 255, 96});
		pset(feat, x, y+1, (atg_colour){160, 255, 255, 96});
		pset(feat, x-1, y-1, (atg_colour){69, 108, 108, 64});
	}
	if (b->turrets.typ[LXN_DORSAL] && !b->turrets.typ[LXN_DORSAL]->art) {
		unsigned int x = (s->w - flen / 5.0f) / 2, y = s->h - x - 1;
		pset(feat, x, y, (atg_colour){160, 255, 255, 96});
		pset(feat, x-1, y, (atg_colour){160, 255, 255, 48});
		pset(feat, x+1, y, (atg_colour){160, 255, 255, 48});
		pset(feat, x, y-1, (atg_colour){160, 255, 255, 48});
		pset(feat, x, y+1, (atg_colour){160, 255, 255, 48});
		pset(feat, x-1, y-1, (atg_colour){69, 108, 108, 48});
		pset(feat, x+1, y-1, (atg_colour){69, 108, 108, 48});
		pset(feat, x-1, y+1, (atg_colour){69, 108, 108, 48});
		pset(feat, x+1, y+1, (atg_colour){69, 108, 108, 48});
	}
	if (b->turrets.typ[LXN_TAIL]) {
		unsigned int x = f0/2, y = s->h - x - 1;
		pset(feat, x, y, (atg_colour){160, 255, 255, 96});
		pset(feat, x+1, y, (atg_colour){160, 255, 255, 96});
		pset(feat, x, y-1, (atg_colour){160, 255, 255, 96});
		pset(feat, x+1, y-1, (atg_colour){69, 108, 108, 96});
	}
}

static float wing_chord_at(const struct bomber *b, float ts)
{
	if (ts < 0 || ts > 1)
		return 0;
	switch (b->fuse.typ) {
	case FT_NORMAL:
		if (ts > 0.6f)
			return 1.6f - ts;
		return 1;
	case FT_SLENDER:
		if (ts > 0.93f)
			return 3.39f - ts * 3;
		if (ts > 0.2f)
			return 0.6f + (0.93f - ts) * 0.4f / 0.73f;
		return 1.6f - 3.0f * ts;
	case FT_SLABBY:
		return powf(1 - ts, 0.1);
	case FT_GEODETIC:
		return powf(1 - ts*ts, 0.4) * 1.2f;
	default:
		return 1;
	}
}

static void render_roundel(SDL_Surface *deca, unsigned int x, unsigned int y)
{
	pset(deca, x, y, (atg_colour){96, 0, 0, ATG_ALPHA_OPAQUE});
	pset(deca, x-1, y, (atg_colour){16, 0, 48, 128});
	pset(deca, x+1, y, (atg_colour){16, 0, 48, 128});
	pset(deca, x, y-1, (atg_colour){16, 0, 48, 128});
	pset(deca, x, y+1, (atg_colour){16, 0, 48, 128});
	pset(deca, x-1, y-1, (atg_colour){16, 0, 48, 64});
	pset(deca, x-1, y+1, (atg_colour){16, 0, 48, 64});
	pset(deca, x+1, y-1, (atg_colour){16, 0, 48, 64});
	pset(deca, x+1, y+1, (atg_colour){16, 0, 48, 64});
}

static void render_mini_wing(SDL_Surface *s, const struct bomber *b, SDL_Surface *deca)
{
	/* Show about 10px for a Blen (56ft), max 15px (120ft?) */
	float wlen = powf(b->wing.span, 0.6f)*0.85f;
	float wcho = wlen / b->wing.ar*4.5f;
	/* Render wing shape to mask layer */
	for (int x = 0; x < s->w; x++)
		for (int y = 0; y < s->h; y++) {
			unsigned int sx = x + (s->h - y - 1);
			unsigned int sy = abs(x + y + 1 - s->h);
			float ts = sy / wlen / 2.0f;
			float wc = wing_chord_at(b, ts);
			float w0 = s->w*0.92f + (1.0f - wc) * wcho;
			float w1 = s->w*0.92f + wcho;
			float l0 = (sx - w0) / 1.5f;
			float l1 = (w1 - sx) / 1.5f;
			clamp(l0, 0, 1);
			clamp(l1, 0, 1);
			unsigned char v = 255 * (1.0f - (l0 * l1));
			unsigned char oldv = pget(s, x, y).a;
			if (oldv!=255)
				v = min(64+v*3/4, oldv);
			pset(s, x, y, (atg_colour){0, 0, 0, v});
		}
	/* Render roundels to decal layer */
	unsigned int rx = (s->w*0.92f + wcho * 1.4f - 5) / 2;
	float ryf;
	switch (b->fuse.typ) {
	case FT_GEODETIC:
		ryf = 0.55f;
		break;
	case FT_SLENDER:
		ryf = 0.48f;
		break;
	default:
		ryf = 0.7f;
		break;
	}
	unsigned int ry = wlen * ryf;
	render_roundel(deca, rx - ry, s->h - rx - ry - 1);
	render_roundel(deca, rx + ry, s->h - rx + ry - 1);
}

static float engine_chord_at(float width, float front, float back, float x)
{
	if (x > front)
		return 0;
	if (x > 0)
		return width;
	if (x > back) {
		float t = (back - x) / back;
		return width * powf(1 - (1 - t)*(1 - t), 0.6);
	}
	return 0;
}

static void render_prop(SDL_Surface *feat, unsigned int x, unsigned int y)
{
	pset(feat, x, y, (atg_colour){96, 96, 96, 128});
	pset(feat, x + 1, y + 1, (atg_colour){160, 160, 128, 128});
	pset(feat, x - 1, y - 1, (atg_colour){160, 160, 128, 128});
}

static void render_mini_engines(SDL_Surface *s, const struct bomber *b, SDL_Surface *feat)
{
	/* copied from _wing, needed for proper positioning */
	float wlen = powf(b->wing.span, 0.6f)*0.85f;
	float wcho = wlen / b->wing.ar*4.5f;
	float w1 = s->w*0.92f + wcho;
	/* copied from _fuse, ditto (odd # engines) */
	float flen = 13 + sqrt(b->core_tare / 26.0f);
	float f1 = s->w + flen / 1.5f;
	/* roughly 1 to 1.4 */
	float escale = powf(b->engines.typ->twt / 1e3f, 1/3.0f);
	float ew = escale * (b->engines.typ->art ? 2.1f : 1.4f) + 1;
	float ef = escale * (b->engines.typ->art ? 1.8f : 2.1f) + 1;
	float eb = -escale * (b->engines.typ->art ? 3.4f : 1.9f) - 1;
	bool squeeze = b->engines.number > 5;
	/* render engine nacelles to mask layer */
	for (int x = 0; x < s->w; x++)
		for (int y = 0; y < s->h; y++) {
			unsigned int sx = x + (s->h - y - 1);
			unsigned int sy = abs(x + y + 1 - s->h);
			if (b->engines.odd) {
				float ecat = engine_chord_at(ew, ef, eb, sx + 1 - f1);
				float ev = (ecat - sy) / 1.9f;
				clamp(ev, 0, 1);
				unsigned char v = 255 * (1.0f - ev);
				unsigned char oldv = pget(s, x, y).a;
				if (v<255) {
					v = min(v, oldv);
					pset(s, x, y, (atg_colour){0, 0, 0, v});
				}
			}
			float spot = 1.0f;
			for (int i = 1; i < 5; i++)
				if ((int)b->engines.number >= 2 * i) {
					float ecat = engine_chord_at(ew, ef, eb, sx - w1);
					if (squeeze)
						ecat *= 0.75f;
					float ev = (ecat - abs((int)sy - i * (squeeze ? 4 : 6))) / 1.9f;
					clamp(ev, 0, 1);
					spot *= 1.0f - ev;
				}
			unsigned char v = 255 * spot;
			unsigned char oldv = pget(s, x, y).a;
			if (v<255) {
				v = min(v, 64+oldv*3/4);
				pset(s, x, y, (atg_colour){0, 0, 0, v});
			}
	}
	/* render props to features layer */
	if (b->engines.odd) {
		unsigned int sx = floor((f1 + ef + 0.5f) / 2.0f);
		render_prop(feat, sx, s->h - sx - 1);
	}
	for (int i = 1; i < 5; i++)
		if ((int)b->engines.number >= 2 * i) {
			unsigned int sx = floor((w1 + ef + 1.5f) / 2.0f);
			unsigned int sy = (i * (squeeze ? 4 : 6)) / 2;
			render_prop(feat, sx + sy, s->h - sx + sy - 1);
			render_prop(feat, sx - sy, s->h - sx - sy - 1);
		}
}

static float tail_chord_at(const struct bomber *b, float ts, unsigned int fins)
{
	if (ts < 0 || ts > 1)
		return 0;
	switch (b->fuse.typ) {
	case FT_NORMAL:
	case FT_SLABBY:
		if (fins < 2 && ts > 0.8f)
			return 1.8f - ts;
		return 1;
	case FT_SLENDER:
		if (fins < 2) // this is never true currently
			return 1.4f - ts * 0.4f;
		return 1.5f - ts * 0.7f;
	case FT_GEODETIC: // always fins==1
		return powf(1 - ts*ts, 0.25) * 1.1f;
	default:
		return 1;
	}
}

static void render_mini_tail(SDL_Surface *s, const struct bomber *b)
{
	/* copied from _fuse, needed for proper positioning */
	float flen = 13 + sqrt(b->core_tare / 26.0f);
	float f0 = s->w - flen / 0.75f;
	/* What kind of tail?  Here are the historicals:
	 * Blen SING 2NOR
	 * Whit TWIN 2SLA
	 * Hamp TWIN 2SLE
	 * Wlng SING 2GEO
	 * Manc TRPL 2NOR(H)
	 * Stir SING 4SLA
	 * Hali TWIN 4NOR
	 * Lanc TWIN 4NOR
	 * Mosq SING 2NOR
	 * Wind SING 4GEO
	 * So let's say that heavy NOR/SLE and medium SLE/SLA get TWIN,
	 * except (H) which get TRPL,
	 * otherwise SING.
	 */
	unsigned int fins = 1;
	switch (b->fuse.typ) {
	case FT_NORMAL:
		if (b->engines.heavy)
			fins = (b->engines.typ->hvy <= 2) ? 3 : 2;
		break;
	case FT_SLENDER:
		if (b->engines.heavy && b->engines.typ->hvy <= 2)
			fins = 3;
		else
			fins = 2;
		break;
	case FT_SLABBY:
		if (!b->engines.heavy)
			fins = 2;
		break;
	case FT_GEODETIC:
	default:
		break;
	}
	float tspn = powf(b->wing.span, 0.6f) * 0.28f;
	float tcho = powf(b->core_tare, 0.4f) / 5.0f + 2.0;
	for (int x = 0; x < s->w; x++)
		for (int y = 0; y < s->h; y++) {
			unsigned int sx = x + (s->h - y - 1);
			unsigned int sy = abs(x + y + 1 - s->h);
			float ts = sy / tspn / 2.0f;
			float wc = tail_chord_at(b, ts, fins);
			float w0 = f0 + (1 - wc) * tcho / 2.0;
			float w1 = f0 + (wc + 1) * tcho / 2.0;
			float l0 = (sx - w0) / 1.5f;
			float l1 = (w1 - sx) / 1.5f;
			clamp(l0, 0, 1);
			clamp(l1, 0, 1);
			if (fins > 1) {
				float l2 = 1 - (fabs(ts - 0.7f) * 6.0f);
				clamp(l2, 0, 0.6f);
				l1 *= (1 - l2);
			}
			unsigned char v = 255 * (1.0f - (l0 * l1));
			unsigned char oldv = pget(s, x, y).a;
			if (oldv!=255)
				v = min(64+v*3/4, oldv);
			if (fins & 1 && !sy) {
				float l0 = (sx - w0) / 1.2f;
				float l1 = (w1 + 2 - sx) / 2.0f;
				clamp(l0, 0, 1);
				clamp(l1, 0, 1);
				v = v * (1.0f - l0 * l1 * 0.5f) + 255 * l0 * l1 * 0.5f;
			}
			pset(s, x, y, (atg_colour){0, 0, 0, v});
		}
}

SDL_Surface *bomber_art_mini(const struct bomber *b)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 36, 36, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	SDL_Surface *mask=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 36, 36, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	SDL_Surface *features=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 36, 36, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	SDL_Surface *decals=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 36, 36, 32, 0xff000000, 0xff0000, 0xff00, 0xff);

	SDL_FillRect(rv, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, 0xffffffff);
	SDL_Rect src={.x=36 * (b->slot_idx % 16), .y=0, .w=36, .h=36};
	SDL_Rect dst={.x=0, .y=0, .w=36, .h=36};
	SDL_BlitSurface(builder->camo_small, &src, rv, &dst);
	SDL_FillRect(mask, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	SDL_FillRect(features, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	SDL_FillRect(decals, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	render_mini_fuse(mask, b, features);
	render_mini_wing(mask, b, decals);
	render_mini_engines(mask, b, features);
	render_mini_tail(mask, b);
	SDL_BlitSurface(decals, NULL, rv, NULL);
	SDL_BlitSurface(mask, NULL, rv, NULL);
	SDL_BlitSurface(features, NULL, rv, NULL);
	return rv;
}
