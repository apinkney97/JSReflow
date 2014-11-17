/* gf_output.c: write objects to one GF file.

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

#include "file-output.h"
#include "gf.h"
#include "scaled-num.h"

#include "gf_opcodes.h"


/* The file we are writing to.  */
static FILE *gf_output_file = NULL;
static string gf_output_filename;


/* The bounding box of all the characters we have written.  The values
   here should be the largest and smallest possible integers.  */
static bounding_box_type font_bb
  = { INT_MAX, INT_MIN, INT_MAX, INT_MIN };


/* We keep track of the character locator information as we output
   characters, to save the caller from doing the bookkeeping.  */
static gf_char_locator_type char_loc[MAX_CHARCODE + 1];

static void init_locators (void);
static void put_locators (void);


/* Other parts of the GF file that deserve their own routines.  */
static void put_boc (gf_char_type);
static void put_bitmap (bitmap_type);


/* Subroutines for writing the bitmap.  */
static void put_paint (unsigned);
static void put_skip (unsigned);
static void put_new_row (unsigned);



/* Low-level output.  These macros call the corresponding routines in
   kbase, using the static variables for the input file and filename.  */

#define GF_FTELL() xftell (gf_output_file, gf_output_filename)
#define GF_PUT_BYTE(b) put_byte (b, gf_output_file, gf_output_filename)
#define GF_PUT_TWO(b) put_two (b, gf_output_file, gf_output_filename)
#define GF_PUT_THREE(n) put_three (n, gf_output_file, gf_output_filename)
#define GF_PUT_FOUR(n) put_four (n, gf_output_file, gf_output_filename)
#define GF_PUT_SIGNED_FOUR(n) \
  put_signed_four (n, gf_output_file, gf_output_filename);



/* Routines to start and end writing a file.  (For the user to call.)
   We make sure the caller can't have two output files open
   simultaneously. */

boolean
gf_open_output_file (string filename)
{
  assert (gf_output_file == NULL);

  gf_output_filename = filename;
  gf_output_file = fopen (filename, "w");

  if (gf_output_file != NULL)
    {
      init_locators ();
      return true;
    }
  else
    return false;
}


void
gf_close_output_file ()
{
  assert (gf_output_file != NULL);

  xfclose (gf_output_file, gf_output_filename);

  gf_output_filename = NULL;
  gf_output_file = NULL;
}



/* Write the preamble.  */

void
gf_put_preamble (string comment)
{
  unsigned comment_length, c;

  assert (gf_output_file != NULL);

  GF_PUT_BYTE (PRE);
  GF_PUT_BYTE (GF_ID);

  comment_length = strlen (comment);

  if (comment_length > 255)
    comment_length = 255;

  GF_PUT_BYTE (comment_length);
  /* We can't just output all the characters in the string, since the
     string might have been too long.  */
  for (c = 0; c < comment_length; c++)
    GF_PUT_BYTE (*(comment + c));
}



/* Write the postamble.  */

void
gf_put_postamble (fix_word design_size, real h_resolution, real v_resolution)
{
  byte_count_type post_ptr;
  unsigned i;

  assert (gf_output_file != NULL);

  fflush (gf_output_file);
  post_ptr = GF_FTELL ();
  GF_PUT_BYTE (POST);
  GF_PUT_FOUR (post_ptr);   /* No specials before the postamble.  */
  GF_PUT_FOUR (design_size);
  GF_PUT_FOUR (0);		/* Don't bother with a checksum.  */

  /* The resolution values are given to us in pixels per inch, but we
     must write them in pixels per point (as well as scaling them).  */
  GF_PUT_FOUR (real_to_scaled (h_resolution / POINTS_PER_INCH));
  GF_PUT_FOUR (real_to_scaled (h_resolution / POINTS_PER_INCH));

  GF_PUT_SIGNED_FOUR (MIN_COL (font_bb));
  GF_PUT_SIGNED_FOUR (MAX_COL (font_bb));
  GF_PUT_SIGNED_FOUR (MIN_ROW (font_bb));
  GF_PUT_SIGNED_FOUR (MAX_ROW (font_bb));

  put_locators ();

  GF_PUT_BYTE (POST_POST);
  GF_PUT_FOUR (post_ptr);
  GF_PUT_BYTE (GF_ID);

  for (i = 0; i < 4; i++)
    GF_PUT_BYTE (GF_SIGNATURE);
}


/* Here we output the information we have accumulated in the `char_loc'
   array.  GF format allows for two different `char_loc' commands, one
   more efficient (but less general) than the other.  We use the
   appropriate one.  */

