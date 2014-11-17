/* bb-outline.c: find the bounding boxes enclosing outlines.

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

#include "bb-outline.h"


typedef enum
{
  left = 0, top = 1, right = 2, bottom = 3, no_edge = 4
} edge_type;

#define NUM_EDGES  4
#define START_EDGE  top

static bounding_box_type find_one_bb (bitmap_type, edge_type,
                                      unsigned row, unsigned col,
                                      bitmap_type *marked);
static void next_outline_edge (bitmap_type, edge_type *, 
                               unsigned *row, unsigned *col);

/* Edge routines.  */
static edge_type next_unmarked_outline_edge (unsigned row, unsigned col,
			                     edge_type edge, bitmap_type ch,
                                             bitmap_type marked);
static boolean is_outline_edge (edge_type edge, bitmap_type character,
                                unsigned row, unsigned col);
static void mark_edge (edge_type edge, unsigned row, unsigned col,
		       bitmap_type *marked);
static boolean is_marked_edge (edge_type edge, unsigned row,
			       unsigned col, bitmap_type marked);
static edge_type next_edge (edge_type edge);
static string edge_name (edge_type edge);

static void append_bb (bounding_box_list_type *, bounding_box_type);



/* A character is made up of a list of one or more outlines.  Here, we
   go through a character's bitmap top to bottom, left to right, looking
   for the next pixel with an unmarked edge also on the character's outline.
   Each one of these we find is the starting place for one outline.  We
   get the bounding boxes of the outlines, and put them in a list to
   return.  We don't look at pixels that are at or after the column
   LEFT_MARK and before RIGHT_MARK.  */

bounding_box_list_type
find_outline_bbs (bitmap_type b, boolean find_all,
                  int left_mark, int right_mark)
{
  unsigned row, col;
  bounding_box_list_type bb_list = init_bounding_box_list ();
  bitmap_type marked = new_bitmap (BITMAP_DIMENSIONS (b));
  int right_edge = INT_MIN;

  /* By looking through the bitmap in column-major order, we preserve
     the original ordering of the characters in the image.  Otherwise,
     `b' would be found before `a' (for example), because of the
     ascender.  */
  for (col = 0; col < BITMAP_WIDTH (b); col++)
    if (col < left_mark || col >= right_mark)
      for (row = 0; row < BITMAP_HEIGHT (b); row++)
        {
          edge_type edge;
          boolean found;
          unsigned this_bb;

          if (BITMAP_PIXEL (b, row, col) == WHITE)
            continue;

          /* If we're past the rightmost place we've been, we can't
             possibly be inside any bounding boxes we've already found.  */
          if (!find_all && col <= right_edge)
            {
              /* Don't look for outlines inside an outline we've already
                 found.  */
              for (found = false, this_bb = 0;
                   !found && this_bb < BB_LIST_LENGTH (bb_list);
                   this_bb++)
                {
                  bounding_box_type bb = BB_LIST_ELT (bb_list, this_bb);
                  found = MIN_COL (bb) <= col && col <= MAX_COL (bb)
                          && MIN_ROW (bb) <= row && row <= MAX_ROW (bb);
                }
              if (found)
                continue;
            }

          edge = next_unmarked_outline_edge (row, col, START_EDGE, b, marked);

          if (edge != no_edge)
            {
              bounding_box_type bb = find_one_bb (b, edge, row, col, &marked);
              append_bb (&bb_list, bb);
              if (MAX_COL (bb) > right_edge)
                right_edge = MAX_COL (bb);
            }
        }

  free_bitmap (&marked);
  return bb_list;
}


/* Here we find one of a character's outlines.  We're passed the
   position (ORIGINAL_ROW and ORIGINAL_COL) of a starting pixel and one
   of its unmarked edges, STARTING_EDGE.  We traverse the adjacent edges
   of the outline pixels but keep the bounding box in pixel coordinates.
   We keep track of the marked edges in MARKED, so it should be
   initialized to zeros when we first get it.  */

static bounding_box_type
find_one_bb (bitmap_type b, edge_type original_edge,
             unsigned original_row, unsigned original_col,
             bitmap_type *marked)
{
  bounding_box_type bb = { INT_MAX, INT_MIN, INT_MAX, INT_MIN };
  unsigned row = original_row, col = original_col;
  edge_type edge = original_edge;

  do
    {
      coordinate_type p = { col, row };
      update_bounding_box (&bb, p);

      mark_edge (edge, row, col, marked);
      next_outline_edge (b, &edge, &row, &col);
    }
  while (!(row == original_row && col == original_col
           && edge == original_edge));

  return bb;
}



