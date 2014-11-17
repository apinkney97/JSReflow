/*
 * Copyright (C) 1990 Mark Leisher.
 *
 * Author: Mark Leisher (mleisher@nmsu.edu)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * A copy of the GNU General Public License can be obtained from this
 * program's author (send electronic mail to mleisher@nmsu.edu) or from
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA
 * 02139, USA.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include "snftobdf.h"

char *program;

void
usage()
{
    fprintf (stderr, "usage:  %s [-options ...] snffile ...\n\n", program);
    fprintf (stderr, "where options include:\n");
    fprintf (stderr, "\t-m\t\tset bit order to Most Significant Bit First\n");
    fprintf (stderr, "\t-l\t\tset bit order to Least Significant Bit First\n");
    fprintf (stderr, "\t-M\t\tset byte order to Most Significant Byte First\n");
    fprintf (stderr, "\t-L\t\tset byte order to Least Significant Byte First\n");
    fprintf (stderr, "\t-p#\t\tset glyph padding to #\n");
    fprintf (stderr, "\t-u#\t\tset scanline unit to #\n\n");
    exit (1);
}

main(argc, argv)
int argc;
char **argv;
{
    char *name;
    TempFont *tf;
    int glyphPad  = DEFAULTGLPAD,
        bitOrder  = DEFAULTBITORDER,
        byteOrder = DEFAULTBYTEORDER,
        scanUnit  = DEFAULTSCANUNIT;

    program = argv[0];

    argc--;
    *argv++;
    while(argc) {
        if (argv[0][0] == '-') {
            switch(argv[0][1]) {
              case 'm': bitOrder = MSBFirst; break;
              case 'l': bitOrder = LSBFirst; break;
              case 'M': byteOrder = MSBFirst; break;
              case 'L': byteOrder = LSBFirst; break;
              case 'p': glyphPad = atoi(argv[0] + 2); break;
              case 'u': scanUnit = atoi(argv[0] + 2); break;
              default: usage(); break;
            }
        } else if (!argv[0][0])
          usage();
        else {
            tf = GetSNFInfo(argv[0], bitOrder, byteOrder, scanUnit);
            BDFHeader(tf, argv[0]);
            BDFBitmaps(tf, glyphPad);
            BDFTrailer(tf);
        }
        argc--;
        *argv++;
    }
}
