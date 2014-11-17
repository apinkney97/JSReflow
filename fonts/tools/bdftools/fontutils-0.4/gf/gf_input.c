/* gf_input.c: read objects from one GF file.

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

#include "file-input.h"
#include "gf.h"
#include "scaled-num.h"

#include "gf_opcodes.h"


/* If true, print out what we read.  */
static boolean tracing_gf_input = false;


/* These identify the file we're reading, for our own use.  */
static string gf_input_filename;
static FILE *gf_input_file;


/* Low-level input.  These macros call the corresponding routines in
   kbase, using the static variables for the input file and filename.  */

#define GF_FTELL() xftell (gf_input_file, gf_input_filename)
#define GF_FSEEK(offset, from_where) \
  xfseek (gf_input_file, offset, from_where, gf_input_filename)
#define GF_GET_BYTE() get_byte (gf_input_file, gf_input_filename)
#define GF_GET_PREVIOUS() get_previous_byte (gf_input_file, gf_input_filename)
#define GF_GET_TWO() get_two (gf_input_file, gf_input_filename)
#define GF_GET_FOUR() get_four (gf_input_file, gf_input_filename)
#define GF_GET_SIGNED_FOUR() \
  get_signed_four (gf_input_file, gf_input_filename)
#define GF_GET_PREVIOUS_FOUR() \
  get_previous_four (gf_input_file, gf_input_filename)
#define GF_MATCH_BYTE(expected) \
  match_byte (expected, gf_input_file, gf_input_filename)

static void get_specials (byte_count_type special_ptr);
static string gf_get_string (unsigned string_length);
static four_bytes gf_get_three (void);


/* We have occasion to refer to the postamble when reading the characters,
   so we keep a copy for ourselves.  */
static gf_postamble_type *private_postamble;

/* This reads into `private_postamble'.  */
static void get_postamble (void);

static void get_locators (void);


/* Pointers to the characters we have read.  */
static gf_char_type *char_list[MAX_CHARCODE + 1];


/* Subroutines for reading a character.  */
static void get_full_boc (gf_char_type *);
static void get_abbreviated_boc (gf_char_type *);
static void get_character_bitmap (gf_char_type *);
static void get_postamble_info (gf_char_type *);
static void deblank (gf_char_type *);

/* Subroutines for reading the bitmap.  */
static void get_paint (coordinate_type *, boolean *, one_byte, gf_char_type *);
static void get_skip (coordinate_type *, boolean *, one_byte, gf_char_type *);
static void get_new_row
  (coordinate_type *, boolean *, one_byte, gf_char_type *);

/* For handling raw characters.  */
static void append_byte (raw_char_type *, one_byte);
static void append_n_bytes (raw_char_type *, four_bytes, one_byte *);
static raw_char_type new_raw_gf_char (gf_char_locator_type);



/* Start reading FILENAME.  Return false if it can't be opened.  */

