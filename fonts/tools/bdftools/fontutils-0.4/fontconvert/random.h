/* random.h: declarations for randomizing a bitmap.

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

#ifndef RANDOM_H
#define RANDOM_H

#include "bitmap.h"
#include "bounding-box.h"
#include "types.h"

/* See random.c.  */
extern real random_max;
extern real random_threshold;

/* Move each pixel in B by a random amount between -random_max and
   random_max (enlarging the bitmap as necessary).  Return the number of
   rows/columns added at each side of bitmap (i.e., all numbers are
   positive) in ADJUST.  (This is not actually a bounding box, but that
   type is a convenient way to return the four numbers.)  */
extern void randomize_bitmap (bitmap_type *b, bounding_box_type *adjust);

#endif /* not RANDOM_H */
