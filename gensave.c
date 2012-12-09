#include <stdio.h>
#include <ctype.h>
#include "bits.h"

int main(void)
{
	char *line;
	while(!feof(stdin)&&(line=fgetl(stdin)))
	{
		if((line[0]=='*')&&isdigit(line[1]))
		{
			size_t count;
			int n;
			if(sscanf(line, "*%zu*%n", &count, &n)!=1)
			{
				fprintf(stderr, "gensave: Error on line: %s\n", line);
				return(1);
			}
			for(size_t i=0;i<count;i++)
				printf("%s\n", line+n);
		}
		else
			printf("%s\n", line);
		free(line);
	}
	return(0);
}
