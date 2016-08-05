/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	history: logging of game events
*/
#include "history.h"
#include <stdlib.h>
#include "bits.h"
#include "date.h"
#include "saving.h"

char *hist_alloc(history *hist)
{
	size_t n=hist->nents++;
	if(hist->nents>hist->nalloc)
	{
		size_t ns=hist->nalloc+1024;
		char (*new)[HIST_LINE]=realloc(hist->ents, ns*sizeof(char [HIST_LINE]));
		if(!new)
		{
			hist->nents--;
			return(NULL);
		}
		hist->nalloc=ns;
		hist->ents=new;
	}
	return(hist->ents[n]);
}

int hist_append(history *hist, const char line[HIST_LINE])
{
	char *p=hist_alloc(hist);
	if(!p) return(1);
	memcpy(p, line, HIST_LINE);
	return(0);
}

int hist_clear(history *hist)
{
	if(!hist) return(1);
	free(hist->ents);
	hist->ents=NULL;
	hist->nents=0;
	hist->nalloc=0;
	return(0);
}

int hist_save(history hist, FILE *out)
{
	if(!out) return(1);
	#ifdef WINDOWS
	fprintf(out, "History:%lu\n", (unsigned long)hist.nents);
	#else
	fprintf(out, "History:%zu\n", hist.nents);
	#endif
	for(size_t i=0;i<hist.nents;i++)
	{
		char *line=hist.ents[i];
		if(strchr(line, '\n')||strchr(line, '\\'))
		{
			while(*line)
			{
				switch(*line)
				{
					case '\n':
						fputs("\\n", out);
					break;
					case '\\':
						fputs("\\\\", out);
					break;
					default:
						fputc(*line, out);
				}
				line++;
			}
			fputc('\n', out);
		}
		else
			fprintf(out, "%s\n", line);
	}
	return(0);
}

int hist_load(FILE *in, size_t nents, history *hist)
{
	char line[HIST_LINE];
	if(!in) return(1);
	if(!hist) return(1);
	int rc=hist_clear(hist);
	if(rc) return rc;
	for(size_t i=0;i<nents;i++)
	{
		if(!fgets(line, HIST_LINE, in))
			return(1);
		size_t l=strlen(line);
		if(line[l-1]=='\n') line[l-1]=0;
		if(strchr(line, '\\'))
		{
			char line2[HIST_LINE];
			char *d_in=line, *d_out=line2;
			while(*d_in)
			{
				if(*d_in=='\\')
					switch(*++d_in)
					{
						case 'n':
							*d_out++='\n';
						break;
						default:
							*d_out++=*d_in;
					}
				else
					*d_out+=*d_in;
				d_in++;
			}
			if(hist_append(hist, line2))
				return(3);
		}
		else if(hist_append(hist, line))
			return(3);
	}
	return(0);
}

int ev_append(history *hist, date d, harris_time t, const char *ev)
{
	char *p=hist_alloc(hist);
	if(!p) return(1);
	size_t i=0;
	i+=writedate(d, p+i);
	p[i++]=' ';
	i+=sprintf(p+i, "%02u:%02u ", t.hour, t.minute);
	strncpy(p+i, ev, HIST_LINE-i);
	return(0);
}

int eva_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, const char *ev)
{
	char buf[HIST_LINE];
	size_t i=sprintf(buf, "A ");
	pacid(id, buf+i);i+=8;
	buf[i++]=' ';
	buf[i++]=ftr?'F':'B';
	i+=sprintf(buf+i, "%d ", type);
	strncpy(buf+i, ev, HIST_LINE-i);
	return(ev_append(hist, d, t, buf));
}

int ct_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type)
{
	return(eva_append(hist, d, t, id, ftr, type, "CT"));
}

int na_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, unsigned int nid)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "NA %u", nid);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int pf_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type)
{
	return(eva_append(hist, d, t, id, ftr, type, "PF"));
}

int ra_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, unsigned int tid)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "RA %u", tid);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int hi_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, unsigned int tid, unsigned int bmb)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "HI %u %u", tid, bmb);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int dmac_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, acid src)
{
	char buf[HIST_LINE];
	size_t i=snprintf(buf, 72, "DM "FLOAT" "FLOAT" AC ", CAST_FLOAT ddmg, CAST_FLOAT cdmg);
	pacid(src, buf+i);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int dmfk_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, unsigned int fid)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "DM "FLOAT" "FLOAT" FK %u", CAST_FLOAT ddmg, CAST_FLOAT cdmg, fid);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int dmtf_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, unsigned int tid)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "DM "FLOAT" "FLOAT" TF %u", CAST_FLOAT ddmg, CAST_FLOAT cdmg, tid);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int fa_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type, unsigned int fa)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "FA %u", fa);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int cr_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type)
{
	return(eva_append(hist, d, t, id, ftr, type, "CR"));
}

