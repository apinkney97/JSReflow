/* bzr_output.c: write a BZR-format font file.

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

#include "bzr.h"
#include "file-output.h"
#include "scaled-num.h"
#include "spline.h"

#include "bzr_opcodes.h"
#include "bzr_types.h"

/* Where the output will go.  */
static FILE *bzr_output_file = NULL;
static string bzr_output_filename;


/* We remember the design size, so we can scale by it.  */
static real design_size = 0.0;


/* We keep track of the character locator information as we output
   characters, to save the caller from doing the bookkeeping.  */
static char_locator_type char_loc[MAX_CHARCODE + 1];


/* The bounding box of all the characters we have written.  The values
   here should be the largest and smallest possible numbers.  */
static real_bounding_box_type font_bb
  = { FLT_MAX, FLT_MIN, FLT_MAX, FLT_MIN };


/* Low-level output.  These macros call the corresponding routines in
   kbase, using the static variables for the input file and filename.  */

#define BZR_FTELL() xftell (bzr_output_file, bzr_output_filename)
#define BZR_PUT_BYTE(b) put_byte (b, bzr_output_file, bzr_output_filename)
#define BZR_PUT_FOUR(n) put_four (n, bzr_output_file, bzr_output_filename)
#define BZR_PUT_N_BYTES(n, a) \
  put_n_bytes (n, a, bzr_output_file, bzr_output_filename);


static void bzr_put_scaled (real);
static void bzr_put_design_scaled (real);
static void bzr_put_point (real_coordinate_type);



/* Routines to start and end writing a file.  (For the user to call.) 
   We make sure the caller doesn't try to have two output files open
   simultaneously. */

boolean
bzr_open_output_file (string filename)
{
  assert (bzr_output_file == NULL);

  bzr_output_filename = filename;
  bzr_output_file = fopen (filename, "w");

  if (bzr_output_file != NULL)
    {
      unsigned this_char;

      for (this_char = 0; this_char <= MAX_CHARCODE; this_char++)
        CHAR_POINTER (char_loc[this_char]) = NULL_BYTE_PTR;
      return true;
    }
  else
    return false;
}


void
bzr_close_output_file ()
{
  assert (bzr_output_file != NULL);

  xfclose (bzr_output_file, bzr_output_filename);
  bzr_output_filename = NULL;
  bzr_output_file = NULL;
}



/* The preamble.  Since we are supposed to scale many numbers in the BZR
   file by the design size, we squirrel that value away.  */

void
bzr_put_preamble (bzr_preamble_type p)
{
  unsigned comment_length;

  assert (bzr_output_file != NULL);

  bzr_put_scaled (BZR_DESIGN_SIZE (p));
  design_size = BZR_DESIGN_SIZE (p);

  comment_length = strlen (BZR_COMMENT (p));
  if (comment_length > 255)
    {
      string comment = xmalloc (256);

      WARNING2 ("BZR comment `%s' too long (%u characters, which is > 255)",
                BZR_COMMENT (p), comment_length);
      /* We don't want to just put a null into BZR_COMMENT, since it
         might be a read-only string.  */
      comment_length = 255;
      strncpy (comment, BZR_COMMENT (p), comment_length);
      comment[comment_length] = 0;
      BZR_COMMENT (p) = comment;
    }

  BZR_PUT_BYTE (comment_length);
  BZR_PUT_N_BYTES (comment_length, BZR_COMMENT (p));
}



/* Characters.  */

/* Write the character C to `bzr_output_file' (which must already be
   open).  It is a (non-fatal) error to try to write two characters with
   the same character code.  */