/* (These routines are copied unmodified from the ones in
    `limn/pxl-outline.c'.)  */

/* We look first for an adjacent edge (on an outline) that is not on the
   current pixel.  We do this by first looking at the edge perpendicular
   to EDGE (clockwise), then looking at the edge parallel to EDGE.  If
   we find such an edge, we return (in ROW and COL) the position of the
   pixel it's on, and (in EDGE) the kind of edge it is.  If we don't find
   such an edge, we return (in EDGE) the next (in a clockwise direction)
   edge on the current pixel.
   
   This implies that we go around an `outside' outline clockwise,
   and an `inside' outline counterclockwise.  */

static void
next_outline_edge (bitmap_type character, edge_type *edge,
		   unsigned *row, unsigned *col)
{
  unsigned original_row = *row;
  unsigned original_col = *col;
  
  switch (*edge)
    {
    case left:
      if (*row != 0)
	{
	  if (*col != 0)
	    { /* Try the pixel to the northwest.  */
	      if (is_outline_edge (bottom, character, *row - 1, *col - 1))
		{
		  (*row)--;
		  (*col)--;
		  *edge = bottom;

		  break;
		}
	    }
	  /* Try the pixel to the north.  */
	  if (is_outline_edge (left, character, *row - 1, *col))
            (*row)--;
	}
      break;

    case top:
      if (*col != BITMAP_WIDTH (character) - 1)
	{
	  if (*row != 0)
	    { /* Try the pixel to the northeast.  */
	      if (is_outline_edge (left, character, *row - 1, *col + 1))
		{
		  (*row)--;
		  (*col)++;
		  *edge = left;
		  break;
		}
	    }
	  /* Try the pixel to the east.  */
	  if (is_outline_edge (top, character, *row, *col + 1))
            (*col)++;
	}
      break;

    case right:
      if (*row != BITMAP_HEIGHT (character) - 1)
	{
	  if (*col != BITMAP_WIDTH (character) - 1)
	    { /* Try the pixel to the southeast.  */
	      if (is_outline_edge (top, character, *row + 1, *col + 1))
		{
		  (*row)++;
		  (*col)++;
		  *edge = top;
		  break;
		}
	    }
	  /* Try the pixel to the south.  */
	  if (is_outline_edge (right, character, *row + 1, *col))
            (*row)++;
	}
      break;

    case bottom:
      if (*col != 0)
	{
	  if (*row != BITMAP_HEIGHT (character) - 1)
	    { /* Try the pixel to the southwest.  */
	      if (is_outline_edge (right, character, *row + 1, *col - 1))
		{
		  (*row)++;
		  (*col)--;
		  *edge = right;
		  break;
		}
	    }
	  /* Try the pixel to the west.  */
	  if (is_outline_edge (bottom, character, *row, *col - 1))
            (*col)--;
        }
      break;

    case no_edge:
    default:
      FATAL1 ("next_outline_edge: Bad edge value (%d)", *edge);

    }

  /* If didn't find an adjacent edge on another pixel, find next edge
     on the current pixel.  */
  if (*row == original_row && *col == original_col)
    *edge = next_edge (*edge);
}


/* We return the next edge on the pixel at position ROW and COL which is
   an unmarked outline edge.  By ``next'' we mean either the one sent in
   in EDGE, if it qualifies, or the next one we find while checking the
   edges in in a clockwise direction.  EDGE must not be no_edge.  */

static edge_type
next_unmarked_outline_edge (unsigned row, unsigned col,
	 edge_type starting_edge, bitmap_type character, bitmap_type marked)
{
  edge_type edge = starting_edge;

  assert (edge != no_edge);

  while (is_marked_edge (edge, row, col, marked)
	 || !is_outline_edge (edge, character, row, col))
    {
      edge = next_edge (edge);
      if (edge == starting_edge)
        return no_edge;
    }

  return edge;
}


/* We check to see if the edge EDGE of the pixel at position ROW and COL
   is an outline edge; i.e., that it is a black pixel which shares that
   edge with a white pixel.  The position ROW and COL should be inside
   the bitmap CHARACTER.  */

