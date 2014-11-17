/* font.h: operations on fonts independent of a particular file format.  

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

#ifndef FONT_LIBRARY_H
#define FONT_LIBRARY_H

#include <stdio.h>
#include "bitmap.h"
#include "bounding-box.h"
#include "tfm.h"
#include "types.h"


/* A font is a collection of characters, generally at a particular point
   size and resolution.  The `bitmap_font_type' structure holds
   information that comes entirely from a bitmap file.  */
typedef struct
{
  real design_size;
  string comment;
  unsigned checksum;
  string filename;  
} bitmap_font_type;

/* The design size is given in points.  It is not necessarily
   related to the actual dimensions of any of the characters in the
   font; indeed, usually the design size is quite a bit larger than the
   largest characters.  */
#define BITMAP_FONT_DESIGN_SIZE(f) ((f).design_size)

/* Sometimes a font comes with a comment identifying its source, who
   owns the trademark, or some such.  */
#define BITMAP_FONT_COMMENT(f) ((f).comment)

/* The checksum found in the bitmap file.  */
#define BITMAP_FONT_CHECKSUM(f) ((f).checksum)

/* The full pathname for the file that we opened.  */
#define BITMAP_FONT_FILENAME(f) ((f).filename)


/* Look for a font named FONT_NAME in PK or GF format, at resolution
   DPI.  This ignores any extension in FONT_NAME; it adds `.<dpi>gf' or
   `.<dpi>pk'.  The font is looked for using the PKFONTS, GFFONTS, and
   TEXFONTS environment variables, just as with the TeX software.  If
   the font can't be found, this gives a fatal error.  */
extern bitmap_font_type get_bitmap_font (string font_name, unsigned dpi);

/* `close_font', below, works for bitmap fonts, too.  */




/* The `font_info_type' holds a `bitmap_font_type', and additional
   information that we get from a font metric file.  */
typedef struct
{
  bitmap_font_type bitmap_font;
  tfm_global_info_type tfm_font;
  string tfm_filename;
  unsigned nchars;
  int interword_space;
} font_info_type;

/* The bitmap information that is associated with this font.  */
#define FONT_BITMAP_FONT(f) ((f).bitmap_font)

/* The TFM information.  */
#define FONT_TFM_FONT(f) ((f).tfm_font)

/* The full pathname for the TFM file that we opened.  */
#define FONT_TFM_FILENAME(f) ((f).tfm_filename)

/* The design size is given in points.  It is not necessarily
   related to the actual dimensions of any of the characters in the
   font; indeed, usually the design size is quite a bit larger than the
   largest characters.  */
#define FONT_DESIGN_SIZE(f) BITMAP_FONT_DESIGN_SIZE ((f).bitmap_font)

/* Sometimes a font comes with a comment identifying its source, who
   owns the trademark, or some such.  */
#define FONT_COMMENT(f) BITMAP_FONT_COMMENT ((f).bitmap_font)

/* The number of characters in the font.  */
#define FONT_NCHARS(f) ((f).nchars)

/* The normal interword space in the font, in pixels.  This ignores any
   stretch and shrink in the interword space that might be present (in a
   TeX font).  This value has nothing to do with the character at
   position 32.  */
#define FONT_SPACE(f) ((f).interword_space)


/* This calls `get_bitmap_font'; in addition, it looks for FONT_NAME in
   TFM format.  If either the bitmap file or the metric file can't be
   found, this gives a fatal error.  */
extern font_info_type get_font (string font_name, unsigned dpi);

/* Close any open files associated with FONT_NAME, whether it was opened
   with `get_bitmap_font' or `get_font'.  */ 
extern void close_font (string font_name);




/* A character is a bitmap image, and is therefore at a particular size
   and resolution.  It is also at a particular position in a font.
   Generally, characters also have `side bearings'---extra space at the
   left and/or right of the character.  (In some scripts, there is
   also displacement above and/or below the character, but we don't take
   that into account here.)  */

typedef struct
{
  charcode_type code;
  int set_width;
  fix_word tfm_width;
  bounding_box_type bb;
  bitmap_type bitmap;
} char_info_type;

/* The character code in our fonts is always between 0 and 255.  */
#define CHARCODE(c) ((c).code)

/* The set width is given in pixels; it's the sum of the left side
   bearing, the bitmap's width, and the right side bearing.  */
#define CHAR_SET_WIDTH(c) ((c).set_width)

/* The TFM width is the character's true width divided by the design
   size (expressed as a fix_word).  */
#define CHAR_TFM_WIDTH(c) ((c).tfm_width)

