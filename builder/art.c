#include <math.h>

#include "art.h"
#include "../render.h"
#include "../rand.h"

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
			return (1 - t) * 17.0f;
		if (t > 0.56f)
			return 1.7;
		if (t > 0.44f)
			return t * 8.0f - 2.78f;
		if (t > 0.1f)
			return t + 0.3f;
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

static void render_mini_fuse(SDL_Surface *s, const struct bomber *b)
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
			if (sy < fw - 1.5)
				v = 0;
			else if (sy < fw)
				v = min(170 * (sy + 1.5 - fw), 255);
			pset(s, x, y, (atg_colour){0, 0, 0, v});
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
			return 3.3f - ts * 3;
		if (ts > 0.2f)
			return 0.51f + (0.93f - ts) * 0.49f / 0.73f;
		return 1.5f - 2.5f * ts;
	case FT_SLABBY:
		return powf(1 - ts, 0.1);
	case FT_GEODETIC:
		return powf(1 - ts*ts, 0.4) * 1.2f;
	default:
		return 1;
	}
}

static void render_mini_wing(SDL_Surface *s, const struct bomber *b)
{
	/* Show about 10px for a Blen (56ft), max 15px (120ft?) */
	float wlen = powf(b->wing.span, 0.6f)*0.85f;
	float wcho = wlen / b->wing.ar*4.5f;
	for (int x = 0; x < s->w; x++)
		for (int y = 0; y < s->h; y++) {
			unsigned int sx = x + (s->h - y - 1);
			unsigned int sy = abs(x + y + 1 - s->h);
			float ts = sy / wlen / 2.0f;
			float wc = wing_chord_at(b, ts);
			float w0 = s->w*0.92f + (1.0f - wc) * wcho;
			float w1 = s->w*0.92f + wcho;
			float l0 = (sx - w0) / 1.5;
			float l1 = (w1 - sx) / 1.5;
			clamp(l0, 0, 1);
			clamp(l1, 0, 1);
			unsigned char v = 255 * (1.0f - (l0 * l1));
			unsigned char oldv = pget(s, x, y).a;
			if (oldv!=255)
				v = min(64+v*3/4, oldv);
			pset(s, x, y, (atg_colour){0, 0, 0, v});
		}
}

SDL_Surface *bomber_art_mini(const struct bomber *b)
{
	SDL_Surface *rv=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 36, 36, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	SDL_Surface *mask=SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCALPHA, 36, 36, 32, 0xff000000, 0xff0000, 0xff00, 0xff);

	render_camo(rv, 3, (atg_colour){31, 94, 12, ATG_ALPHA_OPAQUE}, (atg_colour){80, 50, 10, ATG_ALPHA_OPAQUE});
	//(atg_colour){255,255,255,ATG_ALPHA_OPAQUE}, (atg_colour){255,255,255,ATG_ALPHA_OPAQUE});
	//(atg_colour){31, 94, 12, ATG_ALPHA_OPAQUE}, (atg_colour){80, 50, 10, ATG_ALPHA_OPAQUE});
	SDL_FillRect(mask, &(SDL_Rect){.x=0, .y=0, .w=rv->w, .h=rv->h}, ATG_ALPHA_TRANSPARENT&0xff);
	render_mini_fuse(mask, b);
	render_mini_wing(mask, b);
	SDL_BlitSurface(mask, NULL, rv, NULL);
	return rv;
}
