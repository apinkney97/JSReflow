/* filter.c: run an averaging filter over a bitmap.
   This code is based on code written by Richard Murphey.

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

#include "filter.h"

/* How many passes to make over the bitmap.  (-filter-passes)  */
unsigned filter_passes = 0;

/* How many rows and columns of neighbors to look at; i.e., a side of
   the filter cell is `filter_size' * 2 - 1.  Although only values > 0
   are meaningful, if we make it unsigned, comparisons to ints below
   fail.  (-filter-size)  */
int filter_size = 1;

/* The filter threshold at which we change the pixel.
   (-filter-threshold)  */
real filter_threshold = .5;


static void filter_once (bitmap_type);



/* Filter the bitmap B `filter_passes' times.  */

void
filter_bitmap (bitmap_type b)
{
  unsigned this_pass;
  
  for (this_pass = 0; this_pass < filter_passes; this_pass++)
    filter_once (b);
}


/* Filter the bitmap in B with an averaging filter.  See, for example,
   Digital Image Processing, by Rosenfeld and Kak.  The general idea is
   to average the intensity of the neighbors of each pixel.  If the
   difference between the pixel value and the intensity is more than
   `filter_threshold', flip the value of the pixel.  */

static void
filter_once (bitmap_type b)
{
  unsigned row, col;
  unsigned all_black = SQUARE (2 * filter_size + 1) - 1;
  unsigned t = filter_threshold * all_black; /* Rounded threshold.  */
  
  /* For each pixel in the bitmap...  */
  for (row = filter_size; row < BITMAP_HEIGHT (b) - filter_size; row++)
    for (col = filter_size; col < BITMAP_WIDTH (b) - filter_size; col++)
      {
        int cell_row, cell_col;
        unsigned sum = 0;
        
        /* For each pixel in the cell...  */
        for (cell_row = -filter_size; cell_row <= filter_size;
             cell_row++)
          for (cell_col = -filter_size; cell_col <= filter_size;
               cell_col++) 
            {
              /*assert (VALID_LOCATION (b, row + cell_row, col + cell_col));*/
              /* If center pixel, don't do anything.  */
              if (cell_row != 0 || cell_col != 0)
                sum += BITMAP_PIXEL (b, row + cell_row, col + cell_col);
            }
        
        /* We've computed the sum of the neighbors.  Now change the
           pixel if the difference is greater than the threshold.  Doing
           integer arithmetic is not as precise as floating point (since
           the threshold is rounded), but it's much faster.  */
        if (abs (sum - (all_black * BITMAP_PIXEL (b, row, col))) > t)
          BITMAP_PIXEL (b, row, col) = ((double) sum) / all_black;
      }
}
