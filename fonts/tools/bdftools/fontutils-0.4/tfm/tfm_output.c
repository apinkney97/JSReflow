/* tfm_output.c: write property list files (a human-readable equivalent
   of TFM files), and convert them to TFM format.  PL format is
   described in the source code to the TeX utility PLtoTF, by Donald
   Knuth.

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
#include "filename.h"
#include "report.h"
#include "tfm.h"


/* These identify the output file.  (Only one may be open at a time.)  */
static FILE *pl_output_file;
static string pl_output_filename;

/* The design size for the font is used to scale almost all numbers in
   the file.  It is in units of printer's points.  */
static real design_size;


/* Subroutines for output.  */
static void put_lig_kern_info (tfm_char_type);


/* Routines to actually write the properties.  */
static void start_prop_list (string);
static void finish_prop_list (void);
static void start_integer_prop_list (string, unsigned);
static void put_integer_prop (string, unsigned);
static void put_octal_prop (string, unsigned);
static void put_real_prop (string, real);
static void put_scaled_prop (string, real);
static void put_string_prop (string, string);
static void put_indentation (void);

static void put_lig_prop (tfm_ligature_type *);
static void put_kern_prop (tfm_kern_type *);



/* Routines to start and finish reading a file.  (For the user to call.)  */

boolean
tfm_open_pl_output_file (string filename)
{
  assert (pl_output_file == NULL);
  
  pl_output_filename = filename;
  pl_output_file = fopen (filename, "w");

  return pl_output_file != NULL;
}


void
tfm_close_pl_output_file ()
{
  assert (pl_output_file != NULL);

  xfclose (pl_output_file, pl_output_filename);

  pl_output_file = NULL;
  pl_output_filename = NULL;
}


/* An argument to allow the PL file to be deleted might be in order
   here, and maybe returning the exit status.  */

void
tfm_convert_pl (boolean verbose)
{
  string tfm_filename;
  string cmd;
  int status;
  string arg = verbose ? "" : " >/dev/null 2>&1";

  if (pl_output_filename == NULL)
    {
      fprintf (stderr, "No PL output file to convert to TFM format.\n");
      fprintf (stderr, "(Perhaps you called tfm_close_pl_output_file?)\n");
      return;
    }
  
  tfm_filename = make_suffix (pl_output_filename, "tfm");
  cmd = concat5 ("pltotf ", pl_output_filename, " ", tfm_filename, arg);

  /* Make sure that all data we have written will be read.  */
  if (fflush (pl_output_file) == EOF)
    FATAL1 ("%s: could not flush", pl_output_filename);

  /* Convert to TFM.  */
  if (verbose)
    printf ("Executing `%s'.\n", cmd);
  status = system (cmd);

  if (verbose && status != 0)
    printf ("Exit status = %d.\n", status);
}



/* Return an initialized structure.  */

tfm_global_info_type
tfm_init_global_info ()
{
  tfm_global_info_type info;
  unsigned this_param;
  
  TFM_CHECKSUM (info) = 0;
  TFM_DESIGN_SIZE (info) = 0.0;
  TFM_CODING_SCHEME (info) = "unspecified";
  TFM_FONT_PARAMETER_COUNT (info) = 0;
  for (this_param = 1; this_param <= TFM_MAX_FONT_PARAMETERS; this_param++)
    TFM_FONT_PARAMETER (info, this_param) = 0.0;
  
  return info;
}


/* Output the fontwide information in INFO.  */

void
tfm_put_global_info (tfm_global_info_type info)
{
  unsigned this_param;
  
  /* Save the design size so we can scale the other reals.  */
  design_size = TFM_DESIGN_SIZE (info);

  put_string_prop ("comment", now ());
  put_real_prop ("designsize", design_size);
  put_string_prop ("codingscheme", TFM_CODING_SCHEME (info));
  
  /* If the checksum is zero, don't output it; then PLtoTF will compute
     one for us.  */
  if (TFM_CHECKSUM (info) != 0)
    put_octal_prop ("checksum", TFM_CHECKSUM (info));

  start_prop_list ("fontdimen");
  
  for (this_param = 1; this_param <= TFM_FONT_PARAMETER_COUNT (info);
       this_param++)
    {
      real param_value = TFM_FONT_PARAMETER (info, this_param);
      
      /* Parameter #1 (the font slant) is different than all the
         rest, because it's not scaled by the design size.  */
      if (this_param == 1)
        put_real_prop ("slant", param_value);
      else
        put_scaled_prop (concat ("parameter d ", utoa (this_param)),
                         param_value);
    }

  finish_prop_list ();
}


/* We don't bother to keep track of the `nextlarger' and `varchar'
   properties, which are used in TeX's math mode.
   
   TFM_CHAR should be an array of (exactly) `TFM_SIZE' elements.  */