static void
put_locators ()
{
  unsigned this_char;

  for (this_char = 0; this_char <= MAX_CHARCODE; this_char++)
    {
      gf_char_locator_type this_char_loc = char_loc[this_char];

      if (this_char_loc.char_pointer != NULL_BYTE_PTR)
	{
	  int h_escapement = this_char_loc.h_escapement;

	  /* Decide if we can use CHAR_LOC0.  */
	  if (h_escapement >= 0 && h_escapement < ONE_BYTE_BIG)
	    {
	      GF_PUT_BYTE (CHAR_LOC0);
	      GF_PUT_BYTE (this_char);
	      GF_PUT_BYTE (h_escapement);
	    }
	  else
	    {
	      GF_PUT_BYTE (CHAR_LOC);
	      GF_PUT_BYTE (this_char);
	      GF_PUT_FOUR (real_to_scaled ((real) h_escapement));
	      GF_PUT_FOUR (0);	/* No vertical escapement.  */
	    }

	  GF_PUT_FOUR (this_char_loc.tfm_width);
	  GF_PUT_SIGNED_FOUR (this_char_loc.char_pointer);
	}
    }
}


/* We must initialize the character locators to all null values.  */

static void
init_locators ()
{
  unsigned this_char;

  for (this_char = 0; this_char <= MAX_CHARCODE; this_char++)
    char_loc[this_char].char_pointer = NULL_BYTE_PTR;
}



/* Do what's necessary to prepare for outputting a character.  */

static void
start_put_char (one_byte charcode, unsigned h_escapement,
                fix_word tfm_width, bounding_box_type char_bb)
{
  gf_char_locator_type *this_char_loc = &(char_loc[charcode]);

  assert (gf_output_file != NULL);

  if (this_char_loc->char_pointer != NULL_BYTE_PTR)
    {
      WARNING1 ("start_put_char: Character %u already output", charcode);
      return;
    }

  /* Update the character locator information (part of the postamble).  */
  this_char_loc->char_pointer = GF_FTELL ();
  this_char_loc->h_escapement = h_escapement;
  this_char_loc->tfm_width = tfm_width;
  this_char_loc->charcode = charcode;

  /* Update the font bounding box.  */
  if (MIN_COL (char_bb) < MIN_COL (font_bb))
    MIN_COL (font_bb) = MIN_COL (char_bb);
  if (MAX_COL (char_bb) > MAX_COL (font_bb))
    MAX_COL (font_bb) = MAX_COL (char_bb);
  if (MIN_ROW (char_bb) < MIN_ROW (font_bb))
    MIN_ROW (font_bb) = MIN_ROW (char_bb);
  if (MAX_ROW (char_bb) > MAX_ROW (font_bb))
    MAX_ROW (font_bb) = MAX_ROW (char_bb);
}


/* Output the character GF_CHAR.  */

void
gf_put_char (gf_char_type gf_char)
{
  one_byte charcode = GF_CHARCODE (gf_char);
  
  start_put_char (charcode, GF_H_ESCAPEMENT (gf_char),
                  GF_TFM_WIDTH (gf_char), GF_CHAR_BB (gf_char));
  put_boc (gf_char);
  put_bitmap (GF_BITMAP (gf_char));
  GF_PUT_BYTE (EOC);
}


/* Output the BOC command that begins each character.  BOC commands come
   in two flavors, one of which takes less space but is less general.  */

static void
put_boc (gf_char_type gf_char)
{
  signed_4_bytes row_delta = (GF_CHAR_MAX_ROW (gf_char)
                              - GF_CHAR_MIN_ROW (gf_char));
  signed_4_bytes col_delta = (GF_CHAR_MAX_COL (gf_char)
                              - GF_CHAR_MIN_COL (gf_char));

  /* Decide if we can use BOC1, the abbreviated form.  */
  if (row_delta < ONE_BYTE_BIG && row_delta >= 0
      && col_delta < ONE_BYTE_BIG && col_delta >= 0
      && GF_CHAR_MAX_ROW (gf_char) < ONE_BYTE_BIG
      && GF_CHAR_MAX_ROW (gf_char) >= 0
      && GF_CHAR_MAX_COL (gf_char) < ONE_BYTE_BIG
      && GF_CHAR_MAX_COL (gf_char) >= 0)
    {
      GF_PUT_BYTE (BOC1);
      GF_PUT_BYTE (GF_CHARCODE (gf_char));
      GF_PUT_BYTE (col_delta);
      GF_PUT_BYTE (GF_CHAR_MAX_COL (gf_char));
      GF_PUT_BYTE (row_delta);
      GF_PUT_BYTE (GF_CHAR_MAX_ROW (gf_char));
    }
  else
    {
      GF_PUT_BYTE (BOC);
      GF_PUT_FOUR (GF_CHARCODE (gf_char));
      GF_PUT_SIGNED_FOUR (NULL_BYTE_PTR);   /* We never have a backpointer.  */
      GF_PUT_SIGNED_FOUR (GF_CHAR_MIN_COL (gf_char));
      GF_PUT_SIGNED_FOUR (GF_CHAR_MAX_COL (gf_char));
      GF_PUT_SIGNED_FOUR (GF_CHAR_MIN_ROW (gf_char));
      GF_PUT_SIGNED_FOUR (GF_CHAR_MAX_ROW (gf_char));
    }
}


