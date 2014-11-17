/* random.c: distort a bitmap.

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

#include "random.h"

#ifndef STDC_HEADERS
extern int rand (), srand ();
#endif
#ifndef RAND_MAX
#define RAND_MAX ((1 << 31) - 1)
#endif

/* Distort the bitmap (see below).  (-randomize)  */
real random_max = 0.0;
real random_threshold = 0.2;


static bitmap_type enlarge_bitmap (bitmap_type, int, int);
static real random_number (void);



/* Randomize the bitmap B by moving each black pixel a randomly chosen
   distance between -random_max and random_max (both horizontally and
   vertically).  Return the enlargement of B on all four side in ADJUST.
   */

void
randomize_bitmap (bitmap_type *b, bounding_box_type *adjust)
{
  unsigned this_row, this_col;

  MIN_ROW (*adjust) = MAX_ROW (*adjust) = MIN_COL (*adjust) = MAX_COL (*adjust)
    = 0;

  for (this_row = 0; this_row < BITMAP_HEIGHT (*b); this_row++)
    for (this_col = 0; this_col < BITMAP_WIDTH (*b); this_col++)
      {
        if (BITMAP_PIXEL (*b, this_row, this_col) != WHITE)
          {
            int row_enlarge, col_enlarge;
            /* Generate two random numbers between -random_max and
               random_max to give rise to the new x and y.  */
            real random1 = random_number ();
            int row_delta = random1 + .5 * SIGN (random1);
            int new_row = this_row + row_delta;

            real random2 = random_number ();
            int col_delta = random2 + .5 * SIGN (random2);
            int new_col = this_col + col_delta;

            /* Clear the old location before we (possibly) mess with the
               bitmap dimensions.  */
            BITMAP_PIXEL (*b, this_row, this_col) = WHITE;

            /* Check if the new location is outside the current bitmap.  */
            if (new_row < 0)
              {
                row_enlarge = new_row;
                /* Negative rows are off the top of the bitmap -- it's
                   in C coordinates, not Cartesian.  */
                MAX_ROW (*adjust) -= row_enlarge;
              }
            else if (new_row >= BITMAP_HEIGHT (*b))
              {
                row_enlarge = new_row - BITMAP_HEIGHT (*b) + 1;
                MIN_ROW (*adjust) += row_enlarge;
              }
            else
              row_enlarge = 0;

            if (new_col < 0)
              {
                col_enlarge = new_col;
                MIN_COL (*adjust) -= col_enlarge;
              }
            else if (new_col >= BITMAP_WIDTH (*b))
              {
                col_enlarge = new_col - BITMAP_WIDTH (*b) + 1;
                MAX_COL (*adjust) += col_enlarge;
              }
            else
              col_enlarge = 0;

            if (row_delta != 0 || col_delta != 0)
              { /* Enlarge the bitmap to make the new location valid.  */
                bitmap_type new_b
                  = enlarge_bitmap (*b, row_enlarge, col_enlarge);
                free_bitmap (b);
                *b = new_b;
                
                /* We've made the new row/column at the edge.  */
                if (new_row < 0) new_row = 0;
                if (new_col < 0) new_col = 0;
              }

            BITMAP_PIXEL (*b, new_row, new_col) = BLACK;
          }
      }
}


/* Return the enlargement of the bitmap B by ROW_DELTA and COL_DELTA. 
   Negative deltas enlarge to the bottom/left, positive to the
   top/right.  */

static bitmap_type
enlarge_bitmap (bitmap_type b, int row_delta, int col_delta)
{
  bitmap_type new;
  dimensions_type new_size;
  unsigned row;
  int row_addend = row_delta < 0 ? abs (row_delta) : 0;
  int col_addend = col_delta < 0 ? abs (col_delta) : 0;
  
  DIMENSIONS_HEIGHT (new_size) = BITMAP_HEIGHT (b) + abs (row_delta);
  DIMENSIONS_WIDTH (new_size) = BITMAP_WIDTH (b) + abs (col_delta);
  
  new = new_bitmap (new_size);
  
  /* `new_bitmap' initializes all the pixels to white, so we don't have
     to do anything to the new rows and columns.  */
  for (row = 0; row < BITMAP_HEIGHT (b); row++)
    {
      one_byte *source = BITMAP_ROW (b, row);
      one_byte *target = BITMAP_ROW (new, row + row_addend) + col_addend;
      
      memcpy (target, source, BITMAP_WIDTH (b));
    }
  
  return new;
}



/* Return a random number between -random_max and random_max.  */

static real
random_number ()
{
  static boolean initialized = false;
  real r;
  
  if (!initialized)
    {
      srand (/* getpid ()*/0);
      initialized = true;
    }

  /* If the random number is less than `random_threshold', return zero.
     Otherwise, return a random number in the interval
     [-random_max,random_max].  */
  r = (real) rand () / RAND_MAX;
  r = r > random_threshold
      ? (real) rand () / RAND_MAX * random_max - random_max
      : 0;

  return r;
}
