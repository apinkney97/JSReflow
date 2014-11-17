/* metafont.c: output the spline descriptions as a Metafont program.

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
#include "filename.h"
#include "font.h"
#include "spline.h"
#include "tfm.h"

#include "metafont.h"


/* Where the output goes.  */
static FILE *mf_file;
static string mf_filename;


/* Whether or not a particular character has been written.  */
static boolean mf_char_output_p[MAX_CHARCODE + 1];


/* Macros for output to `mf_file'.  */
#define OUT1(str, arg)        fprintf (mf_file, str, arg)
#define OUT2(str, arg1, arg2) fprintf (mf_file, str, arg1, arg2)

#define OUT_LINE(str)			OUT1 ("%s\n", str)
#define OUT_STRING(str)			OUT1 ("%s", str)
#define OUT_STATEMENT(str)		OUT1 ("%s;\n", str)
#define OUT_PT_ASSIGNMENT(lhs, rhs)	OUT2 ("%s := %.5fpt#;\n", lhs, rhs)
#define OUT_ZASSIGNMENT(lhs, l, s, pt) 					\
  do									\
    {									\
      OUT2 (lhs " = ", l, s);						\
      OUT_POINT (pt);							\
      OUT_STRING (";\n");						\
    }									\
  while (0)

/* The variable `offset' must be in scope for this macro.  */
#define OUT_POINT(p) OUT2 ("(%.2fu,%.2fu)", p.x + offset, p.y)

/* How to indent.  */
#define INDENT "  "


static void output_ligtable (tfm_char_type *);


/* Start things off.  We want to write to a Metafont file, e.g.,
   `cmss.mf', given a font name, e.g., `cmss10'.  */

void
metafont_start_output (string output_name, bzr_preamble_type pre, 
		       tfm_global_info_type info)
{
  unsigned this_param;
  string current_time = now ();

  mf_filename = make_output_filename (output_name, "mf");
  mf_file = xfopen (mf_filename, "w");

  /* Output some identification.  */
  OUT1 ("%% This file defines the Metafont font %s.\n", mf_filename);
  OUT1 ("%% {%s}\n", BZR_COMMENT (pre));
  OUT1 ("%% Generated %s.\n", current_time);
  OUT_LINE ("% This font is in the public domain.");
  
  /* Output the size that our original data is based on.  The font can
     be generated at sizes different than this.  */
  OUT_PT_ASSIGNMENT ("true_design_size#", BZR_DESIGN_SIZE (pre));
  
  /* Output the fontwide information given in INFO.  */
  OUT1 ("font_coding_scheme := \"%s\";\n", TFM_CODING_SCHEME (info));
  
  /* The real work is done in an auxiliary file.  */
  OUT_STATEMENT ("input bzrsetup");

  for (this_param = 1; this_param <= TFM_FONT_PARAMETER_COUNT (info);
       this_param++)
    OUT2 ("fontdimen %u: %.2fpt# * u#;\n", this_param,
          TFM_FONT_PARAMETER (info, this_param)); 
}



/* Output one character.  */

