/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	hist_record: parse history events from a game event log (for stats purposes)
*/

#include "types.h"
#include "date.h"

/* Event classes start here */

/* Aircraft events */
// <ct-ev>  ::= CT
// no data to store

// <nav-ev> ::= NA <navaid:int> # installation of a navaid (or, for fighters, radar)
struct nav_record
{
	unsigned int navaid;
};

// <pff-ev> ::= PF # assignment to PFF - not reversible
// no data to store

// <raid-ev>::= RA <targ-id:int> <ra-crew>
// <ra-crew>::= <cm-uid> [<ra-crew>]
struct raid_record
{
	unsigned int targ;
	unsigned int ncrew; // if 0, then the crew just weren't stored (there must have been _someone_ flying it...)
	cmid crew[MAX_CREW];
};

// <hit-ev> ::= HI <targ-id:int> <bmb:int> # bmb is either lb of bombs, lb of mines, or # of leaflets.  Need targ-id as we might have hit a different target to the one we were aiming for
struct hit_record
{
	unsigned int targ;
	unsigned int bmb;
};

// <dmg-ev> ::= DM <delta-dmg:float> <current-dmg:float> (AC <from:ac-uid> | FK <flak-id:int> | TF <targ-d:int>)
struct dmg_record
{
	double delta, current;
	/* For DS_FIGHTER we store the acid in src.idx, whereas a dmgsrc in the game stores
	 * the index into game.fighters.  Similarly, for DS_BOMBER, we store the acid, but
	 * the game stores the index into game.bombers.
	 */
	dmgsrc src;
};

// <fail-ev>::= FA <failstate:int> # is really a bool
struct fail_record
{
	bool state;
};

// <crsh-ev>::= CR
// <obs-ev> ::= OB
struct crob_record
{
	bool ob;
};

/* <data>   ::= A <ac-uid> <b-or-f><type:int> <a-event> | ...
 * <a-event>::= <ct-ev> | <nav-ev> | <pff-ev> | <raid-ev> | <hit-ev> | <dmg-ev> | <fail-ev> | <crsh-ev> | <obs-ev>
 */
enum acev_type
{
	AE_CT,
	AE_NAV,
	AE_PFF,
	AE_RAID,
	AE_HIT,
	AE_DMG,
	AE_FAIL,
	AE_CROB,

	ACEV_TYPES
};
struct ac_record
{
	acid id;
	bool fighter;
	unsigned int ac_type;
	enum acev_type type;
	union
	{
		/* no data for CT */
		struct nav_record nav;
		/* no data for PF */
		struct raid_record raid;
		struct hit_record hit;
		struct dmg_record dmg;
		struct fail_record fail;
		struct crob_record crob;
	};
};

/* Target events */
// <t-dmg>  ::= DM <delta-dmg:float> <current-dmg:float>
struct tdm_record
{
	double delta, current;
};

// <t-flk>  ::= FK <delta-flk:float> <current-flk:float>
struct tfk_record
{
	double delta, current;
};

// <t-ship> ::= SH
// no data to store

/* <data>   ::= ... | T <targ-id:int> <t-event>
 * <t-event>::= <t-dmg> | <t-flk> | <t-ship>
 */
enum targev_type
{
	TE_DMG,
	TE_FLAK,
	TE_SHIP,

	TARGEV_TYPES
};
struct targ_record
{
	unsigned int id;
	enum targev_type type;
	union
	{
		struct tdm_record tdm;
		struct tfk_record tfk;
		/* no data for T SH */
	};
};

/* Misc. events */
// <cash-ev>::= CA <delta-cash:int> <current-cash:int> # Only for Budget payments, as CT costs are known
struct cash_record
{
	int delta, current;
};

// <conf-ev>::= CO <confid:float>
struct confid_record
{
	double confid;
};

// <mora-ev>::= MO <morale:float>
struct morale_record
{
	double morale;
};

