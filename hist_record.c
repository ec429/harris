/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	hist_record: parse history events from a game event log (for stats purposes)
*/

#include "hist_record.h"
#include <string.h>
#include "bits.h"
#include "date.h"
#include "saving.h"

/* Copied from date.c, which we can't link against because of indirect dependency on some globals.
 * This is arguably wrong, but difficult to fix for reasons too complicated to explain here.
 */
date readdate(const char *t, date nulldate)
{
	date d;
	if(t&&*t&&(sscanf(t, "%u-%u-%d", &d.day, &d.month, &d.year)==3)) return(d);
	return(nulldate);
}

harris_time readtime(const char *text, harris_time nulltime)
{
	harris_time t;
	if(text&&*text&&(sscanf(text, "%u:%u", &t.hour, &t.minute)==2)) return(t);
	return(nulltime);
}

/* Copied from date.c, which we can't link against because of indirect dependency on some globals.
 * This is arguably wrong, but difficult to fix for reasons too complicated to explain here.
 */
size_t writedate(date when, char *buf)
{
	if(!buf) return(0);
	return(sprintf(buf, "%02u-%02u-%04d", when.day, when.month, when.year));
}

size_t writetime(harris_time when, char *buf)
{
	if(!buf) return(0);
	return(sprintf(buf, "%02u:%02u", when.hour, when.minute));
}

/* Get next word from line, or complain we didn't get one */
#define WORD_OR_ERR(_what)	if (!(word = strtok(NULL, " "))) { \
					fprintf(stderr, "Expected " _what ", got EOL\n"); \
					return 1; \
				}

