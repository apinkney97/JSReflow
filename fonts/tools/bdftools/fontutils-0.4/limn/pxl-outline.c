/* pxl-outline.c: find the edges of the bitmap image; we call each such
   edge an ``outline''; each outline is made up of one or more pixels;
   and each pixel participates via one or more edges.

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
#include "font.h"
#include "logreport.h"

#include "pxl-outline.h"


/* We consider each pixel to consist of four edges, and we travel along
   edges, instead of through pixel centers.  This is necessary for those
   unfortunate times when a single pixel is on both an inside and an
   outside outline.
   
   The numbers chosen here are not arbitrary; the code that figures out
   which edge to move to depends on particular values.  See the
   `TRY_PIXEL' macro.  To emphasize this, I've written in the numbers we
   need for each edge value.  */ 

typedef enum
{
  top = 1, left = 2, bottom = 3, right = 0, no_edge = 4
} edge_type;
static string edge_name[] = { "top", "left", "bottom", "right", "no_edge" };
#define NUM_EDGES no_edge

/* Likewise, we can move in any of eight directions as we are traversing
   the outline.  These numbers are not arbitrary, either.  */

typedef enum
{
  north = 0, northwest = 1, west = 2, southwest = 3, south = 4,
  southeast = 5, east = 6, northeast = 7
} direction_type;


/* This choice is also not arbitrary: starting at the top edge makes the
   code find outside outlines before inside ones, which is certainly
   what we want.  */
#define START_EDGE  top


/* Subroutine of find_outline_pixels.  */
static pixel_outline_type find_one_outline (char_info_type, edge_type,
			  		    unsigned row, unsigned col,
                                            bitmap_type *marked);
/* Subroutine of find_one_outline.  */
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


/* List routines.  */
static void append_pixel_outline (pixel_outline_list_type *, 
                                  pixel_outline_type);

static pixel_outline_type new_pixel_outline ();
static void append_outline_pixel (pixel_outline_type *, coordinate_type);
static void append_coordinate (pixel_outline_type *, int x, int y, edge_type);



/* A character is made up of a list of one or more outlines.  Here, we
   go through a character's bitmap top to bottom, left to right, looking
   for the next pixel with an unmarked edge also on the character's outline.
   Each one of these we find is the starting place for one outline.  We
   find these outlines and put them in a list to return.  */

pixel_outline_list_type
find_outline_pixels (char_info_type c)
{
  pixel_outline_list_type outline_list;
  bitmap_type bitmap = CHAR_BITMAP (c);
  bitmap_type marked = new_bitmap (BITMAP_DIMENSIONS (bitmap));
  unsigned row, col;

  O_LIST_LENGTH (outline_list) = 0;
  outline_list.data = NULL;

  for (row = 0; row < BITMAP_HEIGHT (bitmap); row++)
    for (col = 0; col < BITMAP_WIDTH (bitmap); col++)
      {
	edge_type edge;
        
        if (BITMAP_PIXEL (bitmap, row, col) == WHITE)
          continue;
        
        edge = next_unmarked_outline_edge (row, col, START_EDGE, bitmap,
                                           marked);

	if (edge != no_edge)
	  {
            pixel_outline_type outline;
            boolean clockwise = edge == bottom;
            
            LOG2 ("#%u: (%sclockwise)", O_LIST_LENGTH (outline_list),
                                        clockwise ? "" : "counter"); 

            outline = find_one_outline (c, edge, row, col, &marked);
            
            /* Outside outlines will start at a top edge, and move
               counterclockwise, and inside outlines will start at a
               bottom edge, and move clockwise.  This happens because of
               the order in which we look at the edges.  */
            O_CLOCKWISE (outline) = clockwise;
	    append_pixel_outline (&outline_list, outline);

            REPORT (".");
            LOG1 (" [%u].\n", O_LENGTH (outline));
	  }
      }

  free_bitmap (&marked);
  
  return outline_list;
}


/* Here we find one of a character's outlines.  We're passed the
   position (ORIGINAL_ROW and ORIGINAL_COL) of a starting pixel and one
   of its unmarked edges, STARTING_EDGE.  We traverse the adjacent edges
   of the outline pixels, appending to the coordinate list.  We keep
   track of the marked edges in MARKED, so it should be initialized to
   zeros when we first get it.  */

