/* bitmap2.h: declare extra bitmap manipulation routines.

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

#ifndef BITMAP2_H
#define BITMAP2_H

#include "bitmap.h"


/* Put B2 to the right of B1, changing B2.  The two bitmaps must have
   the same number of rows.  */
extern void bitmap_concat (bitmap_type *b1, bitmap_type b2);


/* Put B2 below B1.  The two bitmaps must have the same number of
   columns.  */
extern bitmap_type bitmap_vconcat (bitmap_type b1, bitmap_type b2);

#endif /* not BITMAP2_H */
