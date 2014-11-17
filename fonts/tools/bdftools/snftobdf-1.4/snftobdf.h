#ifndef _snftobdf_h
#define _snftobdf_h

/* Note: n2dChars is a macro defined in
         mit/server/include/font.h

         the file fontutil.c only used for
         bit and byte ordering functions   */

#include <X11/Xos.h>
#include <X11/Xmd.h>
#include <X11/X.h>
#include <X11/Xproto.h>

#include "misc.h"
#include "fontstruct.h"
#include "snfstruct.h"
#include "font.h"
#include "bdftosnf.h"

#define AUTHOR_STRING "mleisher@nmsu.edu (Mark Leisher)"
#define SNFTOBDF_VERSION 1

#define BDF_VERSION "2.1"

#define IsLinear(pfi) (((pfi)->firstRow == 0) && ((pfi)->lastRow == 0))

#define ReallyNonExistent(pci) \
  ((((pci).metrics.leftSideBearing + (pci).metrics.rightSideBearing) == 0) && \
   (((pci).metrics.ascent + (pci).metrics.descent) == 0))


TempFont *
GetSNFInfo();
/*
char *path;
int bitOrder, byteOrder, scanUnit;
*/

unsigned char *
GetSNFBitmap();
/*
TempFont *tf;
unsigned int charnum;
int glyphPad;
*/

void
BDFHeader();
/*
TempFont *tf;
*/

void
BDFBitmaps();
/*
TempFont *tf;
int glyphPad;
*/

void
BDFTrailer();
/*
TempFont *tf;
*/

#endif /* _snftobdf_h */