void
tfm_put_chars (tfm_char_type *tfm_char)
{
  unsigned this_char;

  for (this_char = 0; this_char < TFM_SIZE; this_char++, tfm_char++)
    {
      if (TFM_CHAR_EXISTS (*tfm_char))
        tfm_put_char (*tfm_char);
    }
}


/* Output the single character C to the PL file.  */

void
tfm_put_char (tfm_char_type c)
{
  /* If we're being called, the character should exist.  */
  assert (TFM_CHAR_EXISTS (c));
  
  start_integer_prop_list ("character", TFM_CHARCODE (c));

  put_scaled_prop ("charwd", TFM_WIDTH (c));
  put_scaled_prop ("charht", TFM_HEIGHT (c));

  /* Although the width and height are not zero often enough to matter,
     the depth and italic correction are often zero, so we may as well
     not output them.  */
  if (TFM_DEPTH (c) != 0.0)
    put_scaled_prop ("chardp", TFM_DEPTH (c));

  if (TFM_ITALIC_CORRECTION (c) != 0.0)
    put_scaled_prop ("charic", TFM_ITALIC_CORRECTION (c));

  finish_prop_list ();
  
  put_lig_kern_info (c);
}



/* Output any ligature/kern information in the tfm character C.  */

static void
put_lig_kern_info (tfm_char_type c)
{
  unsigned ligature_count = LIST_SIZE (c.ligature);
  unsigned kern_count = LIST_SIZE (c.kern);

  assert (TFM_CHAR_EXISTS (c));
  
  /* Do nothing if there's neither ligatures nor kerns.  */
  if (ligature_count > 0 || kern_count > 0)
    {
      unsigned step;
      
      start_prop_list ("ligtable");
      put_integer_prop ("label", TFM_CHARCODE (c));

      /* We shouldn't output a lig/kern step unless all the characters
         involved exist.  But we don't know which characters exist until
         the end, when we convert to PL.  So for now, we just output
         everything we get, instead of saving the information away.  It
         doesn't actually affect the validity of the results -- it's
         just that PLtoTF might give some warnings.  */
      for (step = 0; step < ligature_count; step++)
        {
          tfm_ligature_type *lig = LIST_ELT (c.ligature, step);
#if 0
          if (TFM_CHAR_EXISTS (tfm_char[lig->character])
              && TFM_CHAR_EXISTS (tfm_char[lig->ligature]))
#endif
            put_lig_prop (lig);
        }

      for (step = 0; step < kern_count; step++)
        {
          tfm_kern_type *k = LIST_ELT (c.kern, step);
#if 0
          if (TFM_CHAR_EXISTS (tfm_char[k->character]))
#endif
            put_kern_prop (k);
        }
      
      put_string_prop ("stop", "");
      finish_prop_list ();
    }
}



/* It makes the output more readable to make the property lists
   indented.  */

static unsigned indentation = 0;
#define INDENT_INCR 2


static void
start_prop_list (string prop_list_name)
{
  put_indentation ();
  fprintf (pl_output_file, "(%s\n", prop_list_name);
  indentation += INDENT_INCR;
}


static void
start_integer_prop_list (string prop, unsigned v)
{
  put_indentation ();
  fprintf (pl_output_file, "(%s d %u\n", prop, v);
  indentation += INDENT_INCR;
}


static void
finish_prop_list ()
{
  put_indentation ();
  fprintf (pl_output_file, ")\n");
  indentation -= INDENT_INCR;
}


static void
put_lig_prop (tfm_ligature_type *lig)
{
  put_indentation ();
  fprintf (pl_output_file, "(lig d %u d %u)\n",
	   lig->character, lig->ligature);
}


static void
put_kern_prop (tfm_kern_type *kern)
{
  put_indentation ();
  fprintf (pl_output_file, "(krn d %u r %f)\n",
	   kern->character, kern->kern / design_size);
}


static void
put_integer_prop (string prop_name, unsigned v)
{
  put_indentation ();
  fprintf (pl_output_file, "(%s d %u)\n", prop_name, v);
}


static void
put_octal_prop (string prop_name, unsigned v)
{
  put_indentation ();
  fprintf (pl_output_file, "(%s o %o)\n", prop_name, v);
}


static void
put_scaled_prop (string prop_name, real v)
{
  put_real_prop (prop_name, v / design_size);
}


static void
put_real_prop (string prop_name, real v)
{
  put_indentation ();
  fprintf (pl_output_file, "(%s r %f)\n", prop_name, v);
}


static void
put_string_prop (string prop_name, string v)
{
  put_indentation ();
  fprintf (pl_output_file, "(%s%s%s)\n", 
           prop_name, strlen (v) > 0 ? " " : "", v);
}


static void
put_indentation ()
{
  unsigned this_space;

  for (this_space = 0; this_space < indentation; this_space++)
    fprintf (pl_output_file, " ");
}
