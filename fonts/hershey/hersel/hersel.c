#include <stdio.h>

/*
 *	hersel -- Make a PostScript Font from Selected Hershey Characters
 *
 * This program takes a "selection file" and a file of Hershey glyphs
 * and creates 3 output files:
 *
 *	a .pro file -- PostScript code that when included in a PostScript
 *		program makes a font that may then be accessed via 'findfont'
 *	a .afm file -- provides the Adobe Font Metrics information needed
 *		by the TranScript shell file 'afmdit' to construct a
 *		troff .out file
 *	a .map file -- used by TranScript software to map the 2-character
 *		troff font name to the internal font name
 *
 * You should be able to deduce the format of the .sel file from the
 * enclosed examples without much trouble.
 *
 * Usage:
 *
 *	hersel font.sel hersh.oc
 *
 * Author:
 *
 *	Guy Riddle
 *	AT&T Bell Laboratories
 *	Liberty Corner, NJ  07938
 *
 *	<attmail!ggr> or <ihnp4!garage!ggr> or <ggr@btl.csnet>
 */


	typedef struct {
		char	*key;
		char	*value;
		short	glyph;
		short	adjleft;
		short	adjright;
	} Tmt;

	int	glyphcount;
	char	*iam;
	FILE	*sel, *font, *map, *afm, *pro;

	char	longname[128], shortname[16], troffname[4];
	char	penwidth[8]	= "1";
	char	italicangle[8]	= "0";

	FILE	*openin(), *openout();
	Tmt	*match();

	Tmt 	keywords[] = {
		{ "longname", longname },
		{ "shortname", shortname },
		{ "troffname", troffname },
		{ "penwidth", penwidth },
		{ "italicangle", italicangle },
		0
	};

	Tmt	encoding[] = {
#include "standard.h"
		0
	};

	char	*makefont[] = {
#include "makefont.h"
		0
	};

main(argc, argv)
	char	*argv[];
{
	iam = argv[0];

	if(argc != 3){
		fprintf(stderr, "Usage:  %s select-file font-file\n", iam);
		exit(64);
	}

	sel = openin(argv[1]);
	font = openin(argv[2]);

	readselect();

	if(troffname[0]){
		map = openout(troffname, "map");
		fprintf(map, "%s\n", longname);
	}

	afm = openout(shortname, "afm");
	pro = openout(shortname, "pro");

	fprintf(afm, "StartFontMetrics 1.0\n");
	fprintf(afm, "Comment Selected characters from the Hershey Fonts\n");
	fprintf(afm, "FontName %s\n", longname);
	fprintf(afm, "Notice Created by the program `hersel'\n");
	fprintf(afm, "Notice Program author Guy Riddle <ihnp4!nomad!ggr>\n");
	fprintf(afm, "Comment The bounding boxes are bogus\n");
	fprintf(afm, "FontBBox 0 0 1000 1000\n");
	fprintf(afm, "StartCharMetrics\n");

	makepro();

	fprintf(afm, "EndCharMetrics\n");
	fprintf(afm, "EndFontMetrics\n");

	exit(0);
}

readselect()
{
	Tmt	*p;
	char	line[128], left[64], right[64], adjleft[8], adjright[8];

	while(fgets(line, sizeof(line), sel)){
		if(line[0] == '#')
			continue;

		adjleft[0] = adjright[0] = '\0';

		sscanf(line, "%s %s %s %s", left, right, adjleft, adjright);

		if(p = match(left, keywords))
			strcpy(p->value, right);
		else if(p = match(left, encoding)){
			p->glyph = atoi(right);
			p->adjleft = atoi(adjleft);
			p->adjright = atoi(adjright);
			glyphcount++;
		}else
			fprintf(stderr, "%s: no keyword or character %s\n", iam, left);
	}
}

makepro()
{
	Tmt	*p;
	int	glyph, xl, xr, wx;
	char	strokes[2048];

	squirt(pro, makefont);

	fprintf(pro, "%d dict dup begin\n", glyphcount);

	while(glyph = hersheyglyph(&xl, &xr, strokes))
		for(p = encoding; p->key; p++)
			if(glyph == p->glyph){
				xl += p->adjleft;
				xr += p->adjright;

				fprintf(pro, "/%s [%d %d (%s)] def\n",
				  p->key, xl, xr, strokes);

				if(p->key[0] != '.'){
					wx = ((xr - xl) * 1000) / 33;
					fprintf(afm, "C %d ; WX %d ; N %s ; B 0 0 1000 1000 ;\n",
					  (p - encoding), wx, p->key);
				}
			}

	fprintf(pro, "end /%s %s %s MakeHersheyFont\n", longname, penwidth, italicangle);
}

hersheyglyph(xlp, xrp, sp)
	int	*xlp, *xrp;
	char	*sp;
{
	int	c, glen;
	char	glyphnum[6], glyphlength[4];
	
	if(fread(glyphnum, 1, sizeof(glyphnum)-1, font) != (sizeof(glyphnum)-1))
		return(0);

	fread(glyphlength, 1, sizeof(glyphlength)-1, font);

	glen = atoi(glyphlength)*2 - 2;

	*xlp = getc(font) - 'R';
	*xrp = getc(font) - 'R';

	while(--glen >= 0)
		switch(c = getc(font)){
		case '\n':
			glen++;
			break;

		case ' ':
			*sp++ = c;
			(void) getc(font);
			glen--;
			break;

		case '\\':
		case '(':
		case ')':
			*sp++ = '\\';
		default:
			*sp++ = c;
		}
	*sp = '\0';

	if(getc(font) != '\n'){
		fprintf(stderr, "%s: font input format botch\n", iam);
		exit(65);
	}
	if((c = getc(font)) != '\n')
		ungetc(c, font);
	return(atoi(glyphnum));
}

squirt(fp, stuff)
	FILE	*fp;
	char	**stuff;
{
	char	*p;

	while(p = *stuff++)
		fprintf(fp, "%s\n", p);
}

	FILE *
openin(name)
	char	*name;
{
	FILE	*fp;

	fp = fopen(name, "r");

	if(fp == NULL){
		fprintf(stderr, "%s: can't open %s\n", iam, name);
		exit(66);
	}

	return(fp);
}

	FILE *
openout(pre, suf)
	char	*pre, *suf;
{
	FILE	*fp;
	char	name[16];

	sprintf(name, "%s.%s", pre, suf);

	fp = fopen(name, "w");

	if(fp == NULL){
		fprintf(stderr, "%s: can't create %s\n", iam, name);
		exit(73);
	}

	return(fp);
}

	Tmt *
match(key, table)
	char	*key;
	Tmt	table[];
{
	Tmt	*p;

	for(p = table; p->key; p++)
		if(strcmp(key, p->key) == 0)
			return(p);
	return(0);
}
