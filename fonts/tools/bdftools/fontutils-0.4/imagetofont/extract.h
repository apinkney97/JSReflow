/* extract.h: declare operations on bitmaps.

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

#ifndef EXTRACT_H
#define EXTRACT_H

#include "bitmap.h"
#include "bounding-box.h"


/* Return the segment in B starting after the column START that begins
   with a column with at least one black pixel, to a column that is all
   white (or the end of the bitmap).  If no such segment is found,
   return NULL.  */
extern bitmap_type *some_black_to_all_white_column (bitmap_type b);

/* Read scanlines of width WIDTH (in pixels) from the input file until
   we find a segment starting with a row with at least one black pixel,
   and ending with a row that is all white (or the end of the file).  If
   no such segment is found, return NULL.  In TRANSITIONS we return the
   column numbers where white changes to black or black to white.  The
   first transition is black-to-white.  */
extern bitmap_type *some_black_to_all_white_row (unsigned width,
                                                 unsigned **transitions);

/* Like `some_black_to_all_white_row', except appends both the white
   rows and then the black ones to B.  */
extern bitmap_type *append_next_image_row
  (bitmap_type b, unsigned width, unsigned **transitions);

/* Combine the bitmaps B1 and B2 based on the bounding boxes, returning
   the result in B1.  Also combine the bounding boxes.  */
extern void combine_images (bitmap_type *b1, bitmap_type b2,
                            bounding_box_type *, bounding_box_type);
#endif /* not EXTRACT_H */
