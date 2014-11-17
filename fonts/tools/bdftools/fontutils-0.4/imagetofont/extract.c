/* extract.c: operations on bitmaps.  These could be put in
   kbase/bitmap.c, but since they are not particularly common
   operations, we may as well define them separately for now.

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
#include "bounding-box.h"

#include "bitmap2.h"
#include "extract.h"
#include "main.h"
#include "input-img.h"
#include "input-pbm.h"

static bitmap_type *get_image_row (boolean, unsigned);
static unsigned *image_row_transitions (bitmap_type);




/* Combine the images in the bitmaps B1 and B2, returning the result in
   B1.  The relative position of the bitmaps is given by the bounding
   boxes BB1 and BB2.  We update the bounding box, also.  */

void
combine_images (bitmap_type *b1, bitmap_type b2,
                bounding_box_type *bb1, bounding_box_type bb2)
{
  int this_row, this_col;
  bitmap_type whole_bitmap;
  bounding_box_type whole_bb;
  
  /* If either of the bitmaps are nonexistent, we have nothing
     significant to do.  */
  if (BITMAP_BITS (b2) == NULL)
    return;
  if (BITMAP_BITS (*b1) == NULL)
    {
      *b1 = b2;
      return;
    }

  MIN_ROW (whole_bb) = MIN (MIN_ROW (*bb1), MIN_ROW (bb2));
  MAX_ROW (whole_bb) = MAX (MAX_ROW (*bb1), MAX_ROW (bb2));
  MIN_COL (whole_bb) = MIN (MIN_COL (*bb1), MIN_COL (bb2));
  MAX_COL (whole_bb) = MAX (MAX_COL (*bb1), MAX_COL (bb2));

  whole_bitmap = new_bitmap (bb_to_dimensions (whole_bb));
  
  /* First, copy the first image in.  */
  for (this_row = MIN_ROW (*bb1); this_row <= MAX_ROW (*bb1); this_row++)
    {
      one_byte *target_row
        = BITMAP_BITS (whole_bitmap)
          + (this_row - MIN_ROW (whole_bb)) * BITMAP_WIDTH (whole_bitmap)
          + MIN_COL (*bb1) - MIN_COL (whole_bb);
      one_byte *source_row
        = BITMAP_BITS (*b1)
          + (this_row - MIN_ROW (*bb1)) * BITMAP_WIDTH (*b1);
        
      memcpy (target_row, source_row, BITMAP_WIDTH (*b1));
    }

  /* Put the second image in.  */
  for (this_row = MIN_ROW (bb2); this_row <= MAX_ROW (bb2); this_row++)
    for (this_col = MIN_COL (bb2); this_col < MAX_COL (bb2); this_col++)
      {
        if (BITMAP_PIXEL (b2, this_row - MIN_ROW (bb2),
                          this_col - MIN_COL (bb2)) == BLACK)
          BITMAP_PIXEL (whole_bitmap, this_row - MIN_ROW (whole_bb),
                        this_col - MIN_COL (whole_bb))
            = BLACK;
      }

  free_bitmap (b1);
  *b1 = whole_bitmap;
  *bb1 = whole_bb;
}



/* Get the next image row of width WIDTH, ignoring leading white rows.
   Return the columns at which there are black-to-white or white-black
   transitions in TRANSITIONS.  */

bitmap_type *
some_black_to_all_white_row (unsigned width, unsigned **transitions)
{
  bitmap_type *new = get_image_row (false, width);
  if (new == NULL)
    return NULL;
  
  *transitions = image_row_transitions (*new);
  return new;
}


/* Get the next image row, including leading blank rows, concatenating
   it below B with one blank row between.  Otherwise like
   `some_black_to_all_white_row'.  */

bitmap_type *
append_next_image_row (bitmap_type b, unsigned width, unsigned **transitions)
{
  bitmap_type *new = XTALLOC (1, bitmap_type);
  bitmap_type *next = get_image_row (true, width);

  if (next == NULL)
    return NULL;
  
  *new = bitmap_vconcat (b, *next);
  *transitions = image_row_transitions (*new);
  return new;
}


/* Because scanlines take a relatively long time to read, we will want
   some kind of status report.  */

#define REPORT_SCANLINE()						\
  if (verbose && ++scanline_count % 100 == 0)				\
    {									\
      putc ('.', stderr);						\
      fflush (stderr);							\
    }

/* Read scanlines of width WIDTH from the image file until we get one
   that has at least one black pixel in it (i.e., found part of a
   character).  Then read scanlines until we get one that is entirely
   white.  If KEEP_LEADING_WHITE_ROWS, then include the white rows we
   found at the beginning in the returned bitmap; otherwise, it's just
   the rows from the first-black to the one before the all-white.  */

static bitmap_type *
get_image_row (boolean keep_leading_white_rows, unsigned width)
{
  static unsigned scanline_count = 0;
  bitmap_type *bitmap;
  boolean found_white_row;
  one_byte *image;
  unsigned row_count;
  boolean found_black_row = false;
  one_byte *scanline = xmalloc (width);
  unsigned white_row_count = 0;
  
  while ((*image_get_scanline) (scanline))
    {
      REPORT_SCANLINE ();
      
      found_black_row = memchr (scanline, BLACK, width) != NULL;
      if (found_black_row)
        break;
      
      white_row_count++;
    }
  
  /* If reached end of the image before finding a black row, return NULL.  */
  if (!found_black_row)
    return NULL;
  
  /* Found the row with a black pixel; now look for the all-white row.  */
  found_white_row = false;
  row_count = 1;
  if (keep_leading_white_rows && white_row_count > 0)
    {
      image = xcalloc (white_row_count, width);
      image = xrealloc (image, (white_row_count + 1) * width);
      memcpy (image + white_row_count * width, scanline, width);
    }
  else
    image = scanline;

  while (!found_white_row)
    {
      image = xrealloc (image, (row_count + 1) * width);
      scanline = image + row_count * width;
      if (!(*image_get_scanline) (scanline))
        break;
      REPORT_SCANLINE ();
      row_count++;
      found_white_row = memchr (scanline, BLACK, width) == NULL;
    }

  bitmap = xmalloc (sizeof (bitmap_type));
  BITMAP_WIDTH (*bitmap) = width;
  BITMAP_HEIGHT (*bitmap) = row_count - (found_white_row ? 1 : 0);
  BITMAP_BITS (*bitmap) = image;

  return bitmap;
}

/* Return a vector of the column numbers at which B changes from black
   to white or white to black, terminated with an element which is B's
   width + 1.  */

static unsigned *
image_row_transitions (bitmap_type b)
{
  unsigned *transitions;
  unsigned row, col;
  unsigned width = BITMAP_WIDTH (b);
  one_byte *or_of_all = xcalloc (width, 1);

  for (row = 0; row < BITMAP_HEIGHT (b); row++)
    for (col = 0; col < width; col++)
      or_of_all[col] |= BITMAP_PIXEL (b, row, col);
        
  /* We use `or_of_all' to find transitions: its ith column is 1
     if any row in the image had a black pixel at the ith column.  */
  transitions = bitmap_find_transitions (or_of_all, width);
  
  return transitions;
}
