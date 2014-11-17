/* gf.h: manipulate generic font files.  See Metafont: The Program, by
   Don Knuth, (Volume D of Computers & Typesetting), chapter 46, among
   other places, for the precise definition of this bitmap format.

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

#ifndef GENERIC_FONT_LIBRARY_H
#define GENERIC_FONT_LIBRARY_H

#include "bitmap.h"
#include "bounding-box.h"
#include "font.h"
#include "types.h"


/* At most one GF file can be open for reading (or writing) at a time.
   You must call `gf_open_input_file' before using any of the `gf_get...'
   routines; similarly for the output side.  */

extern boolean gf_open_input_file (string filename);
extern void gf_close_input_file (void);

extern boolean gf_open_output_file (string filename);
extern void gf_close_output_file (void);



/* The only interesting information in the preamble is the comment.  */

extern string gf_get_preamble (void);
extern void gf_put_preamble (string comment);


/* An important part of the postamble is the character locators.  A
   character exists in the GF file if its `char_pointer' member in this
   structure is not `NULL_BYTE_PTR'.  */
typedef struct
{
  one_byte charcode;
  signed_4_bytes h_escapement;
  fix_word tfm_width;
  signed_4_bytes char_pointer;
} gf_char_locator_type;


typedef struct
{
  fix_word design_size;
  unsigned checksum;
  real h_resolution, v_resolution;
  bounding_box_type font_bb;
  gf_char_locator_type char_loc[MAX_CHARCODE + 1];
} gf_postamble_type;

/* The design size is given as a fix_word in TeX points.  */
#define GF_DESIGN_SIZE(p)  ((p).design_size)

/* The GF checksum should match the checksums in the TFM file and/or the
   PK file, if they exist.  */
#define GF_CHECKSUM(p)  ((p).checksum)

/* We express the resolutions in pixels per point multiplied by 2^16.  */
#define GF_H_RESOLUTION(p)  ((p).h_resolution)
#define GF_V_RESOLUTION(p)  ((p).v_resolution)

/* The font bounding box may not be the tightest possible.  */
#define GF_FONT_BB(p)  ((p).font_bb)

/* An abbreviation for a particular character locator.  */
#define GF_CHAR_LOC(p, code)  ((p).char_loc[code])


/* Return the postamble in the input file.  */
extern gf_postamble_type gf_get_postamble (void);

/* Write the postamble to the output file.  The library fills in the
   information that is not given as arguments.  This must be called
   after all the characters have been written.  The resolution arguments
   should be given in pixels per inch.  */
extern void gf_put_postamble (fix_word design_size,
                              real h_resolution, real v_resolution);


/* The characters are the most important information in the GF file.  */

typedef struct
{
  one_byte code;
  bitmap_type bitmap;
  bounding_box_type bb;
  signed_4_bytes h_escapement;
  fix_word tfm_width;
} gf_char_type;

/* GF format actually allows character codes to be a full four bytes
   long, but we cannot deal with such fonts.  */
#define GF_CHARCODE(gc)  ((gc).code)

/* The pixels.  See `bitmap.h'.  */
#define GF_BITMAP(gc)  ((gc).bitmap)

/* GF format does not guarantee that the bounding box is the tightest
   possible, but the reading routines do.  */
#define GF_CHAR_BB(gc)  ((gc).bb)

/* The set width, in pixels.  */
#define GF_H_ESCAPEMENT(gc)  ((gc).h_escapement)

/* The character width as a fix_word.  */
#define GF_TFM_WIDTH(gc) ((gc).tfm_width)

/* Conveniently access each member of the bounding box.  */
#define GF_CHAR_MIN_COL(gc)  (MIN_COL (GF_CHAR_BB (gc)))
#define GF_CHAR_MAX_COL(gc)  (MAX_COL (GF_CHAR_BB (gc)))
#define GF_CHAR_MIN_ROW(gc)  (MIN_ROW (GF_CHAR_BB (gc)))
#define GF_CHAR_MAX_ROW(gc)  (MAX_ROW (GF_CHAR_BB (gc)))

/* An abbreviation for the left side bearing ...  */
#define GF_CHAR_LSB  GF_CHAR_MIN_COL

/* ... and one for the right side bearing.  */
#define GF_CHAR_RSB(c) (GF_H_ESCAPEMENT (c) - GF_CHAR_MAX_COL (c))


/* `gf_get_next_char' reads the next character from the input file and returns
   it.  It also returns (as an argument) whether a character was
   actually found. If not, you've read to the postamble.  */
extern gf_char_type gf_get_next_char (boolean *found);

/* `gf_get_char' returns a pointer to the character numbered CODE
   in the input file, or a null pointer if that character doesn't exist.  */ 
extern gf_char_type *gf_get_char (one_byte code);

/* Read the character CODE but don't interpret it; the result is only
   useful as a parameter to `gf_put_raw_char'.  */
extern raw_char_type *gf_get_raw_char (one_byte code);

/* Write the given character to the output file.  */
extern void gf_put_char (gf_char_type);

/* Write the given raw character.  */
extern void gf_put_raw_char (raw_char_type);

#endif /* not GENERIC_FONT_LIBRARY_H */
