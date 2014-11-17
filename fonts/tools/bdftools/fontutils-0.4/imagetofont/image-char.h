/* image-char.h: information about the characters in the image.

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

#ifndef IMAGE_CHAR_H
#define IMAGE_CHAR_H

#include "types.h"


/* A single character in the image.  */
typedef struct 
{
  one_byte charcode;
  boolean omit;
  int baseline_adjust;
  unsigned bb_count;
  boolean alternating;
  int lsb, rsb;
} image_char_type;

/* The character code.  This is garbage if `omit' is true.  */
#define IMAGE_CHARCODE(c) ((c).charcode)

/* Says whether this character should be output.  */
#define IMAGE_CHAR_OMIT(c) ((c).omit)

/* How far the baseline should be moved from the row's baseline.  */
#define IMAGE_CHAR_BASELINE_ADJUST(c) ((c).baseline_adjust)

/* How many bounding boxes comprise this character.  */
#define IMAGE_CHAR_BB_COUNT(c) ((c).bb_count)

/* Says whether the bounding boxes in this character are consecutive
   (the usual case) or alternate.  */
#define IMAGE_CHAR_BB_ALTERNATING(c) ((c).alternating)

/* The side bearings.  */
#define IMAGE_CHAR_LSB(c) ((c).lsb)
#define IMAGE_CHAR_RSB(c) ((c).rsb)

/* A list of the above.  */
typedef struct
{
  image_char_type *data;
  unsigned length;
} image_char_list_type;

/* The Nth element of the list L.  */
#define IMAGE_CHAR(l, n) ((l).data[n])

/* The list as a whole.  */
#define IMAGE_CHAR_LIST_DATA(l) ((l).data)

/* The length of the list.  */
#define IMAGE_CHAR_LIST_LENGTH(l) ((l).length)


/* Return an empty list.  */
extern image_char_list_type new_image_char_list (void);

/* Append the character C to the list L.  */
extern void append_image_char (image_char_list_type *l, image_char_type c);

/* Return false if the box BOX_COUNT boxes beyond FIRST_CHAR in LIST
   is in the middle of a character, true otherwise.  */
extern boolean box_at_char_boundary_p
  (image_char_list_type list, unsigned first_char, unsigned box_count);

#endif /* not IMAGE_CHAR_H */