boolean
gf_open_input_file (string filename)
{
  if (gf_input_file != NULL)
    FATAL2 ("gf_open_input_file: Attempt to open `%s', but `%s' is
already open", filename, gf_input_filename);

  gf_input_filename = filename;
  gf_input_file = fopen (filename, "r");

  return gf_input_file != NULL;
}


void
gf_close_input_file ()
{
  unsigned code;
  
  assert (gf_input_file != NULL);

  xfclose (gf_input_file, gf_input_filename);

  gf_input_filename = NULL;
  gf_input_file = NULL;
  private_postamble = NULL;
  
  for (code = 0; code <= MAX_CHARCODE; code++)
    char_list[code] = NULL;
}



/* The only interesting thing in the preamble, either to the user or to
   us, is the comment.

   It is wrong to save and restore the position in the gf file, since
   this routine is primarily useful if the user intends to use
   `gf_get_next_char', i.e., read the characters in the order they
   appear.  In that case, the user has to read the preamble first.  */

string
gf_get_preamble ()
{
  one_byte comment_length;
  string answer;

  assert (gf_input_file != NULL);

  GF_MATCH_BYTE (PRE);
  GF_MATCH_BYTE (GF_ID);

  comment_length = GF_GET_BYTE ();
  answer = gf_get_string (comment_length);

  return answer;
}



/* Reading the postamble.  This is the user-level call.  We make no
   assumptions about the current position of the file pointer.  */

gf_postamble_type
gf_get_postamble ()
{
  one_byte b;
  byte_count_type old_pos, post_ptr;
  unsigned this_char;

  assert (gf_input_file != NULL);

  if (private_postamble != NULL)   /* Previously read it?  */
    return *private_postamble;

  old_pos = GF_FTELL ();
  GF_FSEEK (0, SEEK_END);

  do
    b = GF_GET_PREVIOUS ();
  while (b == GF_SIGNATURE);

  if (b != GF_ID)
    FATAL2 ("Expected GF id (%u), found %u", GF_ID, b);

  post_ptr = GF_GET_PREVIOUS_FOUR ();

  private_postamble = xmalloc (sizeof (gf_postamble_type));

  for (this_char = 0; this_char <= MAX_CHARCODE; this_char++)
    {
      GF_CHAR_LOC (*private_postamble, this_char).charcode
        = this_char;
      GF_CHAR_LOC (*private_postamble, this_char).char_pointer
        = NULL_BYTE_PTR;
    }

  GF_FSEEK (post_ptr, SEEK_SET);
  get_postamble ();		/* Fills in `private_postamble'.  */

  GF_FSEEK (old_pos, SEEK_SET);
  return *private_postamble;
}


/* Assume we are positioned at the beginning of the postamble.  Fill in
   `private_postamble' with what we find.  */

static void
get_postamble ()
{
  GF_MATCH_BYTE (POST);

  (void) GF_GET_FOUR ();		/* Ignore the special pointer.  */

  GF_DESIGN_SIZE (*private_postamble) = GF_GET_FOUR ();
  GF_CHECKSUM (*private_postamble) = GF_GET_FOUR ();

  /* The resolution values are stored in the file as pixels per point,
     scaled by 2^16.  */
  GF_H_RESOLUTION (*private_postamble) = GF_GET_FOUR ();
  GF_V_RESOLUTION (*private_postamble) = GF_GET_FOUR ();
  
  MIN_COL (GF_FONT_BB (*private_postamble)) = GF_GET_SIGNED_FOUR ();
  MAX_COL (GF_FONT_BB (*private_postamble)) = GF_GET_SIGNED_FOUR ();
  MIN_ROW (GF_FONT_BB (*private_postamble)) = GF_GET_SIGNED_FOUR ();
  MAX_ROW (GF_FONT_BB (*private_postamble)) = GF_GET_SIGNED_FOUR ();

  get_locators ();
}


/* We do not know in advance how many character locators exist, but we do
   place a maximum on it (contrary to what the GF format definition
   says), namely, MAX_CHARCODE.  */

static void
get_locators ()
{
  do
    {
      one_byte code;
      one_byte b = GF_GET_BYTE ();

      if (b == POST_POST)
	break;

      code = GF_GET_BYTE ();

      if (b == CHAR_LOC)
	{
	  GF_CHAR_LOC (*private_postamble, code).h_escapement
	    = scaled_to_real (GF_GET_FOUR ()) + .5;
	  (void) GF_GET_FOUR ();  /* Ignore vertical escapement.  */
	}
      else if (b == CHAR_LOC0)
        GF_CHAR_LOC (*private_postamble, code).h_escapement = GF_GET_BYTE ();
      else
        FATAL1 ("Expected a char_loc command, not %u", b);

      GF_CHAR_LOC (*private_postamble, code).tfm_width = GF_GET_FOUR ();
      GF_CHAR_LOC (*private_postamble, code).char_pointer = GF_GET_FOUR ();
    }
  while (true);
}



/* Unlike `gf_get_next_char' (which comes next), this routine will get any
   character by its code.  It is meant to be called by the user.  We
   save all the characters we read in the static `char_list', to avoid
   going out to the file each time.  */

gf_char_type *
gf_get_char (one_byte charcode)
{
  gf_char_locator_type char_locator;
  byte_count_type char_pointer;

  if (char_list[charcode] != NULL)
    return char_list[charcode];

  (void) gf_get_postamble ();
  assert (private_postamble != NULL);

  char_locator = GF_CHAR_LOC (*private_postamble, charcode);
  char_pointer = char_locator.char_pointer;

  if (char_pointer != NULL_BYTE_PTR)
    {
      boolean found;
      byte_count_type old_pos = GF_FTELL ();
      GF_FSEEK (char_pointer, SEEK_SET);

      char_list[charcode] = xmalloc (sizeof (gf_char_type));
      *(char_list[charcode]) = gf_get_next_char (&found);
      assert (found);

      GF_FSEEK (old_pos, SEEK_SET);
    }

  return char_list[charcode];
}


/* This reads the character starting from the current position (but some
   specials might come first).  */

gf_char_type
gf_get_next_char (boolean *found)
{
  one_byte c;
  gf_char_type gf_char;

  get_specials (GF_FTELL ());

  c = GF_GET_BYTE ();

  if (c == BOC)
    get_full_boc (&gf_char);

  else if (c == BOC1)
    get_abbreviated_boc (&gf_char);

  else if (c == POST)
    {
      *found = false;
      return gf_char;		/* We are returning garbage, but that's OK.  */
    }

  else
    FATAL1 ("Expected a character (or the postamble), found %u", c);

  get_character_bitmap (&gf_char);
  get_postamble_info (&gf_char);

  deblank (&gf_char);

  *found = true;
  return gf_char;
}


/* If the back pointer actually points somewhere, this character is a
   ``residue'', and the font is probably too big.  */

static void
get_full_boc (gf_char_type *gf_char)
{
  signed_4_bytes back_pointer, charcode;

  charcode = GF_GET_SIGNED_FOUR ();
  if (charcode < 0 || charcode > MAX_CHARCODE)
    /* Someone is trying to use a font with character codes that are
       out of our range.  */
    FATAL1 ("Character code %d out of range 0..255 in GF file (sorry)",
            charcode);

  GF_CHARCODE (*gf_char) = charcode;
  
  back_pointer = GF_GET_SIGNED_FOUR ();
  if (back_pointer != -1)
    WARNING2 ("Character %u has a non-null back pointer (to %d)", 
              GF_CHARCODE (*gf_char), back_pointer);

  GF_CHAR_MIN_COL (*gf_char) = GF_GET_SIGNED_FOUR ();
  GF_CHAR_MAX_COL (*gf_char) = GF_GET_SIGNED_FOUR ();
  GF_CHAR_MIN_ROW (*gf_char) = GF_GET_SIGNED_FOUR ();
  GF_CHAR_MAX_ROW (*gf_char) = GF_GET_SIGNED_FOUR ();
}


static void
get_abbreviated_boc (gf_char_type *gf_char)
{
  one_byte col_delta, row_delta;

  GF_CHARCODE (*gf_char) = GF_GET_BYTE ();

  col_delta = GF_GET_BYTE ();
  GF_CHAR_MAX_COL (*gf_char) = GF_GET_BYTE ();
  GF_CHAR_MIN_COL (*gf_char) = GF_CHAR_MAX_COL (*gf_char) - col_delta;

  row_delta = GF_GET_BYTE ();
  GF_CHAR_MAX_ROW (*gf_char) = GF_GET_BYTE ();
  GF_CHAR_MIN_ROW (*gf_char) = GF_CHAR_MAX_ROW (*gf_char) - row_delta;
}


/* We also want to return the horizontal escapement (i.e., set width)
   and the TFM width as part of the character, although this information is
   not part of the boc..eoc, but rather the postamble.  */

static void
get_postamble_info (gf_char_type *gf_char)
{
  gf_char_locator_type char_locator;

  (void) gf_get_postamble ();	/* Use `private_postamble'.  */
  assert (private_postamble != NULL);

  char_locator = GF_CHAR_LOC (*private_postamble, GF_CHARCODE (*gf_char));

  GF_H_ESCAPEMENT (*gf_char) = char_locator.h_escapement;
  GF_TFM_WIDTH (*gf_char) = char_locator.tfm_width;
}


/* The GF format does not guarantee that the bounding box is the
   smallest possible, i.e., that the character bitmap does not have
   blank rows or columns at an edge.  We want to remove such blanks.  */

static void
deblank (gf_char_type *gf_char)
{
  boolean all_white;
  bitmap_type gf_bitmap = GF_BITMAP (*gf_char);
  unsigned width = BITMAP_WIDTH (gf_bitmap),
           height = BITMAP_HEIGHT (gf_bitmap);
  unsigned white_on_left = 0, white_on_right = 0,
           white_on_top = 0, white_on_bottom = 0;
  int this_col, this_row; /* int in case GF_CHAR is zero pixels wide.  */

  /* Let's start with blank columns at the left-hand side.  */
  all_white = true;
  for (this_col = 0; this_col < width && all_white; this_col++)
    {
      for (this_row = 0; this_row < height && all_white; this_row++)
	{
	  if (BITMAP_PIXEL (gf_bitmap, this_row, this_col) != 0)
            all_white = false;
	}
      if (all_white)
        white_on_left++;
    }

  /* Now let's check the right-hand side.  */
  all_white = true;
  for (this_col = width - 1; this_col >= 0 && all_white; this_col--)
    {
      for (this_row = 0; this_row < height && all_white; this_row++)
	{
	  if (BITMAP_PIXEL (gf_bitmap, this_row, this_col) != WHITE)
            all_white = false;
	}
      if (all_white)
        white_on_right++;
    }

  /* Check for all-white rows on top now.  */
  all_white = true;
  for (this_row = 0; this_row < height && all_white; this_row++)
    {
      for (this_col = 0; this_col < width && all_white; this_col++)
        {
          if (BITMAP_PIXEL (gf_bitmap, this_row, this_col) != WHITE)
            all_white = false;
        }
      if (all_white)
        white_on_top++;
    }

  /* And, last, for all-white rows on the bottom.  */
  all_white = true;
  for (this_row = height - 1; this_row >= 0 && all_white; this_row--)
    {
      for (this_col = 0; this_col < width && all_white; this_col++)
        {
          if (BITMAP_PIXEL (gf_bitmap, this_row, this_col) != WHITE)
            all_white = false;
        }
      if (all_white)
        white_on_bottom++;
    }

  /* If we have to remove columns at either the left or the right, we
     have to reallocate the memory, since much code depends on the fact
     that the bitmap is in contiguous memory.  If we have to remove
     rows, we don't necessarily have to reallocate the memory, but we
     might as well, to save space.  */
  if (white_on_left > 0 || white_on_right > 0
      || white_on_top > 0 || white_on_bottom > 0)
    {
      bitmap_type condensed;
      
      if (white_on_left + white_on_right > width)
        { /* The character was entirely blank.  */
          BITMAP_WIDTH (condensed) = BITMAP_HEIGHT (condensed) = 0;
          GF_CHAR_MIN_COL (*gf_char) = GF_CHAR_MAX_COL (*gf_char)
            = GF_CHAR_MIN_ROW (*gf_char) = GF_CHAR_MAX_ROW (*gf_char) = 0;
          BITMAP_BITS (condensed) = NULL;
        }
      else
        {
          dimensions_type d;
          DIMENSIONS_HEIGHT (d) = height - white_on_top - white_on_bottom;
          DIMENSIONS_WIDTH (d) = width - white_on_left - white_on_right;
          condensed = new_bitmap (d);

          for (this_row = 0; this_row < BITMAP_HEIGHT (condensed); this_row++)
            for (this_col = 0; this_col < BITMAP_WIDTH (condensed); this_col++)
              {
	        BITMAP_PIXEL (condensed, this_row, this_col)
                  = BITMAP_PIXEL (gf_bitmap, this_row + white_on_top,
                                             this_col + white_on_left);
	      }
          GF_CHAR_MIN_ROW (*gf_char) += white_on_bottom;
          GF_CHAR_MAX_ROW (*gf_char) -= white_on_top;
          GF_CHAR_MIN_COL (*gf_char) += white_on_left;
          GF_CHAR_MAX_COL (*gf_char) -= white_on_right;
	}

      free_bitmap (&GF_BITMAP (*gf_char));
      GF_BITMAP (*gf_char) = condensed;
    }
}


/* The ``bitmap'' is a sequence of commands that describe it in terms of
   run-length encoding.

   GF's row and column numbers are the lower left corner of a pixel. 
   GF (0,0) is the Cartesian unit square: 0 <= x (col) <= 1,
   0 <= y (row) <= 1.  Yes, it's <=, not <.  What does this mean for
   the maxes and mins?  Let's take the height first:  if a character has
   min_row = 0 and max_row = 10, we start the ``current'' y at 10,
   (possibly) paint some pixels on that row, ..., and end up with it at
   zero, (possibly) painting some pixels on that row.  Thus, there are
   11 (10 - 0 + 1) rows in which we might paint pixels.  Now the width:
   if a character has min_row = 0 and max_row = 4, the current x starts
   at zero, we paint four pixels (let's say), and now the current x is
   four (the max possible), so we cannot paint any more.  Thus there are
   four (4 - 0) columns in which we might paint pixels. 
   
   Weird, huh?  */

static void
get_character_bitmap (gf_char_type *gf_char)
{
  /* We expect these to be >= 0, but if the GF file is improper, they
     might turn to be negative.  */
  int height, width;
  dimensions_type matrix_dimensions;
  coordinate_type current;	/* This will be the GF position.  */
  boolean painting_black = false;

  current.x = GF_CHAR_MIN_COL (*gf_char);
  current.y = GF_CHAR_MAX_ROW (*gf_char);

  width = GF_CHAR_MAX_COL (*gf_char) - GF_CHAR_MIN_COL (*gf_char);
  height = GF_CHAR_MAX_ROW (*gf_char) - GF_CHAR_MIN_ROW (*gf_char) + 1;

  /* If the character has zero or negative extent in either dimension,
     it's not going to have a bitmap.  (If this happens, the GF file is
     incorrect; but the discrepancy isn't serious, so we may as well not
     bomb out when it happens, especially since PKtoGF has a bug that
     produces such a bounding box when the character is all blank.)  */
  if (width <= 0 || height <= 0)
    {
      one_byte b;
      BITMAP_HEIGHT (GF_BITMAP (*gf_char))
        = BITMAP_WIDTH (GF_BITMAP (*gf_char))
        = 0;
      BITMAP_BITS (GF_BITMAP (*gf_char)) = NULL;
      
      /* The next non-NO_OP byte should be EOC.  */
      while ((b = GF_GET_BYTE ()) == NO_OP)
        /* do nothing */ ;
      if (b != EOC)
        FATAL1 ("Expected eoc (for a blank character), found %u", b);

      return;
    }
  
  DIMENSIONS_HEIGHT (matrix_dimensions) = height;
  DIMENSIONS_WIDTH (matrix_dimensions) = width;
  GF_BITMAP (*gf_char) = new_bitmap (matrix_dimensions);

  do
    {
      one_byte c = GF_GET_BYTE ();

      if (c == EOC)
	break;

      if (PAINT_0 <= c && c <= PAINT3)
        get_paint (&current, &painting_black, c, gf_char);

      else if (SKIP0 <= c && c <= SKIP3)
        get_skip (&current, &painting_black, c, gf_char);

      else if (NEW_ROW_0 <= c && c <= NEW_ROW_164)
        get_new_row (&current, &painting_black, c, gf_char);

      else if (c == NO_OP)
        /* do nothing */ ;

      else if ((XXX1 <= c && c <= XXX4) || c == YYY)
        get_specials (GF_FTELL ());

      else
        FATAL1 ("Expected paint or skip or new_row, found %u", c);
    }
  while (true);

  if (tracing_gf_input)
    puts ("");
}


/* The paint commands come in two varieties -- either with the length
   implicitly part of the command, or where it is specified as a separate
   parameter.  */

static void
get_paint (coordinate_type *current, boolean *painting_black,
           one_byte command, gf_char_type *gf_char)
{
  unsigned length;

  if (PAINT_0 <= command && command <= PAINT_63)
    {				/* Implicit paint.  */
      length = command - PAINT_0;
    }
  else
    {
      switch (command)
	{
	case PAINT1:
	  length = GF_GET_BYTE ();
	  break;

	case PAINT2:
	  length = GF_GET_TWO ();
	  break;

	case PAINT3:
	  length = gf_get_three ();
	  break;

	default:
	  FATAL1 ("Expected a paint command, found %u", command);
	}
    }

  /* We have to translate from Cartesian to C coordinates.  That means the
     x's are the same, but the y's are flipped.  */
  if (*painting_black)
    {
      unsigned matrix_y = GF_CHAR_MAX_ROW (*gf_char) - current->y;

      if (tracing_gf_input)
        printf ("%u", length);

      for (; length != 0; length--)
	{
	  unsigned matrix_x = current->x - GF_CHAR_MIN_COL (*gf_char);

	  BITMAP_PIXEL (GF_BITMAP (*gf_char), matrix_y, matrix_x) = 1;

	  current->x++;
	}
    }
  else
    {
      current->x += length;

      if (tracing_gf_input)
        printf ("(%u)", length);
    }
  *painting_black = !*painting_black;
}


/* Skip commands move down in the GF character, leaving blank rows.  */

static void
get_skip (coordinate_type * current, boolean * painting_black,
          one_byte command, gf_char_type *gf_char)
{
  unsigned rows_to_skip;

  switch (command)
    {
    case SKIP0:
      rows_to_skip = 0;
      break;

    case SKIP1:
      rows_to_skip = GF_GET_BYTE ();
      break;

    case SKIP2:
      rows_to_skip = GF_GET_TWO ();
      break;

    case SKIP3:
      rows_to_skip = gf_get_three ();
      break;

    default:
      FATAL1 ("Expected skip command, found %u", command);
    }

  current->y -= rows_to_skip + 1;
  current->x = GF_CHAR_MIN_COL (*gf_char);
  *painting_black = false;

  if (tracing_gf_input)
    printf ("\nskip %u (y now %d) ", rows_to_skip + 1, current->y);
}


/* `new_row' commands move to the next line down and then over.  */

static void
get_new_row (coordinate_type *current, boolean *painting_black,
	     one_byte command, gf_char_type *gf_char)
{
  current->y--;
  current->x = GF_CHAR_MIN_COL (*gf_char) + command - NEW_ROW_0;
  *painting_black = true;

  if (tracing_gf_input)
    printf ("\nnew_row %u (x,y now %d,%d) ", command - NEW_ROW_0,
            current->x, current->y);
}



/* Instead of building a bitmap, this routine just reads the bytes that
   define the character CHARCODE.  If the character doesn't exist, we
   return NULL.  */

/* Append the byte `b' to the buffer in `raw_char', followed by (if
   WITH_SIZE_P is true) SIZE, followed SIZE bytes of the input.  */
#define COMMON_APPEND(size, with_size_p)				\
  do									\
    {									\
      append_byte (raw_char, b);					\
      if (with_size_p) append_byte (raw_char, size);			\
      if (size > 0) append_n_bytes (raw_char, size, get_n_bytes (size,	\
                                    gf_input_file, gf_input_filename));	\
    }									\
  while (0)

#define APPEND_BYTES(n) COMMON_APPEND (n, false)
#define APPEND_VAR_BYTES(n) COMMON_APPEND (n, true)

raw_char_type *
gf_get_raw_char (one_byte charcode)
{
  one_byte b; /* Command byte we've just read.  */
  gf_char_locator_type char_locator;
  byte_count_type char_pointer;
  byte_count_type old_pos;
  raw_char_type *raw_char;

  (void) gf_get_postamble ();
  assert (private_postamble != NULL);
  
  char_locator = GF_CHAR_LOC (*private_postamble, charcode);
  char_pointer = char_locator.char_pointer;

  /* If the character doesn't exist in the font, we're done.  */
  if (char_pointer == NULL_BYTE_PTR)
    return NULL;
  
  /* Save where we are in the file; we'll restore it when we're done.  */
  old_pos = GF_FTELL ();
  GF_FSEEK (char_pointer, SEEK_SET);
  
  raw_char = xmalloc (sizeof (raw_char_type));
  *raw_char = new_raw_gf_char (char_locator);

  /* Unfortunately, the GF format doesn't tell us how many bytes long
     the character description is.  So, we must decode it enough to
     figure out how much to read.  */
  
  /* The first step is the boc: it might be either the one-byte or
     four-byte variant.  */
  b = GF_GET_BYTE ();
  if (b == BOC)
    /* The character code, the backpointer (which will probably be wrong
       in the new file, if it exists, but we don't handle non-null
       backpointers anyway), the bounding box.  */
    APPEND_BYTES (24);
  else /* b == BOC1 */
    APPEND_BYTES (5);
  
  /* Read the character definition; exit when we get to EOC.  */
  while ((b = GF_GET_BYTE ()) != EOC)
    {
      /* First test all the commands that have no parameters.  */
      if (PAINT_0 <= b && b <= PAINT_63
          || b == SKIP0
          || NEW_ROW_0 <= b && b <= NEW_ROW_164)
        APPEND_BYTES (0);
      
      /* Now the ones with a one-byte parameter.  */
      else if (b == PAINT1 || b == SKIP1)
        APPEND_BYTES (1);
      
      /* A two-byte parameter.  */
      else if (b == PAINT2 || b == SKIP2)
        APPEND_BYTES (2);
      
      /* Three-byte.  */
      else if (b == PAINT3 || b == SKIP3)
        APPEND_BYTES (3);
      
      /* Four.  */
      else if (b == YYY)
        APPEND_BYTES (4);
      
      /* Commands with a variable-size parameter.  */
      else if (b == XXX1)
        {
          one_byte size = GF_GET_BYTE ();
          APPEND_VAR_BYTES (size);
        }
      
      else if (b == XXX2)
        {
          two_bytes size = GF_GET_TWO ();
          APPEND_VAR_BYTES (size);
        }
      
      else if (b == XXX3)
        {
          four_bytes size = gf_get_three ();
          APPEND_VAR_BYTES (size);
        }
      
      else if (b == XXX4)
        {
          four_bytes size = GF_GET_FOUR ();
          APPEND_VAR_BYTES (size);
        }
  
      else if (b == NO_OP)
        /* do nothing */;
      
      else
        FATAL1 ("gf_get_raw_char: Bad command byte %u", b);
    }

  /* This appends the `eoc'---and now we're done.  */
  APPEND_BYTES (0);

  GF_FSEEK (old_pos, SEEK_SET);
  return raw_char;
}


/* This macro makes sure the raw character RGC has at least NEEDED
   bytes free.  */
#define ENSURE_AVAILABLE(rgc, needed)					\
  while (RAW_CHAR_ALLOCATED (rgc) - RAW_CHAR_USED (rgc) < needed)	\
    {									\
      RAW_CHAR_ALLOCATED (rgc) *= 2;					\
      RAW_CHAR_BYTES (rgc)						\
        = xrealloc (RAW_CHAR_BYTES (rgc), RAW_CHAR_ALLOCATED (rgc));	\
    }


/* Append the byte B to the end of the RAW_CHAR's buffer.  */

static void
append_byte (raw_char_type *raw_char, one_byte b)
{
  ENSURE_AVAILABLE (*raw_char, 1);
  RAW_CHAR_UNUSED_START (*raw_char) = b;
  RAW_CHAR_USED (*raw_char)++;
}


/* Append the N bytes of data pointed to by DATA to the end of
   RAW_CHAR's buffer.  Free DATA after copying.  */

static void
append_n_bytes (raw_char_type *raw_char, four_bytes n, one_byte *data)
{
  ENSURE_AVAILABLE (*raw_char, n);
  memcpy (&RAW_CHAR_UNUSED_START (*raw_char), data, n);
  RAW_CHAR_USED (*raw_char) += n;
  free (data);
}


/* Make a new raw character.  We have to start off the buffer with some
   nonempty amount of memory, so that the reallocation will have
   something to double.  */

static raw_char_type
new_raw_gf_char (gf_char_locator_type locator)
{
  raw_char_type c;
  
  RAW_CHAR_ALLOCATED (c) = 1;
  RAW_CHAR_BYTES (c) = xmalloc (RAW_CHAR_ALLOCATED (c));
  RAW_CHAR_USED (c) = 0;
  
  GF_CHARCODE (c) = locator.charcode;
  GF_H_ESCAPEMENT (c) = locator.h_escapement;
  GF_TFM_WIDTH (c) = locator.tfm_width;
  GF_CHAR_BB (c) = GF_FONT_BB (*private_postamble);
  
  /* Since we don't want to interpret the bounding box information in
     the character, we just make our bounding box be the font bounding
     box.  Since the only thing a raw character is good for is being
     immediately written back out, this doesn't hurt.  (The actual
     bounding box information in the character isn't lost.)  */
  return c;
}



/* Read all specials beginning from the given location, leaving the file
   pointer at the first non-special.  We do not save the specials,
   though.  */

static void
get_specials (byte_count_type special_ptr)
{
  boolean found_special = true;

  GF_FSEEK (special_ptr, SEEK_SET);

  do
    {
      one_byte c = GF_GET_BYTE ();

      switch (c)
	{
	case XXX1:
	  (void) gf_get_string (GF_GET_BYTE ());
	  break;

	case XXX2:
	  (void) gf_get_string (GF_GET_TWO ());
	  break;

	case XXX3:
	  (void) gf_get_string (gf_get_three ());
	  break;

	case YYY:
	  (void) GF_GET_FOUR ();
	  break;

	default:
	  found_special = false;
	  GF_FSEEK (-1, SEEK_CUR);
	}
    }
  while (found_special);
}


static string
gf_get_string (unsigned length)
{
  string s;

  if (length == 0) return "";

  s = get_n_bytes (length, gf_input_file, gf_input_filename);
  s = xrealloc (s, length + 1);
  s[length] = 0;
  return s;
}


/* C does not provide an obvious type for a 24-bit quantity, so we
   return a 32-bit one.  */

static four_bytes
gf_get_three ()
{
  four_bytes v = get_two (gf_input_file, gf_input_filename) << 8;
  v |= get_byte (gf_input_file, gf_input_filename);

  return v;
}
