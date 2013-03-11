/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	history: logging of game events
*/
#include "history.h"
#include <stdlib.h>

char *hist_alloc(history *hist)
{
	size_t n=hist->nents++;
	if(hist->nents>hist->nalloc)
	{
		size_t ns=hist->nalloc+1024;
		char (*new)[80]=realloc(hist->ents, ns*sizeof(char [80]));
		if(!new)
		{
			hist->nents--;
			return(NULL);
		}
		hist->ents=new;
	}
	return(hist->ents[n]);
}

int hist_append(history *hist, const char line[80])
{
	char *p=hist_alloc(hist);
	if(!p) return(1);
	memcpy(p, line, 80);
	return(0);
}

int hist_save(history hist, FILE *out)
{
	if(!out) return(1);
	fprintf(out, "History: %zu\n", hist.nents);
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
	if(!in) return(1);
	if(!hist) return(1);
	char line[80];
	for(size_t i=0;i<nents;i++)
	{
		if(!fgets(line, 80, in))
			return(1);
		size_t l=strlen(line);
		if(line[l-1]=='\n') line[l-1]=0;
		if(strchr(line, '\\'))
		{
			char line2[80];
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

int ev_append(history *hist, date d, time t, const char *ev)
{
	char *p=hist_alloc(hist);
	if(!p) return(1);
	size_t i=0;
	i+=writedate(d, p+i);
	p[i++]=' ';
	i+=sprintf(p+i, "%02u:%02u ", t.hour, t.minute);
	strncpy(p+i, ev, 80-i);
	return(0);
}

int eva_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, const char *ev)
{
	char buf[80];
	size_t i=sprintf(buf, "A ");
	pacid(id, buf+i);i+=8;
	buf[i++]=' ';
	buf[i++]=ftr?'F':'B';
	i+=sprintf(buf+i, "%d ", type);
	strncpy(buf+i, ev, 80-i);
	return(ev_append(hist, d, t, buf));
}

int ct_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type)
{
	return(eva_append(hist, d, t, id, ftr, type, "CT"));
}

int na_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, unsigned int nid)
{
	char buf[80];
	snprintf(buf, 80, "NA %u", nid);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int pf_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type)
{
	return(eva_append(hist, d, t, id, ftr, type, "PF"));
}

int ra_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, unsigned int tid)
{
	char buf[80];
	snprintf(buf, 80, "RA %u", tid);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int hi_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, unsigned int bmb)
{
	char buf[80];
	snprintf(buf, 80, "HI %u", bmb);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int dmac_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, acid src)
{
	char buf[80];
	size_t i=snprintf(buf, 72, "DM %a %a AC ", ddmg, cdmg);
	pacid(src, buf+i);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int dmfk_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, unsigned int fid)
{
	char buf[80];
	snprintf(buf, 80, "DM %a %a FK %u", ddmg, cdmg, fid);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int dmtf_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, double ddmg, double cdmg, unsigned int tid)
{
	char buf[80];
	snprintf(buf, 80, "DM %a %a TF %u", ddmg, cdmg, tid);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int fa_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type, unsigned int fa)
{
	char buf[80];
	snprintf(buf, 80, "FA %u", fa);
	return(eva_append(hist, d, t, id, ftr, type, buf));
}

int cr_append(history *hist, date d, time t, acid id, bool ftr, unsigned int type)
{
	return(eva_append(hist, d, t, id, ftr, type, "CR"));
}

int evt_append(history *hist, date d, time t, unsigned int tid, const char *ev)
{
	char buf[80];
	size_t i=snprintf(buf, 80, "T %u ", tid);
	strncpy(buf+i, ev, 80-i);
	return(ev_append(hist, d, t, buf));
}

int tdm_append(history *hist, date d, time t, unsigned int tid, double ddmg, double cdmg)
{
	char buf[80];
	snprintf(buf, 80, "DM %a %a", ddmg, cdmg);
	return(evt_append(hist, d, t, tid, buf));
}

int tfk_append(history *hist, date d, time t, unsigned int tid, double dflk, double cflk)
{
	char buf[80];
	snprintf(buf, 80, "FK %a %a", dflk, cflk);
	return(evt_append(hist, d, t, tid, buf));
}

int tsh_append(history *hist, date d, time t, unsigned int tid)
{
	return(evt_append(hist, d, t, tid, "SH"));
}

int evm_append(history *hist, date d, time t, const char *ev)
{
	char buf[80];
	size_t i=sprintf(buf, "M ");
	strncpy(buf+i, ev, 80-i);
	return(ev_append(hist, d, t, buf));
}

int ca_append(history *hist, date d, time t, unsigned int cshr, unsigned int cash)
{
	char buf[80];
	snprintf(buf, 80, "CA %u %u", cshr, cash);
	return(evm_append(hist, d, t, buf));
}

int co_append(history *hist, date d, time t, double confid)
{
	char buf[80];
	snprintf(buf, 80, "CO %a", confid);
	return(evm_append(hist, d, t, buf));
}

int mo_append(history *hist, date d, time t, double morale)
{
	char buf[80];
	snprintf(buf, 80, "MO %a", morale);
	return(evm_append(hist, d, t, buf));
}
