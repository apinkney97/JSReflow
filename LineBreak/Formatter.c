//
//  Formatter.c
//  LineBreak
//
//  Created by Steven Bagley on 01/04/2013.
//  Copyright (c) 2013 Steven Bagley. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

#include "Formatter.h"
#include "parseAFM.h"

TEXTFORMATTER *CreateFormatter(int itemCount)
{
    TEXTFORMATTER *f = (TEXTFORMATTER *) malloc(sizeof(TEXTFORMATTER));

    if (f) {
        f->items = (FORMATTABLEITEM *) malloc(itemCount * sizeof(FORMATTABLEITEM));
        if (!f->items) {
            f = NULL;
            return NULL;
        }

        f->maxItems = itemCount;
        f->nextFreeItem = 0;
    }

    return f;
}

int AddGlue(TEXTFORMATTER * f, double width, double stretch, double shrink)
{
    if (f && f->nextFreeItem < f->maxItems) {
        f->items[f->nextFreeItem].type = GLUE;
        f->items[f->nextFreeItem].width = width;
        f->items[f->nextFreeItem].data.glue.stretch = stretch;
        f->items[f->nextFreeItem].data.glue.shrink = shrink;
        f->nextFreeItem++;

        return 1;
    }
    return 0;
}

int AddTextBox(TEXTFORMATTER * f, char *text, int textLength, FontInfo * font, double pointSize)
{
    double width;
    int i;

    if (f && f->nextFreeItem < f->maxItems) {
        f->items[f->nextFreeItem].type = BOX;
        f->items[f->nextFreeItem].data.box.text = (char *) malloc(textLength);
        memcpy(f->items[f->nextFreeItem].data.box.text, text, textLength);
        f->items[f->nextFreeItem].data.box.textLength = textLength;
        f->items[f->nextFreeItem].data.box.font = font;
        f->items[f->nextFreeItem].data.box.pointSize = pointSize;

        /* Now calculate the width */
        width = 0.0;
        for (i = 0; i < textLength; i++) {
            width += font->cwi[text[i]];
        }

        f->items[f->nextFreeItem].width = width * pointSize / 1000.0;

        //     printf("[%s, %f]\n", f->items[f->nextFreeItem].data.box.text, f->items[f->nextFreeItem].width);
        f->nextFreeItem++;
        return 1;
    }
    return 0;
}

int AddPenalty(TEXTFORMATTER * f, double width, double penalty, int flag)
{
    if (f && f->nextFreeItem < f->maxItems) {
        f->items[f->nextFreeItem].type = PENALTY;
        f->items[f->nextFreeItem].width = width;
        f->items[f->nextFreeItem].data.penalty.penalty = penalty;
        f->items[f->nextFreeItem].data.penalty.flag = flag;
        f->nextFreeItem++;

        return 1;
    }
    return 0;
}

#define ALPHA (3000.0)
#define YOTTA (1000.0)

static inline double ComputeAdjustmentRatio(TEXTFORMATTER * f)
{
    double r, L, Y, Z;
    double lj;

    L = f->Sw - f->a->totalWidth + (f->items[f->i].type == PENALTY ? f->items[f->i].width : 0.0);
    lj = f->lineWidth(f->j);

    r = 0;
    if (L < lj) {
        Y = f->Sy - f->a->totalStretch;
        r = Y > 0 ? (lj - L) / Y : DBL_MAX;
    } else if (L > lj) {
        Z = f->Sz - f->a->totalShrink;
        r = Z > 0 ? (lj - L) / Z : DBL_MAX;
    }


    return r;
}

static inline int fitnessClass(double r)
{
    if (r < -0.5) {
        return 0;
    } else if (r <= 0.5) {
        return 1;
    } else if (r <= 1) {
        return 2;
    } else {
        return 3;
    }

}

static inline void findAfter(TEXTFORMATTER * f)
{
    f->tw = f->Sw;
    f->ty = f->Sy;
    f->tz = f->Sz;
    for (f->iAfter = f->i; f->iAfter < f->nextFreeItem; (f->iAfter)++) {
        if (f->items[f->iAfter].type == BOX)
            break;
        else if (f->items[f->iAfter].type == GLUE) {
            f->tw += f->items[f->iAfter].width;
            f->ty += f->items[f->iAfter].data.glue.stretch;
            f->tz += f->items[f->iAfter].data.glue.shrink;
        } else if (f->items[f->iAfter].type == PENALTY && f->items[f->iAfter].data.penalty.penalty <= -PEN_INF && f->iAfter > f->i) {
            break;
        }
    }

}