// <gprd-ev>::= GP <iclass:int> <gprod:float> <dprod:float> # dprod is _not_ the difference from the previous gprod!
struct gprod_record
{
	enum i_class iclass;
	double gprod, dprod;
};

// <tpri-ev>::= TP <tclass:int> <ignore:int> # really a bool
struct tprio_record
{
	enum t_class tclass;
	bool ignore;
};

// <ipri-ev>::= IP <iclass:int> <ignore:int> # really a bool
struct iprio_record
{
	enum i_class iclass;
	bool ignore;
};

// <prop-ev>::= PG <event:int>
struct prop_record
{
	int event;
};

/* <data>   ::= ... | M <misc-ev>
 * <misc-ev>::= <cash-ev> | <conf-ev> | <mora-ev> | <gprd-ev> | <prio-ev> | <prop-ev>
 * <prio-ev>::= <tpri-ev> | <ipri-ev>
 */
enum miscev_type
{
	ME_CASH,
	ME_CONFID,
	ME_MORALE,
	ME_GPROD,
	ME_TPRIO,
	ME_IPRIO,
	ME_PROP,

	MISCEV_TYPES
};
struct misc_record
{
	enum miscev_type type;
	union
	{
		struct cash_record cash;
		struct confid_record confid;
		struct morale_record morale;
		struct gprod_record gprod;
		struct tprio_record tprio;
		struct iprio_record iprio;
		struct prop_record prop;
	};
};

/* Crew events */
// <gen-ev> ::= GE <lrate:int> # generation of a new (student) crewman
struct gen_record
{
	unsigned int lrate;
};

// <ski-ev> ::= SK <skill:int> # reached a new integer-part of skill
struct skill_record
{
	unsigned int skill;
};

/* <csta-ev>::= ST <cstatus> # status change (see also EX)
 * <cstatus>::= C | S | I # never have E as that's an <escp-ev> rather than a <csta-ev>
 */
struct csta_record
{
	enum cstatus status;
};

// <op-ev>  ::= OP <tops:int> # completed an op (*not* generated daily for instructors!)
struct op_record
{
	unsigned int tops;
};

// <deth-ev>::= DE | PW
struct death_record
{
	bool pw;
};

// <escp-ev>::= EX <rtime:int> # number of days until return
struct esc_record
{
	int rtime;
};

/* <data>   ::= ... | C <cm-uid> <cclass> <crew-ev>
 * <crew-ev>::= <gen-ev> | <ski-ev> | <csta-ev> | <op-ev> | <deth-ev> | <escp-ev>
 */
enum crewev_type
{
	CE_GEN,
	CE_SKILL,
	CE_STATUS,
	CE_OP,
	CE_DEATH, // Includes PW
	CE_ESC,

	CREWEV_TYPES
};
struct crew_record
{
	cmid id;
	enum cclass cclass;
	enum crewev_type type;
	union
	{
		struct gen_record gen;
		struct skill_record skill;
		struct csta_record csta;
		struct op_record op;
		struct death_record death;
		struct esc_record esc;
	};
};

/* Overall event container */

/* <record> ::= <date> <time> <data>
 * <data>   ::= A <ac-uid> <b-or-f><type:int> <a-event> | T <targ-id:int> <t-event> | I <init-ev> | M <misc-ev> | C <cm-uid> <cclass> <crew-ev>
 * <init-ev>::= INIT <filename:str> # Never generated, but should appear in all startpoints (*.sav.in)
 */
enum hist_type
{
	HT_INIT,
	HT_AC,
	HT_TARG,
	HT_MISC,
	HT_CREW,

	HIST_TYPES
};
struct hist_record
{
	date date;
	harris_time time;
	enum hist_type type;
	union
	{
		char *init; // filename
		struct ac_record ac;
		struct targ_record targ;
		struct misc_record misc;
		struct crew_record crew;
	};
};

int parse_event(struct hist_record *rec, char *line);