static pixel_outline_type
find_one_outline (char_info_type c, edge_type original_edge,
		  unsigned original_row, unsigned original_col,
		  bitmap_type *marked)
{
  pixel_outline_type outline = new_pixel_outline ();
  bitmap_type bitmap = CHAR_BITMAP (c);
  unsigned row = original_row, col = original_col;
  edge_type edge = original_edge;

  do
    {
      /* Put this edge on to the output list, changing to Cartesian, and
         taking account of the side bearings.  */
      append_coordinate (&outline, col + CHAR_MIN_COL (c), 
                         CHAR_MAX_ROW (c) - row, edge);

      mark_edge (edge, row, col, marked);
      next_outline_edge (bitmap, &edge, &row, &col);
    }
  while (row != original_row || col != original_col || edge != original_edge);

  return outline;
}



/* These macros are used (directly or indirectly) by the
   `next_outline_edge' routine.  */ 

/* Given the direction DIR of the pixel to test, decide which edge on
   that pixel we are supposed to test.  Because we've chosen the mapping
   from directions to numbers carefully, we don't have to do much.  */

#define FIND_TEST_EDGE(dir) ((dir) / 2)


/* Find how to move in direction DIR on the axis AXIS (either `ROW' or
  `COL').   We are in the ``display'' coordinate system, with y
  increasing downward and x increasing to the right.  Therefore, we are
  implementing the following table:
  
  direction  row delta  col delta
    north       -1          0  
    south	+1	    0
    east	 0	   +1
    west	 0	   +1
    
  with the other four directions (e.g., northwest) being the sum of
  their components (e.g., north + west).
  
  The first macro, `COMPUTE_DELTA', handles splitting up the latter
  cases, all of which have been assigned odd numbers.  */
  
#define COMPUTE_DELTA(axis, dir)					\
  ((dir) % 2 != 0							\
    ? COMPUTE_##axis##_DELTA ((dir) - 1)				\
      + COMPUTE_##axis##_DELTA (((dir) + 1) % 8)			\
    : COMPUTE_##axis##_DELTA (dir)					\
  )

/* Now it is trivial to implement the four cardinal directions.  */
#define COMPUTE_ROW_DELTA(dir)						\
  ((dir) == north ? -1 : (dir) == south ? +1 : 0)

#define COMPUTE_COL_DELTA(dir)						\
  ((dir) == west ? -1 : (dir) == east ? +1 : 0)


/* See if the appropriate edge on the pixel from (row,col) in direction
   DIR is on the outline.  If so, update `row', `col', and `edge', and
   break.  We also use the variable `character' as the bitmap in which
   to look.  */

#define TRY_PIXEL(dir)							\
  {									\
    int delta_r = COMPUTE_DELTA (ROW, dir);				\
    int delta_c = COMPUTE_DELTA (COL, dir);				\
    int test_row = *row + delta_r;					\
    int test_col = *col + delta_c;					\
    edge_type test_edge = FIND_TEST_EDGE (dir);				\
									\
    if (BITMAP_VALID_PIXEL (character, test_row, test_col)		\
        && is_outline_edge (test_edge, character, test_row, test_col))	\
      {									\
        *row = test_row;						\
        *col = test_col;						\
        *edge = test_edge;						\
        break;								\
      }									\
  }


/* Finally, we are ready to implement the routine that finds the next
   edge on the outline.  We look first for an adjacent edge that is not
   on the current pixel.  We want to go around outside outlines
   counterclockwise, and inside outlines clockwise (because that is how
   both Metafont and Adobe Type 1 format want their curves to be drawn).
   The very first outline (an outside one) on each character will start
   on a top edge; so, if we're at a top edge, we want to go only to the
   left (on the pixel to the west) or down (on the same pixel), to begin
   with.  Then, when we're on a left edge, we want to go to the top edge
   (on the southwest pixel) or to the left edge (on the south pixel).
   
   All well and good. But if you draw a rasterized circle (or whatever),
   eventually we have to come back around to the beginning; at that
   point, we'll be on a top edge, and we'll have to go to the right edge
   on the northwest pixel.  Draw pictures.
   
   The upshot is, if we find an edge on another pixel, we return (in ROW
   and COL) the position of the new pixel, and (in EDGE) the kind of
   edge it is.  If we don't find such an edge, we return (in EDGE) the
   next (in a counterclockwise direction) edge on the current pixel.  */

