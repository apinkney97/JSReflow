/* bzr.h: manipulate Bezier-format font files.  See ../bzr/README
   for the precise definition of the file format.

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

#ifndef BZR_H
#define BZR_H

#include "bounding-box.h"
#include "font.h"
#include "spline.h"
#include "types.h"


/* At most one output (and one input) file can be open at a time;
   calling either of these routines twice, with no intervening call to
   close the file, results in a fatal error.  */
extern boolean bzr_open_output_file (string filename);
extern boolean bzr_open_input_file (string filename);

/* If no output (or input) file is open, a fatal error results.  */
extern void bzr_close_output_file (void);
extern void bzr_close_input_file (void);



/* Fontwide information at the beginning of the file.  */
typedef struct
{
  string comment;
  real design_size;
} bzr_preamble_type;

/* The comment usually identifies the creator of the file, but it need
   not be anything in particular.  */
#define BZR_COMMENT(p) ((p).comment)

/* The design size is given in points (72.27pt=1in).  */
#define BZR_DESIGN_SIZE(p) ((p).design_size)


/* You should write the preamble before writing anything els.  */
extern void bzr_put_preamble (bzr_preamble_type);

/* You should be positioned at the beginning of the file (e.g., have
   just opened it) before calling this.  Furthermore, you should call
   this routine before calling any of the other input routines. 
   (Because this is what reads the design size of the font, and other
   values in the file are scaled by that.)  */
extern bzr_preamble_type bzr_get_preamble (void);



/* Character information.  */

typedef struct
{
  charcode_type code;
  real set_width;
  real_bounding_box_type bb;
  spline_list_array_type shape;
} bzr_char_type;


/* The character code is always in the range 0 to 255.  You can use the
   `CHARCODE' macro to access it.  */

/* The set width is given in points.  You can use the `CHAR_SET_WIDTH'
   macro to access it.  */

/* The character bounding box is not guaranteed to be the tightest
   possible, but it should be close.  The values are in points.  The
   CHAR_HEIGHT, CHAR_DEPTH, CHAR_LSB, and CHAR_RSB macros work.  */
#define BZR_CHAR_BB(c) ((c).bb)
#define BZR_CHAR_MIN_COL(c) (MIN_COL (BZR_CHAR_BB (c)))
#define BZR_CHAR_MAX_COL(c) (MAX_COL (BZR_CHAR_BB (c)))
#define BZR_CHAR_MIN_ROW(c) (MIN_ROW (BZR_CHAR_BB (c)))
#define BZR_CHAR_MAX_ROW(c) (MAX_ROW (BZR_CHAR_BB (c)))

/* The shape is given as a list of lists of lines and cubic splines.
   Each element of the outer list represents a closed path.  The values
   are in points.  */
#define BZR_SHAPE(c) ((c).shape)


/* This routine writes the given character.  It recomputes the tightest
   bounding box, so that element of the structure need not be set.  */
extern void bzr_put_char (bzr_char_type);

/* This routine returns the next character in the file.  If the postamble
   is next, it returns NULL.  Each spline in the `shape' element is
   guaranteed to have degree `LINEAR' or `CUBIC' (see `spline.h').  */
extern bzr_char_type *bzr_get_next_char (void);

/* This routine returns the character with the given CODE, or NULL.
   Each spline in the `shape' element is guaranteed to have degree
   `LINEAR' or `CUBIC' (see `spline.h').  */
extern bzr_char_type *bzr_get_char (one_byte code);



/* More fontwide information, this at the end of the file.  */

typedef struct
{
  real_bounding_box_type font_bb;
  one_byte nchars;
} bzr_postamble_type;

/* The font bounding box is also guaranteed to be the tightest possible.
   The values are in points.  */
#define BZR_FONT_BB(p) ((p).font_bb)

/* The total number of characters (possibly zero) in the font.  */
#define BZR_NCHARS(p) ((p).nchars)


/* The postamble must be written last; the library itself determines the
   font bounding box, based on the character bounding boxes, so you do
   not supply it as an argument.  */
extern void bzr_put_postamble (void);

/* And this reads the postamble back.  You should be positioned at the
   postamble (e.g., `bzr_get_char' should have just returned NULL) before
   calling this.  */
extern bzr_postamble_type bzr_get_postamble (void);

#endif /* not BZR_H */