/* Unlike the font's bounding box, the character bounding box is
   guaranteed to be the tightest possible; i.e., no all-blank rows occur
   at the top or bottom, and no all-blank columns occur at the left or
   right.  */
#define CHAR_BB(c) ((c).bb)

/* The pixels.  See `bitmap.h'.  */
#define CHAR_BITMAP(c) ((c).bitmap)


/* Abbreviations for the width and height of a character's bitmap.  */
#define CHAR_BITMAP_WIDTH(c) BITMAP_WIDTH (CHAR_BITMAP (c))
#define CHAR_BITMAP_HEIGHT(c) BITMAP_HEIGHT (CHAR_BITMAP (c))

/* Abbreviations for the parts of the character's bounding box.  */
#define CHAR_MIN_COL(c) MIN_COL (CHAR_BB (c))
#define CHAR_MAX_COL(c) MAX_COL (CHAR_BB (c))
#define CHAR_MIN_ROW(c) MIN_ROW (CHAR_BB (c))
#define CHAR_MAX_ROW(c) MAX_ROW (CHAR_BB (c))


/* The height of a character is how far it extends above the baseline,
   including the row of pixels on the baseline.  */
#define CHAR_HEIGHT(c) (CHAR_MAX_ROW (c) >= 0 ? CHAR_MAX_ROW (c) + 1 : 0)

/* The depth is far it extends below the baseline (but as a positive
   number, e.g., if a character's bitmap goes down to row -4, the depth
   is 4).  */
#define CHAR_DEPTH(c) (CHAR_MIN_ROW (c) < 0 ? -CHAR_MIN_ROW (c) : 0)


/* An abbreviation for the left side bearing ...  */
#define CHAR_LSB  CHAR_MIN_COL

/* ... and one for the right side bearing.  */
#define CHAR_RSB(c) (CHAR_SET_WIDTH (c) - CHAR_MAX_COL (c))


/* Return the character numbered CODE in the font FONT_NAME, or NULL if
   that character doesn't exist in that font.  If `get_font' or
   `get_bitmap_font' has not been previously called on FONT_NAME,
   `get_char' gives a fatal error.  */
extern char_info_type *get_char (string font_name, charcode_type code);


/* Print a plain text representation of the character C to the file F.  */
extern void print_char (FILE *f, char_info_type c);


/* Typeset the TEXT in the font FONT_NAME, at a resolution of DPI.  */
extern bitmap_type string_to_bitmap (string text,
                                     string font_name, unsigned dpi);


/* A raw character is the byte string that defines the character in the
   font file, in some format.  */

typedef enum { pk_format, gf_format } bitmap_format_type;

typedef struct
{
  bitmap_format_type bitmap_format;
  charcode_type code;
  one_byte *bytes;
  unsigned allocated;
  unsigned used;
  bounding_box_type bb;
  signed_4_bytes h_escapement;
  fix_word tfm_width;
} raw_char_type;

/* CHARCODE works to access the character code of a `raw_char_type'
   variable.  Likewise for the bounding box, horizontal escapement, and
   TFM width.  */

/* The file format the character definition was read from.  */
#define RAW_CHAR_BITMAP_FORMAT(rc) ((rc).bitmap_format)

/* The bytes in the file that comprise the character definition.  */
#define RAW_CHAR_BYTES(rc) ((rc).bytes)

/* The number of allocated bytes to which the buffer points.  */
#define RAW_CHAR_ALLOCATED(rc) ((rc).allocated)

/* The number of bytes actually used.  */
#define RAW_CHAR_USED(rc) ((rc).used)

/* A convenience macro for the first unused byte.  */
#define RAW_CHAR_UNUSED_START(rc) RAW_CHAR_BYTES (rc)[RAW_CHAR_USED (rc)]


/* Return the raw character numbered CODE in the font named FONT_NAME,
   or NULL.  The only useful operation on a raw character is to write it
   to an output file with the appropriate format.  */
extern raw_char_type *get_raw_char (string font_name,
                                    charcode_type code);

/* Free all allocated storage in the raw character RAW_CHAR, including
   the character itself.  */
extern void free_raw_char (raw_char_type *raw_char);




/* Miscellanous constants.  */

/* We will only deal with fonts that have at most this many characters,
   although the bitmap formats allow more.  */
#define MAX_CHARCODE 255

/* Some font formats have pointers within the file (to other places in
   the file).  The null value for such pointers is -1.  */
#define NULL_BYTE_PTR (-1)

#endif /* not FONT_LIBRARY_H */