int Format(TEXTFORMATTER * f, double ro)
{
    double r;
    double R[4];
    double D[4], dA, d;
    int j0;
    int c;
    TEXTFORMATNODE *prevA, *nextA;
    TEXTFORMATNODE *A[4];
    TEXTFORMATNODE *n;

    j0 = 3;

    f->a = (TEXTFORMATNODE *) malloc(sizeof(TEXTFORMATNODE));
    f->a->position = 0;
    f->a->line = -1;
    f->a->fitness = 1;
    f->a->totalWidth = f->a->totalStretch = f->a->totalShrink = 0;
    f->a->totalDemerits = 0;
    f->a->previous = NULL;
    f->a->next = NULL;

    f->activeNodes = f->a;

    f->Sw = f->Sy = f->Sz = 0.0;
    for (f->i = 0; f->i < f->nextFreeItem; (f->i)++) {
        if (f->items[f->i].type == BOX) {
            f->Sw = f->Sw + f->items[f->i].width;
            continue;           /* Not a legal break point */
        } else if (f->items[f->i].type == GLUE && f->items[f->i - 1].type != BOX) {
            f->Sw = f->Sw + f->items[f->i].width;
            f->Sy = f->Sy + f->items[f->i].data.glue.stretch;
            f->Sz = f->Sz + f->items[f->i].data.glue.shrink;
            continue;
        } else if (f->items[f->i].type == PENALTY && f->items[f->i].data.penalty.penalty >= PEN_INF) {
            continue;
        }

        /* OK, got a legal break position */
        f->a = f->activeNodes;
        prevA = NULL;
        do {
            D[0] = D[1] = D[2] = D[3] = dA = DBL_MAX;
            do {
                nextA = f->a->next;
                f->j = f->a->line + 1;

                /* Compute adjustment ratio, r */
                r = ComputeAdjustmentRatio(f);

                if (r < -1 || (f->items[f->i].type == PENALTY && f->items[f->i].data.penalty.penalty <= -PEN_INF)) {
                    /* Deactive node a */
                    if (prevA == NULL) {
                        if (nextA == NULL && dA == DBL_MAX && r < -1) {
                            r = -1;
                        } else
                            f->activeNodes = nextA;
                    } else {
                        prevA->next = nextA;
                    }
                } else {
                    prevA = f->a;
                }

                if (r >= -1 && r <= ro) {
                    /* Computer demerits, d, and fitness class c */
                    if (f->items[f->i].type == PENALTY && f->items[f->i].data.penalty.penalty >= 0) {
                        d = pow(1 + 100 * pow(fabs(r), 3) + f->items[f->i].data.penalty.penalty, 2);
                    } else if (f->items[f->i].type == PENALTY && f->items[f->i].data.penalty.penalty > -PEN_INF) {
                        d = pow(1 + 100 * pow(fabs(r), 3), 2) - pow(f->items[f->i].data.penalty.penalty, 2);
                    } else {
                        d = pow(1 + 100 * pow(fabs(r), 3), 2);
                    }

                    /* Do flagged check */
                    if (f->items[f->i].type == PENALTY
                        && f->items[f->a->position].type == PENALTY
                        && f->items[f->i].data.penalty.flag == f->items[f->a->position].data.penalty.flag) {
                        d += ALPHA;
                    }

                    c = fitnessClass(r);

                    if (fabs(c - f->a->fitness) > 1) {
                        d = d + YOTTA;
                    }

                    d += f->a->totalDemerits;

                    if (d < D[c]) {
                        D[c] = d;
                        A[c] = f->a;
                        R[c] = r;
                        if (d < dA) {
                            dA = d;
                        }
                    }

                }
                /* -1 <= r <= ro */
                f->a = nextA;
                if (f->a && f->a->line > f->j && f->j < j0) {
                    break;
                }
            } while (f->a != NULL);
            /* for each active node */

            if (d < DBL_MAX) {
                /* Compute tw, ty, tz after(b) */

                for (c = 0; c < 4; c++) {
                    if (D[c] < dA + YOTTA) {
                        findAfter(f);

                        /* Get new node */

                        n = (TEXTFORMATNODE *)
                            malloc(sizeof(TEXTFORMATNODE));
                        n->position = f->i;
                        n->afterPosition = f->iAfter;
                        n->adjustmentRatio = R[c];
                        n->line = A[c]->line + 1;
                        n->fitness = c;
                        n->totalWidth = f->tw;
                        n->totalStretch = f->ty;
                        n->totalShrink = f->tz;
                        n->totalDemerits = D[c];
                        n->previous = A[c];
                        n->next = f->a;

                        if (prevA == NULL) {
                            f->activeNodes = n;
                        } else {
                            prevA->next = n;
                        }
                        prevA = n;
                    }

                }

            }

        } while (f->a != NULL);


        if (f->activeNodes == NULL) {
            fprintf(stderr, "Drastic solution needed here! (Overfull/underfull hbox?)\n");
            return 3;
        }

        if (f->items[f->i].type == GLUE) {
            f->Sw = f->Sw + f->items[f->i].width;
            f->Sy = f->Sy + f->items[f->i].data.glue.stretch;
            f->Sz = f->Sz + f->items[f->i].data.glue.shrink;
        }

    }                           /* for each item */

    /* Select best node and export lines */
    dA = f->activeNodes->totalDemerits;
    n = f->activeNodes;
    nextA = f->activeNodes->next;
    while (nextA != NULL) {
        if (nextA->totalDemerits < dA) {
            n = nextA;
            dA = nextA->totalDemerits;
        }
        nextA = nextA->next;
    }

    f->numLines = n->line + 1;
    f->lines = (TEXTFORMATNODE **) malloc(f->numLines * sizeof(TEXTFORMATNODE *));

    while (n->line != -1) {
        f->lines[n->line] = n;
        n = n->previous;
    }

//    free(L);
    return 0;
}