static boolean
is_outline_edge (edge_type edge, bitmap_type character,
		 unsigned row, unsigned col)
{
  assert (BITMAP_VALID_PIXEL (character, row, col));

  /* If this pixel isn't black, it's not part of the outline.  */
  if (BITMAP_PIXEL (character, row, col) == WHITE)
    return false;

  switch (edge)
    {
    case left:
      return col == 0 || BITMAP_PIXEL (character, row, col - 1) == WHITE;

    case top:
      return row == 0 || BITMAP_PIXEL (character, row - 1, col) == WHITE;

    case right:
      return col == BITMAP_WIDTH (character) - 1
	|| BITMAP_PIXEL (character, row, col + 1) == WHITE;

    case bottom:
      return row == BITMAP_HEIGHT (character) - 1
	|| BITMAP_PIXEL (character, row + 1, col) == WHITE;

    case no_edge:
    default:
      FATAL1 ("is_outline_edge: Bad edge value(%d)", edge);
    }
  
  return false;
}


/* If EDGE is not already marked, we mark it; otherwise, it's a fatal error.
   The position ROW and COL should be inside the bitmap MARKED.  EDGE can
   be `no_edge'; we just return false.  */

static void
mark_edge (edge_type edge, unsigned row, unsigned col, bitmap_type *marked)
{
  if (is_marked_edge (edge, row, col, *marked))
    FATAL3 ("mark_edge: Already marked %s edge at row %u, col %u",
            edge_name (edge), row, col);

  if (edge != no_edge)
    BITMAP_PIXEL (*marked, row, col) |= 1 << edge;
}


static boolean
is_marked_edge (edge_type edge, unsigned row, unsigned col, bitmap_type marked)
{
  return
    edge == no_edge ? false : BITMAP_PIXEL (marked, row, col) & (1 << edge);
}


/* We return the edge which is adjacent to EDGE in a counterclockwise
   position.  This depends on the numeric values of the enumeration
   constants.  */

static edge_type
next_edge (edge_type edge)
{
  return edge == no_edge ? edge : (edge + 1) % NUM_EDGES;
}


static string
edge_name (edge_type edge)
{
  static string edge_names[] = { "left", "top", "right", "bottom", "no_edge" };

  return edge_names[edge];
}



/* Make all bounding boxes be OFFSET to the right.  */

void
offset_bounding_box_list (bounding_box_list_type *bb_list, int offset)
{
  unsigned this_bb;
  
  for (this_bb = 0; this_bb < BB_LIST_LENGTH (*bb_list); this_bb++)
    {
      bounding_box_type *bb = &BB_LIST_ELT (*bb_list, this_bb);
      MIN_COL (*bb) += offset;
      MAX_COL (*bb) += offset;
    }
}


/* Append a bounding box to the list.  This is called when we have
   completed an entire pixel outline.  The numbers in BB as it comes in
   refer to pixels; we have to change the maximum row so that it refers
   to the edge beyond the last pixel.  */

static void
append_bb (bounding_box_list_type *bb_list, bounding_box_type bb)
{
  BB_LIST_LENGTH (*bb_list)++;
  BB_LIST_DATA (*bb_list)
    = xrealloc (BB_LIST_DATA (*bb_list),
                 BB_LIST_LENGTH (*bb_list) * sizeof (bounding_box_type));
  MAX_COL (bb)++;
  BB_LIST_ELT (*bb_list, BB_LIST_LENGTH (*bb_list) - 1) = bb;
}


/* This routine returns an initialized list.  */

bounding_box_list_type
init_bounding_box_list ()
{
  bounding_box_list_type bb_list;
  
  BB_LIST_LENGTH (bb_list) = 0;
  BB_LIST_DATA (bb_list) = NULL;
  
  return bb_list;
}


/* Put the elements in the list B2 onto the end of B1.  B2 is not
   changed.  */

void
append_bounding_box_list (bounding_box_list_type *b1,
                          bounding_box_list_type b2)
{
  unsigned new_length;
  unsigned this_bb;

  assert (b1 != NULL);
  
  if (BB_LIST_LENGTH (b2) == 0)
    return;
    
  new_length = BB_LIST_LENGTH (*b1) + BB_LIST_LENGTH (b2);
  BB_LIST_DATA (*b1)
    = xrealloc (BB_LIST_DATA (*b1), new_length * sizeof (bounding_box_type));
  
  for (this_bb = 0; this_bb < BB_LIST_LENGTH (b2); this_bb++)
    BB_LIST_ELT (*b1, BB_LIST_LENGTH (*b1)++) = BB_LIST_ELT (b2, this_bb);
}


/* Here is a routine that frees the (relatively small) amount of memory
   in a bounding box list.  */

void
free_bounding_box_list (bounding_box_list_type *bb_list)
{
  if (BB_LIST_DATA (*bb_list) != NULL)
    safe_free ((address *) &BB_LIST_DATA (*bb_list));
}
