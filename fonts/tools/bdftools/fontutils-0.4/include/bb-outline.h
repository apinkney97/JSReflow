/* bb-outline.h: find a list of bounding boxes enclosing outlines.

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

#ifndef BB_OUTLINE_H
#define BB_OUTLINE_H

#include "bitmap.h"
#include "types.h"


/* This is a list of bounding boxes enclosing outlines.  */
typedef struct
{
  bounding_box_type *data;
  unsigned length;
} bounding_box_list_type;

/* The length of the list.  */
#define BB_LIST_LENGTH(bb_l)  ((bb_l).length)

/* The array of elements as a whole.  */
#define BB_LIST_DATA(bb_l)  ((bb_l).data)

/* The Nth element in the list.  */
#define BB_LIST_ELT(bb_l, n)  ((bb_l).data[n])


/* Find the bounding boxes around the outlines in the bitmap B.  If ALL is
   true, we find the bounding boxes around all the outlines, including
   counterforms.  If ALL is false, we find only ``outside'' outlines. 
   The character `a', for example, would be represented by one bounding
   box.  We don't look in any of the columns from LEFT to RIGHT,
   left-inclusive.  */
extern bounding_box_list_type
  find_outline_bbs (bitmap_type b, boolean all, int left, int right);

/* Initialize a list.  */
extern bounding_box_list_type init_bounding_box_list (void);

/* Move all the elements in L to the right by OFFSET.  */
extern void offset_bounding_box_list (bounding_box_list_type *l, int offset);

/* Append the elements in list B2 to B1, changing B1.  */
extern void append_bounding_box_list (bounding_box_list_type *B1,
                                      bounding_box_list_type B2);
/* Free the memory in a list.  */
extern void free_bounding_box_list (bounding_box_list_type *);

#endif /* not BB_OUTLINE_H */
