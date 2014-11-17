#ifndef _bintobdf_h
#define _bintobdf_h

#include <X11/Xos.h>
#include <X11/Xmd.h>
#include <X11/X.h>
#include <X11/Xproto.h>

#include "fontstruct.h"
#include "bdftosnf.h"

#define AUTHOR_STRING "mleisher@nmsu.edu (Mark Leisher)"
#define BINTOBDFVERSION 1

#define FALSE 0
#define TRUE  1

/* Make sure the directory separator is defined for loading fonts. */

#ifndef DIR_SEP
#define DIR_SEP '/'
#endif

/*
 * This is the x and y resolution the font was designed at.
 * Most of the time, these values are just guessed at for bitmapped
 * fonts.
 */
#ifndef DEFAULTXRES
#define DEFAULTXRES 100
#endif

#ifndef DEFAULTYRES
#define DEFAULTYRES 100
#endif

#endif /* _bintobdf_h */