/* Bitmaps in GF format are run-encoded.  There are three commands:
   `paint', which writes a run of pixels (either black or white);
   `skip', which leaves entirely blank rows; and `new_row', which begins
   a new row with the first black pixel some number of pixels from the
   left edge.  */

static void
put_bitmap (bitmap_type b)
{
  unsigned run = 0;		/* Length of the current run.  */
  one_byte current_color = WHITE;
  boolean recent_eol = false;	/* Have we seen only white since eol?  */
  unsigned all_white = 0;	/* Count of entirely blank rows.  */
  unsigned this_row, this_col;

  for (this_row = 0; this_row < BITMAP_HEIGHT (b); this_row++)
    {
      for (this_col = 0; this_col < BITMAP_WIDTH (b); this_col++)
	{
	  one_byte p = BITMAP_PIXEL (b, this_row, this_col);

	  if (p == current_color)
            run++;
	  else
	    {
	      if (recent_eol)
		{		/* First transition to black on new row?  */
		  recent_eol = false;
		  put_skip (all_white);
		  all_white = 0;
		  /* We've seen only white up to now, since
                     `current_color' is zero after each row.  */
		  put_new_row (run);
		}
	      else
               put_paint (run);

	      current_color = p;
	      run = 1;
	    }
	}

      if (current_color == BLACK)
	{
	  put_paint (run);	/* The row ended with black.  */
	  current_color = WHITE;
	}

      else if (recent_eol)
        all_white++;		/* The row was entirely white.  */

      recent_eol = true;
      run = 0;
    }
}


/* Routines to output the actual GF commands that describe the bitmap. 
   We use the shortest possible command.  */

static void
put_paint (unsigned run)
{
   if (run < 64)
     GF_PUT_BYTE (run);
   else if (run < ONE_BYTE_BIG)
     {
       GF_PUT_BYTE (PAINT1);
       GF_PUT_BYTE (run);
     }
   else if (run < TWO_BYTES_BIG)
     {
       GF_PUT_BYTE (PAINT2);
       GF_PUT_TWO (run);
     }
   else if (run < THREE_BYTES_BIG)
     {
       GF_PUT_BYTE (PAINT3);
       GF_PUT_THREE (run);
     }
   else
     FATAL1 ("put_paint: Run of %u pixels too long for GF format", run);
}


static void
put_skip (unsigned all_white)
{
  if (all_white == 0)
    ; /* Do nothing.  */
  else if (all_white == 1)
    GF_PUT_BYTE (SKIP0);
  else if (all_white < ONE_BYTE_BIG)
    {
      GF_PUT_BYTE (SKIP1);
      GF_PUT_BYTE (all_white - 1);
    }
  else if (all_white < TWO_BYTES_BIG)
    {
      GF_PUT_BYTE (SKIP2);
      GF_PUT_TWO (all_white - 1);
    }
  else if (all_white < THREE_BYTES_BIG)
    {
      GF_PUT_BYTE (SKIP3);
      GF_PUT_THREE (all_white - 1);
    }
  else
    FATAL1 ("put_skip: %u rows is too many for GF format", all_white);
}


static void
put_new_row (unsigned indent)
{
  if (indent <= 164)
    GF_PUT_BYTE (NEW_ROW_0 + indent);
  else
    {
      /* Too large for a new_row command; have to skip and then paint.  */
      GF_PUT_BYTE (SKIP0);
      put_paint (indent);
    }
}


/* Output a raw character.  */

void
gf_put_raw_char (raw_char_type raw_char)
{
  one_byte charcode = GF_CHARCODE (raw_char);
  
  start_put_char (charcode, GF_H_ESCAPEMENT (raw_char),
                  GF_TFM_WIDTH (raw_char), GF_CHAR_BB (raw_char));
  put_n_bytes (RAW_CHAR_USED (raw_char), RAW_CHAR_BYTES (raw_char),
               gf_output_file, gf_output_filename);
}
