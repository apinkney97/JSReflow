/* bounding-box.c: definitions for bounding box operations.

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

#include "bounding-box.h"


/* See the comments at `get_character_bitmap' in gf_input.c for why the
   width and height are treated asymmetrically.  */

const bounding_box_type
dimensions_to_bb (dimensions_type dimensions)
{
  bounding_box_type answer;

  MIN_ROW (answer) = 0;
  MIN_COL (answer) = 0;
  MAX_ROW (answer) = MAX ((int) DIMENSIONS_HEIGHT (dimensions) - 1, 0);
  MAX_COL (answer) = MAX (DIMENSIONS_WIDTH (dimensions), 0);

  return answer;
}


/* Dimensions can't be negative, even though bounding boxes can, so we
   have to check.  */
   
const dimensions_type
bb_to_dimensions (bounding_box_type bb)
{
  dimensions_type d;
  
  DIMENSIONS_WIDTH (d) = MAX (BB_WIDTH (bb), 0);
  DIMENSIONS_HEIGHT (d) = MAX (BB_HEIGHT (bb), 0);
  
  return d;
}


/* If the point P lies outside of any of the current bounds in BB,
   update BB.  */

void
update_real_bounding_box (real_bounding_box_type *bb, real_coordinate_type p)
{
  if (p.x < MIN_COL (*bb)) MIN_COL (*bb) = p.x;
  if (p.x > MAX_COL (*bb)) MAX_COL (*bb) = p.x;
  if (p.y < MIN_ROW (*bb)) MIN_ROW (*bb) = p.y;
  if (p.y > MAX_ROW (*bb)) MAX_ROW (*bb) = p.y;
    
}


void
update_bounding_box (bounding_box_type *bb, coordinate_type p)
{
  if (p.x < MIN_COL (*bb)) MIN_COL (*bb) = p.x;
  if (p.x > MAX_COL (*bb)) MAX_COL (*bb) = p.x;
  if (p.y < MIN_ROW (*bb)) MIN_ROW (*bb) = p.y;
  if (p.y > MAX_ROW (*bb)) MAX_ROW (*bb) = p.y;
    
}
