/* psutil.h: utility routines for PostScript output (used for both Type 1 and
   Type 3 format).

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

#ifndef PSUTIL_H
#define PSUTIL_H

#include <stdio.h>
#include "tfm.h"
#include "types.h"


/* Global information about a PostScript font.  */
typedef struct
{
  string family_name;
  string font_name;
  int italic_angle; /* In degrees.  */
  boolean monospace_p;
  int underline_position, underline_thickness; /* In 1000 units/em.  */
  int unique_id;
  string version;
  string weight;
} ps_font_info_type;


/* Return the information that goes in the FontInfo dictionary for the
   font FONT_NAME, as well as a few other miscellanous things.  We also
   initialize the encoding vector and the `ps_char_output_p' global.  */
extern ps_font_info_type ps_init (string font_name, tfm_global_info_type);

/* Output the starting boilerplate that is the same for Type 1 and Type
   3 fonts to the file F, using the other args for information.  */
extern void ps_start_font (FILE *f, ps_font_info_type ps_info,
                           string comment);

/* Output an encoding vector, using a global vector `ps_char_output_p'
   (declared below) to tell which characters exist in the font.
   `ps_init' must be called before this.  */
extern void ps_output_encoding (FILE *);

/* Return the name corresponding to the character code N in the current
   encoding.  */
extern string ps_encoding_name (charcode_type n);

/* Given the name NAME of a character, find its number in the current
   encoding vector.  If NAME is not defined, return -1.  */
extern int ps_encoding_number (string name);

/* Says whether a particular character has been output.  Initialized by
   `ps_init', and used by `ps_output_encoding', et al.  Defined in
   psutil.c.  */
extern boolean ps_char_output_p[];

#endif /* not PSUTIL_H */
