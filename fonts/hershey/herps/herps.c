#include <stdio.h>

/*
 *	herps -- Convert Hershey Fonts to PostScript
 *
 * The output is a series of sample pages suitable for printing
 * on your Apple LaserWriter or other PostScript printer.  It doesn't
 * attempt to give you what to need to get something useful out of troff
 * or another formatter -- this is left as an exercise to the reader.
 *
 * Usage:
 *
 *	cat hersh.oc? ^ herps ^ lp -dPostScript
 *
 * Author:
 *
 *	Guy Riddle
 *	AT&T Bell Laboratories
 *
 *	<ihnp4!nomad!ggr> or <ggr@btl.csnet>
 */

	char	glyphnum[6], glyphlength[4];

	int	gcount, page;

	char	*prolog[] = {
#include "prolog.h"
		0
	};
	char	*pagestart[] = {
#include "pagestart.h"
		0
	};
	char	*pagefinish[] = {
#include "pagefinish.h"
		0
	};
	char	*trailer[] = {
#include "trailer.h"
		0
	};

main()
{
	squirt(prolog);

	while(fread(glyphnum, 1, sizeof(glyphnum)-1, stdin) == (sizeof(glyphnum)-1))
		glyph(atoi(glyphnum));

	finishpage();
	squirt(trailer);
	printf("%%%%Pages: %d\n", page);

	exit(0);
}

glyph(gnum)
{
	int	c, glen, xl, xr;

	if(gcount == 64){
		finishpage();
		gcount = 0;
	}
	if(gcount == 0)
		startpage();

	fread(glyphlength, 1, sizeof(glyphlength)-1, stdin);

	glen = atoi(glyphlength)*2 - 2;

	xl = getchar() - 'R';
	xr = getchar() - 'R';

	printf("/%d [%d %d %d\n(", gcount, gnum, xl, xr);

	while(--glen >= 0)
		switch(c = getchar()){
		case '\n':
			glen++;
			break;

		case ' ':
			putchar(c);
			(void) getchar();
			glen--;
			break;

		case '\\':
		case '(':
		case ')':
			putchar('\\');
		default:
			putchar(c);
		}

	printf(")\n] def\n");

	if(getchar() != '\n'){
		fprintf(stderr, "herps: Input format botch\n");
		exit(65);
	}
	if((c = getchar()) != '\n')
		ungetc(c, stdin);

	gcount++;
}

startpage()
{
	printf("%%%%Page: %d\n", ++page);

	squirt(pagestart);
}

finishpage()
{
	squirt(pagefinish);

	printf("%d HersheyPage\n", gcount);
}

squirt(stuff)
	char	**stuff;
{
	char	*p;

	while(p = *stuff++)
		printf("%s\n", p);
}
