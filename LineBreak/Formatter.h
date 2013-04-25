//
//  Formatter.h
//  LineBreak
//
//  Created by Steven Bagley on 01/04/2013.
//  Copyright (c) 2013 Steven Bagley. All rights reserved.
//

#ifndef LineBreak_Formatter_h
#define LineBreak_Formatter_h

#include "parseAFM.h"


typedef enum {
    BOX = 0,
    PENALTY,
    GLUE
} FORMATTABLETYPE;

typedef struct _item {
    FORMATTABLETYPE type;
    double width;
    union {
        struct {
            char *text;
            int textLength;
            FontInfo *font;
            double pointSize;
        } box;
        struct {
            double stretch;     /* y */
            double shrink;      /* z */
        } glue;
        struct {
            double penalty;
            int flag;
        } penalty;
    } data;

} FORMATTABLEITEM;

#define PEN_INF 1000.0

/* Internal Structures */

typedef struct _textFormatterNode {
    int position;
    int afterPosition;
    int line;
    int fitness;
    double totalWidth, totalStretch, totalShrink;
    double totalDemerits;
    double adjustmentRatio;
    struct _textFormatterNode *previous;
    struct _textFormatterNode *next;

} TEXTFORMATNODE;

typedef double (*LWFUNCP) (int line);

typedef struct _textFormatter {
    LWFUNCP lineWidth;
    int maxItems;
    int nextFreeItem;
    FORMATTABLEITEM *items;
    int numLines;
    TEXTFORMATNODE **lines;
    /* internal */
    double Sw, Sy, Sz;
    TEXTFORMATNODE *a, *activeNodes;
    int i, j;
    double tw, ty, tz;
    int iAfter;

} TEXTFORMATTER;


/* Function Prototypes */
TEXTFORMATTER *CreateFormatter(int itemCount);
int AddGlue(TEXTFORMATTER * f, double width, double stretch, double shrink);
int AddTextBox(TEXTFORMATTER * f, char *text, int textLength, FontInfo * font, double pointSize);
int AddPenalty(TEXTFORMATTER * f, double width, double penalty, int flag);



int Format(TEXTFORMATTER * f, double ro);


#endif