/* <nav-ev> ::= NA <navaid:int> # installation of a navaid (or, for fighters, radar) */
int parse_nav(struct nav_record *rec)
{
	char *word;

	WORD_OR_ERR("navaid");
	if (sscanf(word, "%u", &rec->navaid) != 1)
	{
		fprintf(stderr, "bad navaid '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <raid-ev>::= RA <targ-id:int> <ra-crew>
 * <ra-crew>::= <cm-uid> [<ra-crew>]
 */
int parse_raid(struct raid_record *rec)
{
	char *word;

	WORD_OR_ERR("targ-id");
	if (sscanf(word, "%u", &rec->targ) != 1)
	{
		fprintf(stderr, "bad targ-id '%s'\n", word);
		return 2;
	}
	for (rec->ncrew = 0; rec->ncrew <= MAX_CREW; rec->ncrew++)
	{
		word = strtok(NULL, " ");
		if (!word)
			break;
		if (gcmid(word, &rec->crew[rec->ncrew]))
		{
			fprintf(stderr, "bad C/M ID '%s'\n", word);
			return 2;
		}
	}
	return 0;
}

/* <hit-ev> ::= HI <targ-id:int> <bmb:int> # bmb is either lb of bombs, lb of mines, or # of leaflets. */
int parse_hit(struct hit_record *rec)
{
	char *word;

	WORD_OR_ERR("targ-id");
	if (sscanf(word, "%u", &rec->targ) != 1)
	{
		fprintf(stderr, "bad targ-id '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("bmb");
	if (sscanf(word, "%u", &rec->bmb) != 1)
	{
		fprintf(stderr, "bad bmb '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <dmg-ev> ::= DM <delta-dmg:float> <current-dmg:float> (AC <from:ac-uid> | FK <flak-id:int> | TF <targ-d:int>) */
int parse_dmg(struct dmg_record *rec)
{
	char *word;

	WORD_OR_ERR("delta-dmg");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->delta) != 1)
	{
		fprintf(stderr, "bad delta-dmg '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("current-dmg");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->current) != 1)
	{
		fprintf(stderr, "bad current-dmg '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("source type");
	if (!strcmp(word, "AC"))
	{
		bool ftr = container_of(rec, struct ac_record, dmg)->fighter;
		rec->src.ds = ftr ? DS_BOMBER : DS_FIGHTER;
		WORD_OR_ERR("source ID");
		if (gacid(word, &rec->src.idx))
		{
			fprintf(stderr, "bad damage source index '%s'\n", word);
			return 2;
		}
		return 0;
	}
	else if (!strcmp(word, "FK"))
	{
		rec->src.ds = DS_FLAK;
	}
	else if (!strcmp(word, "TF"))
	{
		rec->src.ds = DS_TFLK;
	}
	else
	{
		fprintf(stderr, "bad damage source type '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("source index");
	if (sscanf(word, "%u", &rec->src.idx) != 1)
	{
		fprintf(stderr, "bad damage source index '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <fail-ev>::= FA <failstate:int> # is really a bool */
int parse_fail(struct fail_record *rec)
{
	char *word;
	unsigned int fs;

	WORD_OR_ERR("failstate");
	if (sscanf(word, "%u", &fs) != 1)
	{
		fprintf(stderr, "bad failstate '%s'\n", word);
		return 2;
	}
	if (fs > 1)
		fprintf(stderr, "Warning: weird failstate %u (expect 0 or 1)\n", fs);
	rec->state = !!fs;
	return 0;
}

/* <crsh-ev>::= CR
 * <obs-ev> ::= OB
 */

/* <data>   ::= A <ac-uid> <b-or-f><type:int> <a-event> | ...
 * <ac-uid> ::= <hex>*8 # randomly generated token.  P(match) = 2^-32 ~= 1/4.2e9
 * <b-or-f> ::= B | F
 * <cclass> ::= P | N | B | E | W | G
 * <a-event>::= <ct-ev> | <nav-ev> | <pff-ev> | <raid-ev> | <hit-ev> | <dmg-ev> | <fail-ev> | <crsh-ev> | <obs-ev>
 */
int parse_ac(struct ac_record *rec)
{
	char *word;

	WORD_OR_ERR("A/C ID");
	if (gacid(word, &rec->id))
	{
		fprintf(stderr, "bad A/C ID '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("type");
	if (word[0] == 'B')
	{
		rec->fighter = false;
	}
	else if (word[0] == 'F')
	{
		rec->fighter = true;
	}
	else
	{
		fprintf(stderr, "bad <b-or-f> '%s'\n", word);
		return 2;
	}
	if (sscanf(word+1, "%u", &rec->ac_type) != 1)
	{
		fprintf(stderr, "bad A/C type '%s'\n", word+1);
		return 2;
	}
	WORD_OR_ERR("A-event class");
	if (!strcmp(word, "CT"))
	{
		rec->type = AE_CT;
		return 0;
	}
	if (!strcmp(word, "NA"))
	{
		rec->type = AE_NAV;
		return parse_nav(&rec->nav);
	}
	if (!strcmp(word, "PF"))
	{
		rec->type = AE_PFF;
		return 0;
	}
	if (!strcmp(word, "RA"))
	{
		rec->type = AE_RAID;
		return parse_raid(&rec->raid);
	}
	if (!strcmp(word, "HI"))
	{
		rec->type = AE_HIT;
		return parse_hit(&rec->hit);
	}
	if (!strcmp(word, "DM"))
	{
		rec->type = AE_HIT;
		return parse_dmg(&rec->dmg);
	}
	if (!strcmp(word, "FA"))
	{
		rec->type = AE_FAIL;
		return parse_fail(&rec->fail);
	}
	if (!strcmp(word, "CR"))
	{
		rec->type = AE_CROB;
		rec->crob.ob = false;
		return 0;
	}
	if (!strcmp(word, "OB"))
	{
		rec->type = AE_CROB;
		rec->crob.ob = true;
		return 0;
	}
	fprintf(stderr, "Unrecognised A-event class '%s'\n", word);
	return 2;
}

/* <t-dmg>  ::= DM <delta-dmg:float> <current-dmg:float> */
int parse_tdm(struct tdm_record *rec)
{
	char *word;

	WORD_OR_ERR("delta-dmg");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->delta) != 1)
	{
		fprintf(stderr, "bad delta-dmg '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("current-dmg");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->current) != 1)
	{
		fprintf(stderr, "bad current-dmg '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <t-flk>  ::= FK <delta-flk:float> <current-flk:float> */
int parse_tfk(struct tfk_record *rec)
{
	char *word;

	WORD_OR_ERR("delta-flk");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->delta) != 1)
	{
		fprintf(stderr, "bad delta-flk '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("current-flk");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->current) != 1)
	{
		fprintf(stderr, "bad current-flk '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <data>   ::= ... | T <targ-id:int> <t-event>
 * <t-event>::= <t-dmg> | <t-flk> | <t-ship>
 */
int parse_targ(struct targ_record *rec)
{
	char *word;

	WORD_OR_ERR("targ-id");
	if (sscanf(word, "%u", &rec->id) != 1)
	{
		fprintf(stderr, "bad targ-id '%s'\n", word);
		return 2;
	}

	WORD_OR_ERR("T-event class");
	if (!strcmp(word, "DM"))
	{
		rec->type = TE_DMG;
		return parse_tdm(&rec->tdm);
	}
	if (!strcmp(word, "FK"))
	{
		rec->type = TE_FLAK;
		return parse_tfk(&rec->tfk);
	}
	if (!strcmp(word, "SH"))
	{
		rec->type = TE_SHIP;
		return 0;
	}
	fprintf(stderr, "Unrecognised T-event class '%s'\n", word);
	return 2;
}

/* <cash-ev>::= CA <delta-cash:int> <current-cash:int> # Only for Budget payments, as CT costs are known */
int parse_cash(struct cash_record *rec)
{
	char *word;

	WORD_OR_ERR("delta-cash");
	if (sscanf(word, "%d", &rec->delta) != 1)
	{
		fprintf(stderr, "bad delta-cash '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("current-cash");
	if (sscanf(word, "%d", &rec->current) != 1)
	{
		fprintf(stderr, "bad current-cash '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <conf-ev>::= CO <confid:float> */
int parse_confid(struct confid_record *rec)
{
	char *word;

	WORD_OR_ERR("confid");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->confid) != 1)
	{
		fprintf(stderr, "bad confid '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <mora-ev>::= MO <morale:float> */
int parse_morale(struct morale_record *rec)
{
	char *word;

	WORD_OR_ERR("morale");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->morale) != 1)
	{
		fprintf(stderr, "bad morale '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <gprd-ev>::= GP <iclass:int> <gprod:float> <dprod:float> # dprod is _not_ the difference from the previous gprod! */
int parse_gprod(struct gprod_record *rec)
{
	char *word;

	WORD_OR_ERR("iclass");
	if (sscanf(word, "%u", &rec->iclass) != 1)
	{
		fprintf(stderr, "bad iclass '%s'\n", word);
		return 2;
	}
	if (rec->iclass < 0 || rec->iclass >= ICLASS_MIXED)
	{
		fprintf(stderr, "bad iclass '%d', expected 0 to %d\n", rec->iclass, ICLASS_MIXED - 1);
		return 2;
	}
	WORD_OR_ERR("gprod");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->gprod) != 1)
	{
		fprintf(stderr, "bad gprod '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("dprod");
	if (sscanf(word, FLOAT, CAST_FLOAT_PTR &rec->dprod) != 1)
	{
		fprintf(stderr, "bad dprod '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <misc-ev>::= <cash-ev> | <conf-ev> | <mora-ev> | <gprd-ev> */
int parse_misc(struct misc_record *rec)
{
	char *word;

	WORD_OR_ERR("M-event class");
	if (!strcmp(word, "CA"))
	{
		rec->type = ME_CASH;
		return parse_cash(&rec->cash);
	}
	if (!strcmp(word, "CO"))
	{
		rec->type = ME_CONFID;
		return parse_confid(&rec->confid);
	}
	if (!strcmp(word, "MO"))
	{
		rec->type = ME_MORALE;
		return parse_morale(&rec->morale);
	}
	if (!strcmp(word, "GP"))
	{
		rec->type = ME_GPROD;
		return parse_gprod(&rec->gprod);
	}
	fprintf(stderr, "Unrecognised M-event class '%s'\n", word);
	return 2;
}

/* <gen-ev> ::= GE <lrate:int> # generation of a new (student) crewman */
int parse_gen(struct gen_record *rec)
{
	char *word;

	WORD_OR_ERR("lrate");
	if (sscanf(word, "%u", &rec->lrate) != 1)
	{
		fprintf(stderr, "bad lrate '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <ski-ev> ::= SK <skill:int> # reached a new integer-part of skill */
int parse_skill(struct skill_record *rec)
{
	char *word;

	WORD_OR_ERR("skill");
	if (sscanf(word, "%u", &rec->skill) != 1)
	{
		fprintf(stderr, "bad skill '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <csta-ev>::= ST <cstatus> # status change (see also EX)
 * <cstatus>::= C | S | I # never have E as that's an <escp-ev> rather than a <csta-ev>
 */
int parse_csta(struct csta_record *rec)
{
	char *word;

	WORD_OR_ERR("cstatus");
	if (strlen(word) != 1)
	{
		fprintf(stderr, "bad cstatus '%s'\n", word);
		return 2;
	}
	switch (*word)
	{
	case 'C':
		rec->status = CSTATUS_CREWMAN;
		return 0;
	case 'S':
		rec->status = CSTATUS_STUDENT;
		return 0;
	case 'I':
		rec->status = CSTATUS_INSTRUC;
		return 0;
	default:
		fprintf(stderr, "bad cstatus '%s'\n", word);
		return 2;
	}
}

/* <op-ev>  ::= OP <tops:int> # completed an op (*not* generated daily for instructors!) */
int parse_op(struct op_record *rec)
{
	char *word;

	WORD_OR_ERR("tops");
	if (sscanf(word, "%u", &rec->tops) != 1)
	{
		fprintf(stderr, "bad tops '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <escp-ev>::= EX <rtime:int> # number of days until return */
int parse_esc(struct esc_record *rec)
{
	char *word;

	WORD_OR_ERR("rtime");
	if (sscanf(word, "%d", &rec->rtime) != 1)
	{
		fprintf(stderr, "bad rtime '%s'\n", word);
		return 2;
	}
	return 0;
}

/* <data>   ::= ... | C <cm-uid> <cclass> <crew-ev>
 * <crew-ev>::= <gen-ev> | <ski-ev> | <csta-ev> | <op-ev> | <deth-ev> | <escp-ev>
 */
int parse_crew(struct crew_record *rec)
{
	char *word;

	WORD_OR_ERR("C/M ID");
	if (gcmid(word, &rec->id))
	{
		fprintf(stderr, "bad C/M ID '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("cclass");
	if (strlen(word) != 1)
	{
		fprintf(stderr, "bad cclass '%s'\n", word);
		return 2;
	}
	if ((rec->cclass = lookup_crew_letter(*word)) == CCLASS_NONE)
	{
		fprintf(stderr, "bad cclass '%s'\n", word);
		return 2;
	}
	WORD_OR_ERR("C-event class");
	if (!strcmp(word, "GE"))
	{
		rec->type = CE_GEN;
		return parse_gen(&rec->gen);
	}
	if (!strcmp(word, "SK"))
	{
		rec->type = CE_SKILL;
		return parse_skill(&rec->skill);
	}
	if (!strcmp(word, "ST"))
	{
		rec->type = CE_STATUS;
		return parse_csta(&rec->csta);
	}
	if (!strcmp(word, "OP"))
	{
		rec->type = CE_OP;
		return parse_op(&rec->op);
	}
	if (!strcmp(word, "DE"))
	{
		rec->type = CE_DEATH;
		rec->death.pw = false;
		return 0;
	}
	if (!strcmp(word, "PW"))
	{
		rec->type = CE_DEATH;
		rec->death.pw = true;
		return 0;
	}
	if (!strcmp(word, "EX"))
	{
		rec->type = CE_ESC;
		return parse_esc(&rec->esc);
	}
	fprintf(stderr, "Unrecognised C-event class '%s'\n", word);
	return 2;
}

// <init-ev>::= INIT <filename:str> # Never generated, but should appear in all startpoints (*.sav.in)
int parse_init(char **rec)
{
	char *word;

	WORD_OR_ERR("INIT");
	if (strcmp(word, "INIT"))
	{
		fprintf(stderr, "Expected INIT, got '%s'\n", word);
		return 2;
	}
	*rec = strtok(NULL, "");
	return 0;
}

/* <record> ::= <date> <time> <data>
 * <data>   ::= A <ac-uid> <b-or-f><type:int> <a-event> | T <targ-id:int> <t-event> | I <init-ev> | M <misc-ev> | C <cm-uid> <cclass> <crew-ev>
 * <date>   ::= <day>-<month>-<year> # as save file; times < 12:00 are saved under the previous day
 * <time>   ::= <hh>:<mm> # raids cannot begin before 18:00; post-raid is 11:00 thru 11:59
 */
int parse_event(struct hist_record *rec, char *line)
{
	char *copy = strdup(line);
	char *word;

	if (!(word = strtok(copy, " ")))
	{
		fprintf(stderr, "Expected date, got EOL\n");
		return 1;
	}
	rec->date = readdate(word, (date){0, 0, 0});
	WORD_OR_ERR("time");
	rec->time = readtime(word, (harris_time){0, 0});
	WORD_OR_ERR("event class");
	if (!strcmp(word, "I"))
	{
		rec->type = HT_INIT;
		return parse_init(&rec->init);
	}
	if (!strcmp(word, "A"))
	{
		rec->type = HT_AC;
		return parse_ac(&rec->ac);
	}
	if (!strcmp(word, "T"))
	{
		rec->type = HT_TARG;
		return parse_targ(&rec->targ);
	}
	if (!strcmp(word, "M"))
	{
		rec->type = HT_MISC;
		return parse_misc(&rec->misc);
	}
	if (!strcmp(word, "C"))
	{
		rec->type = HT_CREW;
		return parse_crew(&rec->crew);
	}
	fprintf(stderr, "Unrecognised event class '%s'\n", word);
	return 2;
}
