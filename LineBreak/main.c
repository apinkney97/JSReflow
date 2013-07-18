//
//  main.c
//  LineBreak
//
//  Created by Steven Bagley on 30/03/2013.
//  Copyright (c) 2013 Steven Bagley. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parseAFM.h"
#include "Formatter.h"


double gLineWidth = 2.0;
double gLeading;
double lineWidth(int line)
{
    return gLineWidth * 72.0;
}


void ExportPS(TEXTFORMATTER * f)
{
    int i, j;
    int lineStart = 0;
    FontInfo *pf = NULL;
    double ps = 0;
    double r;
    int c;
    char *p;
	double x;
    for (i = 0; i < f->numLines; i++) {
        r = f->lines[i]->adjustmentRatio;
		x = 0;
		
		// printf("[", gLeading);
		
        for (j = lineStart; j <= f->lines[i]->position; j++) {
            if (f->items[j].type == BOX) {

                if (pf != f->items[j].data.box.font || ps != f->items[j].data.box.pointSize) {
                    pf = f->items[j].data.box.font;
                    ps = f->items[j].data.box.pointSize;
                    //printf("/%s %f selectfont\n", pf->gfi->fontName, ps);
                }
                printf("%g,", x);

                p = f->items[j].data.box.text;
                for (c = 0; c < f->items[j].data.box.textLength; c++) {
                    if (*p == '<') {
                        printf("&lt;");
                    } else if (*p == '>') {
						printf("&gt;");
					} else if (*p == '&') {
						printf("&amp;");
					} else if (*p == '"') {
						printf("&quot;");
					} else if (*p == '\'') {
						printf("&#39;"); // apparently &apos; isn't supported by all browsers...
					} else if (*p == '\\') {
						printf("&#92;");
					} else {
						putchar(*p);
					}
                    p++;
                }

                printf(" "); // trailing commas will probably be ok (see http://stackoverflow.com/questions/7246618/trailing-commas-in-javascript )
                x += f->items[j].width;
            } else if (f->items[j].type == GLUE) {
                x += f->items[j].width + (r < 0 ? (r * f->items[j].data.glue.shrink) : (r * f->items[j].data.glue.stretch));
            } else if (f->items[j].type == PENALTY && j == f->lines[i]->position) {

            }

        }
        lineStart = f->lines[i]->afterPosition;
		
		printf("\n");
		
    }


}


int main(int argc, const char *argv[])
{
    const char *afmPath, *textfile;
    double pointSize;
    FontInfo *fontInfo = NULL;
    FILE *afm;
    FILE *text;
    int state;
    int c;
    char word[256];
    char *p;
    TEXTFORMATTER *formatter;
    int i;


    /* Parse command line */
    /* Default: LineBreak <textfile> <afm_file> <point_size> */

    if (argc < 4) {
        fprintf(stderr, "%s <textfile> <afm_file> <point_size>\n", argv[0]);
        return 1;
    }

    textfile = argv[1];
    afmPath = argv[2];
    pointSize = atof(argv[3]);
	
	if (argc >= 5) {
		gLineWidth = atof(argv[4]);
	}

    /* Load and parse the AFM file */
    afm = fopen(afmPath, "r");

    if (afm && ok != parseFile(afm, &fontInfo, P_G | P_W)) {
        fprintf(stderr, "Error parsing AFM file: %s\n", afmPath);
        return 2;
    }

    fclose(afm);

    formatter = CreateFormatter(15000);


    /* Now we can read in the text and start building up the data structures */
	if (strcmp(textfile, "-") == 0) {
		text = stdin;
	} else {
		text = fopen(textfile, "r");
	}
	
    if (!text) {
        fprintf(stderr, "Error opening text file: %s\n", textfile);
        return 3;
    }
    state = 0;
    p = word;
    while ((c = fgetc(text)) != EOF) {
        if (!isspace(c)) {
            *p++ = c;
            state = 1;
        } else {
            if (state == 1) {
                *p++ = '\0';
                
                state = 0;
				
				if ((p = strchr(word, '-')) && p != word)
				{
					AddTextBox(formatter, word, (int)(p - word+1), fontInfo, pointSize);
					AddPenalty(formatter, 0, 0, 1);
					AddTextBox(formatter, p+1, (int)strlen(p+1), fontInfo, pointSize);
				}
				else
					AddTextBox(formatter, word, (int) strlen(word), fontInfo, pointSize);
				p = word;
                AddGlue(formatter, pointSize / 4.0, pointSize / 3.0 - pointSize / 4.0, pointSize / 4.0 - pointSize / 5.0);
            }
        }
    }

    if (state) {
        *p++ = '\0';
        //   printf("[%s]\n", word);
        AddTextBox(formatter, word, (int) strlen(word), fontInfo, pointSize);
        AddGlue(formatter, pointSize / 4.0, pointSize / 3.0 - pointSize / 4.0, pointSize / 4.0 - pointSize / 5.0);
    }

    fclose(text);

    /* Hack last glue */
    if (formatter->items[formatter->nextFreeItem - 1].type == GLUE) {
        formatter->items[formatter->nextFreeItem - 1].width = 0;
        formatter->items[formatter->nextFreeItem - 1].data.glue.stretch = 10000.0;
        formatter->items[formatter->nextFreeItem - 1].data.glue.shrink = 0;
    } else {
        fprintf(stderr, "That was unexpected\n");
    }


    AddPenalty(formatter, 0, -PEN_INF, 1);
    formatter->lineWidth = lineWidth;

    Format(formatter, 15);

    gLeading = pointSize * 1.2;
    ExportPS(formatter);


    return 0;
}
