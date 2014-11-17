/* main.h: global variable declarations.

Copyright (C) 1992 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef MAIN_H
#define MAIN_H

#include "encoding.h"
#include "font.h"
#include "types.h"


/* If DEBUG is defined (and `debug' is nonzero), these print many
   voluminous messages about what the program is doing.  */ 

#ifdef DEBUG

extern int debug;

#define DEBUG_PRINT1(x) if (debug) printf (x)
#define DEBUG_PRINT2(x1, x2) if (debug) printf (x1, x2)
#define DEBUG_PRINT3(x1, x2, x3) if (debug) printf (x1, x2, x3)
#define DEBUG_PRINT4(x1, x2, x3, x4) if (debug) printf (x1, x2, x3, x4)

#else /* not DEBUG */

#define DEBUG_PRINT1(x)
#define DEBUG_PRINT2(x1, x2)
#define DEBUG_PRINT3(x1, x2, x3)
#define DEBUG_PRINT4(x1, x2, x3, x4)

#endif /* not DEBUG */

/* See main.c.  */
extern string dpi;
extern string font_name;
extern int starting_char, ending_char;
extern boolean verbose;
extern encoding_info_type encoding_info;
extern string encoding_name;
extern boolean no_gf;

/* This is defined in version.c.  */
extern string version_string;

#endif /* not MAIN_H */