int ob_append(history *hist, date d, harris_time t, acid id, bool ftr, unsigned int type)
{
	return(eva_append(hist, d, t, id, ftr, type, "OB"));
}

int evt_append(history *hist, date d, harris_time t, unsigned int tid, const char *ev)
{
	char buf[HIST_LINE];
	size_t i=snprintf(buf, HIST_LINE, "T %u ", tid);
	strncpy(buf+i, ev, HIST_LINE-i);
	return(ev_append(hist, d, t, buf));
}

int tdm_append(history *hist, date d, harris_time t, unsigned int tid, double ddmg, double cdmg)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "DM "FLOAT" "FLOAT, CAST_FLOAT ddmg, CAST_FLOAT cdmg);
	return(evt_append(hist, d, t, tid, buf));
}

int tfk_append(history *hist, date d, harris_time t, unsigned int tid, double dflk, double cflk)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "FK "FLOAT" "FLOAT, CAST_FLOAT dflk, CAST_FLOAT cflk);
	return(evt_append(hist, d, t, tid, buf));
}

int tsh_append(history *hist, date d, harris_time t, unsigned int tid)
{
	return(evt_append(hist, d, t, tid, "SH"));
}

int evm_append(history *hist, date d, harris_time t, const char *ev)
{
	char buf[HIST_LINE];
	size_t i=sprintf(buf, "M ");
	strncpy(buf+i, ev, HIST_LINE-i);
	return(ev_append(hist, d, t, buf));
}

int ca_append(history *hist, date d, harris_time t, unsigned int cshr, unsigned int cash)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "CA %u %u", cshr, cash);
	return(evm_append(hist, d, t, buf));
}

int co_append(history *hist, date d, harris_time t, double confid)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "CO "FLOAT, CAST_FLOAT confid);
	return(evm_append(hist, d, t, buf));
}

int mo_append(history *hist, date d, harris_time t, double morale)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "MO "FLOAT, CAST_FLOAT morale);
	return(evm_append(hist, d, t, buf));
}

int gp_append(history *hist, date d, harris_time t, unsigned int iclass, double gprod, double dprod)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "GP %u "FLOAT" "FLOAT, iclass, CAST_FLOAT gprod, CAST_FLOAT dprod);
	return(evm_append(hist, d, t, buf));
}

int tp_append(history *hist, date d, harris_time t, enum t_class cls, bool ignore)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "TP %u %u", cls, ignore?1:0);
	return(evm_append(hist, d, t, buf));
}

int ip_append(history *hist, date d, harris_time t, enum i_class cls, bool ignore)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "IP %u %u", cls, ignore?1:0);
	return(evm_append(hist, d, t, buf));
}

int evc_append(history *hist, date d, harris_time t, cmid id, enum cclass cls, const char *ev)
{
	char buf[HIST_LINE];
	size_t i=sprintf(buf, "C ");
	pcmid(id, buf+i);i+=16;
	buf[i++]=' ';
	buf[i++]=cclasses[cls].letter;
	buf[i++]=' ';
	strncpy(buf+i, ev, HIST_LINE-i);
	return(ev_append(hist, d, t, buf));
}

int ge_append(history *hist, date d, harris_time t, cmid id, enum cclass cls, unsigned int lrate)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "GE %u", lrate);
	return(evc_append(hist, d, t, id, cls, buf));
}

int sk_append(history *hist, date d, harris_time t, cmid id, enum cclass cls, unsigned int skill)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "SK %u", skill);
	return(evc_append(hist, d, t, id, cls, buf));
}

int st_append(history *hist, date d, harris_time t, cmid id, enum cclass cls, enum cstatus status)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "ST %c", cstatuses[status][0]);
	return(evc_append(hist, d, t, id, cls, buf));
}

int op_append(history *hist, date d, harris_time t, cmid id, enum cclass cls, unsigned int tops)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "OP %u", tops);
	return(evc_append(hist, d, t, id, cls, buf));
}

int de_append(history *hist, date d, harris_time t, cmid id, enum cclass cls)
{
	return(evc_append(hist, d, t, id, cls, "DE"));
}

int pw_append(history *hist, date d, harris_time t, cmid id, enum cclass cls)
{
	return(evc_append(hist, d, t, id, cls, "PW"));
}

int ex_append(history *hist, date d, harris_time t, cmid id, enum cclass cls, unsigned int rt)
{
	char buf[HIST_LINE];
	snprintf(buf, HIST_LINE, "EX %u", rt);
	return(evc_append(hist, d, t, id, cls, buf));
}
