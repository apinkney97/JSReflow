/* pk.h: manipulate packed format font files.  See the PKtype source
   code (by Tomas Rokicki), among other places, for the precise
   definition of this bitmap format.

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

#ifndef PACKED_FONT_LIBRARY_H
#define PACKED_FONT_LIBRARY_H

#include "bounding-box.h"
#include "font.h"
#include "types.h"


/* Prepare and finish off FONT_NAME for reading.  */
extern boolean pk_open_input_file (string font_name);
extern void pk_close_input_file (string font_name);

/* The global information about the PK file is given in the preamble.  */
typedef struct
{
  string comment;
  fix_word design_size;
  unsigned checksum;
  double h_resolution, v_resolution;
} pk_preamble_type;

/* This usually indicates the source of the PK file.  */
#define PK_COMMENT(p)  ((p).comment)

/* The font's design size is in TeX points, expressed as a fix_word.  */
#define PK_DESIGN_SIZE(p)  ((p).design_size)

/* The PK checksum should match the checksums in the TFM file and/or the
   GF file, if they exist.  */
#define PK_CHECKSUM(p)  ((p).checksum)

/* We express the resolutions in pixels per point multiplied by 2^16.  */
#define PK_H_RESOLUTION(p)  ((p).h_resolution)
#define PK_V_RESOLUTION(p)  ((p).v_resolution)

/* Return the above structure for the font FONT_NAME.  */
extern pk_preamble_type pk_get_preamble (string font_name);



/* The characters comprise the bulk of the file.  */
typedef struct
{
  one_byte code;
  fix_word tfm_width;
  signed_4_bytes h_escapement;
  bounding_box_type bb;
  bitmap_type bitmap;
} pk_char_type;

/* The library guarantees that the character code is in the range 0 to
   255 (although the PK format does not require that).  */
#define PK_CHARCODE(c)  ((c).code)

/* The TFM width is the character's true width divided by the design
   size (expressed as a fix_word).  */
#define PK_TFM_WIDTH(c)  ((c).tfm_width)

/* The horizontal escapement (i.e., set width) is in pixels.  */
#define PK_H_ESCAPEMENT(c)  ((c).h_escapement)

/* The bounding box is guaranteed to be the tightest possible.  */
#define PK_CHAR_BB(c)  ((c).bb)

/* The bits themselves.  See `bitmap.h'.  */
#define PK_BITMAP(c)  ((c).bitmap)


/* Allocate and return a pointer to the above structure for the
   character CODE in the font FONT_NAME, or NULL if that character is
   not in that font.  */
extern pk_char_type *pk_get_char (one_byte code, string font_name);

#endif /* not PACKED_FONT_LIBRARY_H */