void
bzr_put_char (bzr_char_type c)
{
  unsigned this_list;
  spline_list_array_type shape = BZR_SHAPE (c);
  char_locator_type *this_char_loc = &(char_loc[CHARCODE (c)]);

  assert (bzr_output_file != NULL);

  /* We should update the character locator information first, to be
     sure we don't try to output the same character twice.  */
  if (CHAR_POINTER (*this_char_loc) != NULL_BYTE_PTR)
    {
      WARNING1 ("Attempt to output BZR character %u more than once; only \
the first will be written", CHARCODE (c));
      return;
    }

  /* OK, this is a new character.  Remember our current position, for
     writing the postamble.  */
  CHAR_POINTER (*this_char_loc) = BZR_FTELL ();

  /* Now, on to the real business of outputting the character
     definition.  */
  BZR_PUT_BYTE (BOC);
  BZR_PUT_BYTE (CHARCODE (c));
  bzr_put_design_scaled (CHAR_SET_WIDTH (c));
  
  /* We believe the bounding box we are given, since to compute it
     ourselves would require rasterizing the splines, which should not
     be part of this routine.  */
  bzr_put_design_scaled (BZR_CHAR_MIN_COL (c));
  bzr_put_design_scaled (BZR_CHAR_MIN_ROW (c));
  bzr_put_design_scaled (BZR_CHAR_MAX_COL (c));
  bzr_put_design_scaled (BZR_CHAR_MAX_ROW (c));

  /* Update the font bounding box.  */
  if (BZR_CHAR_MIN_COL (c) < MIN_COL (font_bb))
    MIN_COL (font_bb) = BZR_CHAR_MIN_COL (c);
  if (BZR_CHAR_MAX_COL (c) > MAX_COL (font_bb))
    MAX_COL (font_bb) = BZR_CHAR_MAX_COL (c);
  if (BZR_CHAR_MIN_ROW (c) < MIN_ROW (font_bb))
    MIN_ROW (font_bb) = BZR_CHAR_MIN_ROW (c);
  if (BZR_CHAR_MAX_ROW (c) > MAX_ROW (font_bb))
    MAX_ROW (font_bb) = BZR_CHAR_MAX_ROW (c);


  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      spline_type first_spline = SPLINE_LIST_ELT (list, 0);

      BZR_PUT_BYTE (START_PATH);
      bzr_put_point (START_POINT (first_spline));

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == LINEAR)
            {
              BZR_PUT_BYTE (LINE);
              bzr_put_point (END_POINT (s));
            }

          else if (SPLINE_DEGREE (s) == CUBIC)
            {
              BZR_PUT_BYTE (SPLINE);
              bzr_put_point (CONTROL1 (s));
              bzr_put_point (CONTROL2 (s));
              bzr_put_point (END_POINT (s));
            }

          else
            FATAL2 ("bzr_put_char: Spline #%d's degree was %d in out_splines", 
                    this_spline, SPLINE_DEGREE (s));
        }
    }

  BZR_PUT_BYTE (EOC);
}



/* The postamble.  */

void
bzr_put_postamble ()
{
  unsigned c;
  byte_count_type post_ptr;
  unsigned nchars = 0;
  
  assert (bzr_output_file != NULL);

  post_ptr = BZR_FTELL ();
  BZR_PUT_BYTE (POST);

  /* First comes the font bounding box.  */
  bzr_put_design_scaled (MIN_COL (font_bb));
  bzr_put_design_scaled (MIN_ROW (font_bb));
  bzr_put_design_scaled (MAX_COL (font_bb));
  bzr_put_design_scaled (MAX_ROW (font_bb));

  /* Then the character locators.  */
  for (c = 0; c <= MAX_CHARCODE; c++)
    {
      char_locator_type this_char_loc = char_loc[c];

      if (CHAR_POINTER (this_char_loc) != NULL_BYTE_PTR)
        {
          BZR_PUT_BYTE (CHAR_LOC);
          BZR_PUT_BYTE (c);
          BZR_PUT_FOUR (CHAR_POINTER (this_char_loc));
          nchars++;
        }
    }

  /* Then the number of characters.  */
  BZR_PUT_BYTE (nchars);
  
  /* Now a pointer back to the postamble.  */
  BZR_PUT_BYTE (POST_POST);
  BZR_PUT_FOUR (post_ptr);
  BZR_PUT_BYTE (BZR_ID);
  BZR_PUT_BYTE (NO_OP);
}



/* Low-level output routines.  */

static void
bzr_put_scaled (real r)
{
  scaled s;

  if (r > 255.0)
    {
      WARNING1 ("bzr_put_scaled: Value too large to be output (%f)", r);
      r = 255.0;
    }

  s = real_to_scaled (r);

  BZR_PUT_BYTE ((s & 0xff0000) >> 16);
  BZR_PUT_BYTE ((s & 0xff00) >> 8);
  BZR_PUT_BYTE ((s & 0xff));
}


/* We have remembered the design size value from when we output the
   preamble.  */

static void
bzr_put_design_scaled (real r)
{
  bzr_put_scaled (r / design_size);
}


static void
bzr_put_point (real_coordinate_type c)
{
  bzr_put_design_scaled (c.x);
  bzr_put_design_scaled (c.y);
}
