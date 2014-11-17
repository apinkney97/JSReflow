/* output.h: declarations for outputting the newly spaced font.

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

#ifndef OUTPUT_H
#define OUTPUT_H

#include "font.h"

/* See output.c.  */
extern string encoding_filename;
extern string fontdimens;
extern string output_name;


/* Output a TFM and GF file with the spacings specified in the input file;
   use B for the character definitions.  The font is written to the
   current directory, using the name the user specified.  */
   
extern void output_font (bitmap_font_type b, 
			 char_type chars[MAX_CHARCODE + 1]);

#endif /* not OUTPUT_H */
