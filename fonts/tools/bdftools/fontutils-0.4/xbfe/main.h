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

#include "xt-common.h"

#include "font.h"
#include "types.h"

#include "char.h"


/* For the resource database.  */
#define CLASS_NAME "XBfe"

typedef struct
{
  string name;
  real dpi;
  boolean modified;
  font_info_type info;
  char_type *chars[MAX_CHARCODE + 1];
  charcode_type current_charcode;
} font_type;

/* The basename of the font, as it was given to the program.  */
#define FONT_NAME(f) ((f).name)

/* The resolution of the font, in pixels per inch.  */
#define FONT_DPI(f) ((f).dpi)

/* Says whether any of the characters in the font have been edited.  */
#define FONT_MODIFIED(f) ((f).modified)

/* The information about the font that the library supplies.  See
   `../include/font.h' for the details on this structure.  */
#define FONT_INFO(f) ((f).info)

/* Pointers to the characters.  */
#define FONT_CHAR(f, code) ((f).chars[code])

/* A pointer to the character we're currently editing in the font F.  */
#define FONT_CURRENT_CHAR(f) FONT_CHAR (f, FONT_CURRENT_CHARCODE (f))

/* The character code of the character we are currently displaying.  */
#define FONT_CURRENT_CHARCODE(f) ((f).current_charcode)

#endif /* not MAIN_H */
