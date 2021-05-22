/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	stats/kills: Extract kill/loss data from game event log
*/

#include <stdio.h>
#include <unistd.h>
#include "bits.h"
#include "globals.h"
#include "hist_record.h"
#include "hashtable.h"
#include "load_data.h"
#include "saving.h"

struct fighter
{
	struct hlist list; // acid is list.hash
	unsigned int ftype;
	dmgsrc src; // see comment in struct dmg_record
	bool dead;
};

struct bomber
{
	struct hlist list; // acid is list.hash
	unsigned int type;
	dmgsrc src; // see comment in struct dmg_record
	bool dead;
};

int main(int argc, char **argv)
{
	struct hist_record rec;
	char *line;
	char outbuf[80];
	game initgame;
	struct hashtable fighters, bombers;
	bool dmg_cleared = true;
#ifndef WINDOWS
	bool localdat = false;
#endif
	char cwd_buf[1024];
	int arg;
	int rc;
	for (arg = 1; arg < argc; arg++)
	{
		if (!strcmp(argv[arg], "--localdat"))
		{
			localdat=true;
		}
		else
		{
			fprintf(stderr, "Unrecognised argument '%s'\n", argv[arg]);
			return(2);
		}
	}

	if(!(cwd=getcwd(cwd_buf, 1024)))
	{
		perror("getcwd");
		return(1);
	}

	fprintf(stderr, "Loading data files...\n");
	
#ifndef WINDOWS
	if(chdir(localdat?cwd:DATIDIR))
	{
		perror("Failed to enter data dir: chdir");
		if(!localdat)
			fprintf(stderr, "(Maybe try with --localdat, or make install)\n");
		return(1);
	}
#endif

	rc = load_data();
	if (rc)
		return(rc);
	rc = set_init_state(&initgame);
	if (rc)
		return(rc);
	
	fprintf(stderr, "Data files loaded\n");

	rc = hashtable_create(&fighters, 2);
	if (rc)
	{
		fprintf(stderr, "Error %d creating hashtable 'fighters'\n", rc);
		return 5;
	}
	rc = hashtable_create(&bombers, 2);
	if (rc)
	{
		fprintf(stderr, "Error %d creating hashtable 'bombers'\n", rc);
		return 5;
	}

	while(!feof(stdin))
	{
		line = fgetl(stdin);
		if (!line)
		{
			fprintf(stderr, "Unexpected EOF\n");
			return 3;
		}
		if (!strncmp(line, "History:", 8))
		{
			free(line);
			break;
		}
	}

	while(!feof(stdin))
	{
		line = fgetl(stdin);
		if (!line) break;
		rc = parse_event(&rec, line);
		if (rc)
		{
			fprintf(stderr, "Error %d in line: %s\n", rc, line);
			free(line);
			return rc;
		}
		free(line);
		if (rec.type == HT_INIT)
		{
			char savbuf[80];
			char *savp = savbuf;

			if (rec.init[0] == '/')
				savp = rec.init;
			else
				snprintf(savbuf, 80, "save/%s", rec.init);
			rc = loadgame(savp, &initgame);
			if (rc)
			{
				fprintf(stderr, "Error %d loading INITgame '%s'\n", rc, savp);
				return 6;
			}
			for (unsigned int i = 0; i < initgame.nbombers; i++)
			{
				struct bomber *b = malloc(sizeof(*b));
				if (!b)
				{
					perror("malloc");
					return 5;
				}
				b->list.hash = initgame.bombers[i].id;
				b->type = initgame.bombers[i].type;
				b->src.ds = DS_NONE;
				b->dead = false;
				hashtable_insert(&bombers, &b->list);
			}
			for (unsigned int i = 0; i < initgame.nfighters; i++)
			{
				struct fighter *f = malloc(sizeof(*f));
				if (!f)
				{
					perror("malloc");
					return 5;
				}
				f->list.hash = initgame.fighters[i].id;
				f->ftype = initgame.fighters[i].type;
				f->src.ds = DS_NONE;
				f->dead = false;
				hashtable_insert(&fighters, &f->list);
			}
			dmg_cleared = true;
		}
		else if (rec.type == HT_AC)
		{
			if (rec.ac.type == AE_CT)
			{
				if (rec.ac.fighter)
				{
					struct fighter *f = malloc(sizeof(*f));
					if (!f)
					{
						perror("malloc");
						return 5;
					}
					f->list.hash = rec.ac.id;
					f->ftype = rec.ac.ac_type;
					f->src.ds = DS_NONE;
					f->dead = false;
					hashtable_insert(&fighters, &f->list);
				}
				else
				{
					struct bomber *b = malloc(sizeof(*b));
					if (!b)
					{
						perror("malloc");
						return 5;
					}
					b->list.hash = rec.ac.id;
					b->type = rec.ac.ac_type;
					b->src.ds = DS_NONE;
					b->dead = false;
					hashtable_insert(&bombers, &b->list);
				}
			}
			else if (rec.ac.type == AE_DMG)
			{
				dmg_cleared = false;
				if (rec.ac.fighter)
				{
					struct hlist *fl = hashtable_lookup(&fighters, rec.ac.id);
					struct fighter *f;

					if (!fl)
					{
						fprintf(stderr, "Warning: encountered unknown fighter %08x\n", rec.ac.id);
						f = malloc(sizeof(*f));
						if (!f)
						{
							perror("malloc");
							return 5;
						}
						f->list.hash = rec.ac.id;
						f->ftype = rec.ac.ac_type;
						f->dead = false;
					}
					else
					{
						f = container_of(fl, struct fighter, list);
					}
					f->src = rec.ac.dmg.src;
				}
				else
				{
					struct hlist *bl = hashtable_lookup(&bombers, rec.ac.id);
					struct bomber *b;

					if (!bl)
					{
						fprintf(stderr, "Warning: encountered unknown bomber %08x\n", rec.ac.id);
						b = malloc(sizeof(*b));
						if (!b)
						{
							perror("malloc");
							return 5;
						}
						b->list.hash = rec.ac.id;
						b->type = rec.ac.ac_type;
						b->dead = false;
					}
					else
					{
						b = container_of(bl, struct bomber, list);
					}
					b->src = rec.ac.dmg.src;
				}
			}
			else if (rec.ac.type == AE_FAIL)
			{
				struct hlist *bl;
				struct bomber *b;

				dmg_cleared = false;
				if (rec.ac.fighter) // can't happen
				{
					writedate(rec.date, outbuf);
					fprintf(stderr, "FA event on a fighter, date=%s, id=%08x\n", outbuf, rec.ac.id);
					return 7;
				}
				bl = hashtable_lookup(&bombers, rec.ac.id);
				if (!bl)
				{
					fprintf(stderr, "Warning: encountered unknown bomber %08x\n", rec.ac.id);
					b = malloc(sizeof(*b));
					if (!b)
					{
						perror("malloc");
						return 5;
					}
					b->list.hash = rec.ac.id;
					b->type = rec.ac.ac_type;
					b->dead = false;
				}
				else
				{
					b = container_of(bl, struct bomber, list);
				}
				if (b->src.ds == DS_NONE)
					b->src.ds = DS_MECH;
			}
			else if (rec.ac.type == AE_CROB && rec.ac.crob.ty == CROB_CR)
			{
				switch (rec.ac.crob.ty)
				{
					case CROB_CR:
						writedate(rec.date, outbuf);
						fputs(outbuf, stdout);
						putchar(' ');
						if (rec.ac.fighter)
						{
							struct hlist *fl = hashtable_lookup(&fighters, rec.ac.id), *bl;
							struct fighter *f;
							struct bomber *b;

							if (!fl)
							{
								fprintf(stderr, "Unknown fighter %08x crashed\n", rec.ac.id);
								return 7;
							}
							f = container_of(fl, struct fighter, list);
							printf("F %u ", f->ftype);
							switch (f->src.ds)
							{
							case DS_FIGHTER:
							case DS_FLAK:
							case DS_TFLK:
								puts("FF"); // friendly-fire, can't happen
								break;
							case DS_MECH: // can't happen
								puts("MECH");
								break;
							case DS_BOMBER:
								bl = hashtable_lookup(&bombers, f->src.idx);
								if (!bl)
								{
									fprintf(stderr, "Fighter %08x killed by unknown bomber %08x\n", rec.ac.id, f->src.idx);
									return 7;
								}
								b = container_of(bl, struct bomber, list);
								printf("BT %u\n", b->type);
								break;
							case DS_NONE: // assume FUEL, since the log doesn't record that directly
							case DS_FUEL:
								puts("FUEL");
								break;
							default: // can't happen
								printf("%u %u\n", f->src.ds, f->src.idx);
							}
							f->dead = true;
						}
						else
						{
							struct hlist *bl = hashtable_lookup(&bombers, rec.ac.id), *fl;
							struct bomber *b;
							struct fighter *f;

							if (!bl)
							{
								fprintf(stderr, "Unknown bomber %08x crashed\n", rec.ac.id);
								return 7;
							}
							b = container_of(bl, struct bomber, list);
							printf("B %u ", b->type);
							switch (b->src.ds) // enum {DS_NONE, DS_FIGHTER, DS_FLAK, DS_TFLK, DS_MECH, DS_BOMBER, DS_FUEL} ds;
							{
							case DS_NONE: // can't happen
								puts("NONE");
								break;
							case DS_FIGHTER:
								fl = hashtable_lookup(&fighters, b->src.idx);
								if (!fl)
								{
									fprintf(stderr, "Bomber %08x killed by unknown fighter %08x\n", rec.ac.id, b->src.idx);
									return 7;
								}
								f = container_of(fl, struct fighter, list);
								printf("FT %u\n", f->ftype);
								break;
							case DS_FLAK:
								printf("FL %u\n", b->src.idx);
								break;
							case DS_TFLK:
								printf("TF %u\n",b->src.idx);
								break;
							case DS_MECH:
								puts("MECH");
								break;
							case DS_BOMBER: // friendly-fire, can't happen
								puts("FF");
								break;
							case DS_FUEL: // can't happen
								puts("FUEL");
								break;
							default: // can't happen
								printf("%u %u\n", b->src.ds, b->src.idx);
							}
							b->dead = true;
						}
						break;
					case CROB_SC:
					case CROB_OB:
						if (rec.ac.fighter)
						{
							struct hlist *fl = hashtable_lookup(&fighters, rec.ac.id);
							struct fighter *f;

							if (!fl)
							{
								fprintf(stderr, "Warning: unknown fighter %08x scrapped\n", rec.ac.id);
							}
							else
							{
								f = container_of(fl, struct fighter, list);
								f->dead = true;
							}
						}
						else
						{
							struct hlist *bl = hashtable_lookup(&bombers, rec.ac.id);
							struct bomber *b;

							if (!bl)
							{
								fprintf(stderr, "Warning: Unknown bomber %08x scrapped\n", rec.ac.id);
							}
							else
							{
								b = container_of(bl, struct bomber, list);
								b->dead = true;
							}
						}
						break;
					default:
						fprintf(stderr, "Unknown crob event type %d\n", rec.ac.crob.ty);
						return 7;
				}
			}
			else if (rec.ac.type == AE_RAID)
			{
				struct hlist *bl;
				struct bomber *b;

				if (rec.ac.fighter) // can't happen
				{
					writedate(rec.date, outbuf);
					fprintf(stderr, "RA event on a fighter, date=%s, id=%08x\n", outbuf, rec.ac.id);
					return 7;
				}
				bl = hashtable_lookup(&bombers, rec.ac.id);
				if (!bl)
				{
					fprintf(stderr, "Warning: encountered unknown bomber %08x\n", rec.ac.id);
					b = malloc(sizeof(*b));
					if (!b)
					{
						perror("malloc");
						return 5;
					}
					b->list.hash = rec.ac.id;
					b->type = rec.ac.ac_type;
					b->dead = false;
				}
				else
				{
					b = container_of(bl, struct bomber, list);
				}
				// clear all damage from this bomber
				b->src.ds = DS_NONE;
				if (!dmg_cleared) // clear all damage from all fighters
				{
					struct hlist *bucket, *fl;
					struct fighter *f;
					unsigned int bi;

					FOR_ALL_HASH_BUCKETS(&fighters, bucket, bi)
						FOR_ALL_BUCKET_ITEMS(bucket, fl)
						{
							f = container_of(fl, struct fighter, list);
							f->src.ds = DS_NONE;
						}
					dmg_cleared = true;
				}
			}
		}
	}
	return 0;
}