void
metafont_output_char (bzr_char_type c)
{
  unsigned this_list;
  spline_list_array_type shape = BZR_SHAPE (c);
  
  int offset = CHAR_LSB (c);

  fprintf (mf_file, "\nbeginchar (%d, %.2fu#, %.2fu#, %.2fu#);\n", 
           CHARCODE (c), CHAR_SET_WIDTH (c), CHAR_HEIGHT (c), CHAR_DEPTH (c));

  /* Go through the list of splines once, assigning the control points
     to Metafont variables.  This allows us to produce more information
     on proofsheets.  */
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);

      OUT_ZASSIGNMENT (INDENT "z%u\\%us", this_list, 0,
                       START_POINT (SPLINE_LIST_ELT (list, 0))); 

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++) 
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == CUBIC)
            {
              OUT_ZASSIGNMENT (INDENT "z%u\\%uc1", this_list, this_spline,
                               CONTROL1 (s)); 
              OUT_ZASSIGNMENT (INDENT "z%u\\%uc2", this_list, this_spline,
                               CONTROL2 (s)); 
            }

          /* The last point in the list is also the first point, and we
             don't want to output two variables for the same point.  */
          if (this_spline < SPLINE_LIST_LENGTH (list) - 1)
            OUT_ZASSIGNMENT (INDENT "z%u\\%u", this_list, this_spline,
                             END_POINT (s));
        }
    }


  /* Go through the list of splines again, outputting the
     path-construction commands.  */

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);

      OUT2 (INDENT "fill_or_unfill z%u\\%us\n", this_list, 0);

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == LINEAR)
            OUT_STRING (INDENT INDENT "--");
          else
            {
              OUT_STRING (INDENT INDENT "..controls ");
              OUT2 ("z%u\\%uc1 and ", this_list, this_spline);
              OUT2 ("z%u\\%uc2..", this_list, this_spline);
            }
	  
          if (this_spline < SPLINE_LIST_LENGTH (list) - 1)
            OUT2 ("z%u\\%u\n", this_list, this_spline);
        }
        
      OUT_STRING ("cycle;\n");
    }
  
  /* The plain Metafont `labels' command makes it possible to produce
     proofsheets with all the points labeled.  We always want labels for
     the starting and ending points, and possibly labels for the control
     points on each spline, so we have our own
     command `proof_labels', defined in `bzrsetup.mf'.  */
  OUT_LINE (INDENT "proof_labels (");
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        OUT2 (INDENT INDENT "%u\\%u,\n", this_list, this_spline);
    }
  OUT_STRING (");\n");

  OUT_STATEMENT ("endchar");
  mf_char_output_p[CHARCODE (c)] = true;
}




/* We use `bye' to finish off the Metafont output, instead of `end'.
   The standard convention is to redefine `bye' to output specials with
   information about the mode selected, to put the font name and other
   information in the TFM file, and so on.  */

void
metafont_finish_output (tfm_char_type *tfm_chars)
{
  output_ligtable (tfm_chars);
  
  OUT_LINE ("\nbye.");
  xfclose (mf_file, mf_filename);
}


/* True if CODE didn't get output to `mf_file', or if the character
   doesn't exist in the TFM file.  (The latter should only occur if the
   TFM file being read doesn't match the bitmap file from which we
   generated the BZR file .)  */
#define NO_CHAR_P(code) \
  (!mf_char_output_p[code] || !TFM_CHAR_EXISTS (tfm_chars[code]))

/* Start or continue the ligtable for the character CODE.  */
#define LIGTABLE_ENTRY(code)						\
  if (output_something)							\
    OUT_LINE (",");							\
  else									\
    {									\
      output_something = true;						\
      OUT1 ("ligtable hex\"%x\":\n", code);				\
    }


/* Output the kerning and ligature information for this font.  Omit it
   for any character which didn't get output.  */

static void
output_ligtable (tfm_char_type *tfm_chars)
{
  unsigned code;
  
  for (code = 0; code <= MAX_CHARCODE; code++)
    {
      list_type kern_list, ligature_list;
      unsigned this_kern, this_lig;
      tfm_char_type c = tfm_chars[code];
      boolean output_something = false;
      
      /* If this character didn't get output, or if we don't have TFM
         information for it, don't do anything.  */
      if (NO_CHAR_P (code))
        continue;
      
      /* Output the kerns.  */
      kern_list = c.kern;
      for (this_kern = 0; this_kern < LIST_SIZE (kern_list); this_kern++)
        {
          tfm_kern_type *kern = LIST_ELT (kern_list, this_kern);

          if (NO_CHAR_P (kern->character))
            continue;

          LIGTABLE_ENTRY (code);
          OUT2 ("hex\"%x\" kern %.4fpt#", kern->character, kern->kern);
        }
      
      /* Output the ligatures.  */
      ligature_list = c.ligature;
      for (this_lig = 0; this_lig < LIST_SIZE (ligature_list); this_lig++)
        {
          tfm_ligature_type *lig = LIST_ELT (ligature_list, this_lig);

          if (NO_CHAR_P (lig->character) || NO_CHAR_P (lig->ligature))
            continue;

	  LIGTABLE_ENTRY (code);
          OUT2 ("hex\"%x\" =: hex\"%x\"", lig->character, lig->ligature);
        }

      if (output_something)
        OUT_LINE (";");
    }
}
