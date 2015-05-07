/*
	harris - a strategy game
	Copyright (C) 2012-2015 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	bits: misc. functions, mostly for string buffers
*/
#include "bits.h"
#include <stdbool.h>
#include <ctype.h>

char *fgetl(FILE *fp)
{
	bool eof=false;
	string s=init_string();
	signed int c;
	while(!feof(fp))
	{
		c=fgetc(fp);
		if(c==EOF)
		{
			eof=true;
			break;
		}
		if(c=='\n')
			break;
		if(c!=0)
		{
			append_char(&s, c);
		}
	}
	if(eof&&!s.i)
	{
		free(s.buf);
		return(NULL);
	}
	return(s.buf);
}

char *slurp(FILE *fp)
{
	string s=init_string();
	signed int c;
	while(!feof(fp))
	{
		c=fgetc(fp);
		if(c==EOF)
			break;
		if(c!=0)
			append_char(&s, c);
	}
	return(s.buf);
}

string sslurp(FILE *fp)
{
	string s=init_string();
	signed int c;
	while(!feof(fp))
	{
		c=fgetc(fp);
		if(c==EOF)
			break;
		append_char(&s, c);
	}
	return(s);
}

void append_char(string *s, char c)
{
	if(s->buf)
	{
		s->buf[s->i++]=c;
	}
	else
	{
		*s=init_string();
		append_char(s, c);
	}
	char *nbuf=s->buf;
	if(s->i>=s->l)
	{
		if(s->i)
			s->l=s->i*2;
		else
			s->l=80;
		nbuf=(char *)realloc(s->buf, s->l);
	}
	if(nbuf)
	{
		s->buf=nbuf;
		s->buf[s->i]=0;
	}
	else
	{
		free(s->buf);
		*s=init_string();
	}
}

void append_str(string *s, const char *str)
{
	while(str && *str) // not the most tremendously efficient implementation, but conceptually simple at least
	{
		append_char(s, *str++);
	}
}

void append_string(string *s, const string t)
{
	if(!s) return;
	size_t i=s->i+t.i;
	if(i>=s->l)
	{
		size_t l=i+1;
		char *nbuf=(char *)realloc(s->buf, l);
		if(!nbuf) return;
		s->l=l;
		s->buf=nbuf;
	}
	memcpy(s->buf+s->i, t.buf, t.i);
	s->buf[i]=0;
	s->i=i;
}

string init_string(void)
{
	string s;
	s.buf=(char *)malloc(s.l=80);
	if(s.buf)
		s.buf[0]=0;
	else
		s.l=0;
	s.i=0;
	return(s);
}

string null_string(void)
{
	return((string){.buf=NULL, .l=0, .i=0});
}

string make_string(const char *str)
{
	string s=init_string();
	append_str(&s, str);
	return(s);
}

void free_string(string *s)
{
	free(s->buf);
}

const char hex[16]="0123456789abcdef";

void pacid(acid id, char buf[9])
{
	for(size_t i=0;i<8;i++)
		buf[i]=hex[(id>>(i<<2))&0xf];
	buf[8]=0;
}

int gacid(const char from[8], acid *buf)
{
	if(!buf) return(1);
	*buf=0;
	unsigned int val;
	char fbuf[2]={0, 0};
	for(size_t i=0;i<8;i++)
	{
		if(!isxdigit(from[i])) return(1);
		fbuf[0]=from[i];
		sscanf(fbuf, "%x", &val);
		*buf|=val<<(i<<2);
	}
	return(0);
}

#ifdef WINDOWS /* doesn't have strndup, we need to implement one */
char *strndup(const char *s, size_t size)
{
	char *rv=(char *)malloc(size+1);
	if(rv==NULL) return(NULL);
	strncpy(rv, s, size);
	rv[size]=0;
	return(rv);
}
#endif
