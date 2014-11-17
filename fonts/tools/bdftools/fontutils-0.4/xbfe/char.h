/* char.h: declarations for handling TFM and bitmap characters simultaneously.

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

#ifndef CHAR_H
#define CHAR_H

#include "font.h"
#include "tfm.h"
#include "types.h"
#include "xt-common.h"


/* The representation of a single character.  */
typedef struct
{
  char_info_type bitmap_info;
  tfm_char_type tfm_info;
} char_type;

/* The bitmap-derived information about the character C.  */
#define CHAR_BITMAP_INFO(c) ((c).bitmap_info)
#define BCHAR_BB(c) CHAR_BB (CHAR_BITMAP_INFO (c))
#define BCHAR_BITMAP(c) CHAR_BITMAP (CHAR_BITMAP_INFO (c))
#define BCHAR_MAX_COL(c) MAX_COL (BCHAR_BB (c))
#define BCHAR_MIN_COL(c) MIN_COL (BCHAR_BB (c))
#define BCHAR_MAX_ROW(c) MAX_ROW (BCHAR_BB (c))
#define BCHAR_MIN_ROW(c) MIN_ROW (BCHAR_BB (c))
#define BCHAR_SET_WIDTH(c) CHAR_SET_WIDTH (CHAR_BITMAP_INFO (c))
#define BCHAR_TFM_WIDTH(c) CHAR_TFM_WIDTH (CHAR_BITMAP_INFO (c))

/* The tfm-derived information about the character C.  */
#define CHAR_TFM_INFO(c) ((c).tfm_info)


/* This defines `font_type', which uses `char_type', so we can't include
   it until now.  */
#include "main.h"


/* Return a pointer to the character CODE from the font F.  If the
   character hasn't been read yet, call `font_read_char'.  */
extern char_type *read_char (font_type f, charcode_type code);

/* Display the character C in the font F using the widgets below W.  */
extern void show_char (Widget w, font_type *f, char_type *c);

#endif /* not CHAR_H */
