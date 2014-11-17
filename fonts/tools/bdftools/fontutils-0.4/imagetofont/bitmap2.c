/* bitmap2.c: extra bitmap manipulation routines.

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

#include "config.h"

#include "bitmap.h"

#include "bitmap2.h"



/* Put B2 to the right of B1, changing B1.  For example, if B1 looked
   like `A' and B2 like `B', the result would look like `AB'.  The two
   bitmaps must have the same number of rows.  */

void
bitmap_concat (bitmap_type *b1, bitmap_type b2)
{
  bitmap_type new;
  dimensions_type new_dimens;
  unsigned row;
  
  assert (BITMAP_HEIGHT (*b1) == BITMAP_HEIGHT (b2));
  
  DIMENSIONS_WIDTH (new_dimens) = BITMAP_WIDTH (*b1) + BITMAP_WIDTH (b2);
  DIMENSIONS_HEIGHT (new_dimens) = BITMAP_HEIGHT (b2);
  new = new_bitmap (new_dimens);
  
  for (row = 0; row < BITMAP_HEIGHT (b2); row++)
    {
      one_byte *target1 = BITMAP_ROW (new, row);
      one_byte *target2 = target1 + BITMAP_WIDTH (*b1);
      
      memcpy (target1, BITMAP_ROW (*b1, row), BITMAP_WIDTH (*b1));
      memcpy (target2, BITMAP_ROW (b2, row), BITMAP_WIDTH (b2));
    }
  
  free_bitmap (b1);
  *b1 = new;
}


/* Return a bitmap which consists of B1, one blank row, and B2.  The two
   bitmaps must have the same width.  (The blank row is necessary
   because one blank row is removed the first time we call
   some_black_to_all_white; consider the row consisting of just an `!'.)*/

bitmap_type
bitmap_vconcat (bitmap_type b1, bitmap_type b2)
{
  bitmap_type new;
  dimensions_type new_dimens;
  unsigned width = BITMAP_WIDTH (b1);
  unsigned size1 = width * BITMAP_HEIGHT (b1);
  unsigned size2 = width * BITMAP_HEIGHT (b2);
  
  assert (width == BITMAP_WIDTH (b2));
  
  DIMENSIONS_WIDTH (new_dimens) = width;
  DIMENSIONS_HEIGHT (new_dimens) = BITMAP_HEIGHT (b1) + BITMAP_HEIGHT (b2) + 1;
  new = new_bitmap (new_dimens);
  
  memcpy (BITMAP_BITS (new), BITMAP_BITS (b1), size1);
  memcpy (BITMAP_BITS (new) + size1 + width, BITMAP_BITS (b2), size2);

  return new;
}