static void
next_outline_edge (bitmap_type character, edge_type *edge,
		   unsigned *row, unsigned *col)
{
  unsigned original_row = *row;
  unsigned original_col = *col;
  
  switch (*edge)
    {
    case right:
      TRY_PIXEL (north);
      TRY_PIXEL (northeast);
      break;

    case top:
      TRY_PIXEL (west);
      TRY_PIXEL (northwest);
      break;

    case left:
      TRY_PIXEL (south);
      TRY_PIXEL (southwest);
      break;

    case bottom:
      TRY_PIXEL (east);
      TRY_PIXEL (southeast);
      break;

    default:
      FATAL1 ("next_outline_edge: Bad edge value (%d)", *edge);

    }

  /* If we didn't find an adjacent edge on another pixel, return the
     next edge on the current pixel.  */
  if (*row == original_row && *col == original_col)
    *edge = next_edge (*edge);
}


/* We return the next edge on the pixel at position ROW and COL which is
   an unmarked outline edge.  By ``next'' we mean either the one sent in
   in EDGE, if it qualifies, or the next one returned by `next_edge'.  */

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
  
  return 0; /* NOTREACHED */
}


/* If EDGE is not already marked, we mark it; otherwise, it's a fatal error.
   The position ROW and COL should be inside the bitmap MARKED.  EDGE can
   be `no_edge'; we just return false.  */

static void
mark_edge (edge_type edge, unsigned row, unsigned col, bitmap_type *marked)
{
  if (is_marked_edge (edge, row, col, *marked))
    FATAL3 ("mark_edge: Already marked %s edge at row %u, col %u",
            edge_name[edge], row, col);

  if (edge != no_edge)
    BITMAP_PIXEL (*marked, row, col) |= 1 << edge;
}


static boolean
is_marked_edge (edge_type edge, unsigned row, unsigned col, bitmap_type marked)
{
  return
    edge == no_edge ? false : BITMAP_PIXEL (marked, row, col) & (1 << edge);
}


/* Return the edge which is counterclockwise-adjacent to EDGE.  This
   code makes use of the numeric values of the enumeration constants;
   sorry about that.  */

static edge_type
next_edge (edge_type edge)
{
  return edge == no_edge ? edge : (edge + 1) % NUM_EDGES;
}



/* Append an outline to an outline list.  This is called when we have
   completed an entire pixel outline.  */

static void
append_pixel_outline (pixel_outline_list_type *outline_list,
		      pixel_outline_type outline)
{
  O_LIST_LENGTH (*outline_list)++;
  XRETALLOC (outline_list->data, outline_list->length, pixel_outline_type);
  O_LIST_OUTLINE (*outline_list, O_LIST_LENGTH (*outline_list) - 1) = outline;
}


/* Here is a routine that frees a list of such lists.  */

void
free_pixel_outline_list (pixel_outline_list_type *outline_list)
{
  unsigned this_outline;

  for (this_outline = 0; this_outline < outline_list->length; this_outline++)
    {
      pixel_outline_type o = outline_list->data[this_outline];
      safe_free ((address *) &(o.data));
    }

  if (outline_list->data != NULL)
    safe_free ((address *) &(outline_list->data));
}



/* Return an empty list of pixels.  */
   

pixel_outline_type
new_pixel_outline ()
{
  pixel_outline_type pixel_outline;

  O_LENGTH (pixel_outline) = 0;
  pixel_outline.data = NULL;

  return pixel_outline;
}


/* Add the coordinate C to the pixel list O.  */

static void
append_outline_pixel (pixel_outline_type *o, coordinate_type c)
{
  O_LENGTH (*o)++;
  XRETALLOC (o->data, O_LENGTH (*o), coordinate_type);
  O_COORDINATE (*o, O_LENGTH (*o) - 1) = c;
}


/* We are given an (X,Y) in Cartesian coordinates, and the edge of the pixel
   we're on.  We append a corner of that pixel as our coordinate.
   If we're on a top edge, we use the upper-left hand corner, and so
   on, counterclockwise.  Perhaps we should be doing counterclockwise
   only on outside outlines?  */

void
append_coordinate (pixel_outline_type *o, int x, int y, edge_type edge)
{
  coordinate_type c = { x, y};

  switch (edge)
    {
    case top:
      c.y++;
      break;
    
    case right:
      c.x++;
      c.y++;
      break;
    
    case bottom:
      c.x++;
      break;
    
    case left:
      break;
    
    default:
      FATAL1 ("append_coordinate: Bad edge (%d)", edge);
    }

  LOG2 (" (%d,%d)", c.x, c.y);
  append_outline_pixel (o, c);
}
