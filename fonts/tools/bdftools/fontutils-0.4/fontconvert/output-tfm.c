/* output-tfm.c: write a TFM file.

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

#include "filename.h"
#include "filter.h"
#include "main.h"
#include "random.h"
#include "tfm.h"

#include "output-tfm.h"


/* The assignments to the TFM character header values.  */
string tfm_header = NULL;

/* The assignments to the TFM fontdimen values.  */
string fontdimens = NULL;

/* This accumulates the information about each character.  */
static tfm_char_type *tfm_chars;

/* This holds the information about each character old TFM character.  */
static tfm_char_type *old_tfm_chars;


static void tfm_do_chars_defaults ();



/* At the beginning, we can set up the output file and write the fontwide
   information.  This includes the fontdimen parameters, in the global
   `fontdimens', if the user specified them.  */
   
void
tfm_start_output (string input_basename, string output_name,
		  real user_design_size, real default_design_size)
{
  tfm_global_info_type tfm_info;
  string tfm_name = find_tfm_filename (input_basename);
  string pl_name = make_output_filename (output_name, "pl");
  real tfm_file_design_size;

  if (!tfm_open_pl_output_file (pl_name))
    FATAL_PERROR (pl_name);
  
  /* See if we can find a TFM file from which to get defaults.  */
  if (tfm_name != NULL && tfm_open_input_file (tfm_name))
    {
      tfm_info = tfm_get_global_info ();
      tfm_file_design_size = TFM_DESIGN_SIZE (tfm_info);
      TFM_DESIGN_SIZE (tfm_info) = 0.0;

      /* Never carry along the checksum.  */
      TFM_CHECKSUM (tfm_info) = 0;
    }
  else
    tfm_info = tfm_init_global_info ();
    

  /* Set `tfm_info' according to the user's string(s).  */

  if (tfm_header != NULL)
    tfm_set_header (tfm_header, &tfm_info);
      
  if (fontdimens != NULL)
    tfm_set_fontdimens (fontdimens, &tfm_info);
  

  /* Set design size if it wasn't set by the header option: first with the
     global design size the user may have requested, then with the one
     from the TFM file, and then with the global default one.  */

  if (TFM_DESIGN_SIZE (tfm_info) == 0.0 && user_design_size != 0.0)
    tfm_set_design_size (user_design_size, &tfm_info);

  if (TFM_DESIGN_SIZE (tfm_info) == 0.0)
    tfm_set_design_size (tfm_file_design_size, &tfm_info);

  if (TFM_DESIGN_SIZE (tfm_info) == 0.0)
    tfm_set_design_size (default_design_size, &tfm_info);


  /* Output this to start us off.  */
  tfm_put_global_info (tfm_info);

  tfm_chars = tfm_new_chars ();
}


/* Store the relevant information about C into `tfm_chars'.  */

void
tfm_output_char (char_info_type c, real dpi_real)
{
  tfm_char_type *old_tfm_char = tfm_get_char (CHARCODE (c));
  tfm_char_type *new_tfm_char = &tfm_chars[CHARCODE (c)];
  
  TFM_CHAR_EXISTS (*new_tfm_char) = true;
  TFM_CHARCODE (*new_tfm_char) = CHARCODE (c);
  
  if (baseline_adjust[CHARCODE (c)] != NULL
      || column_split[CHARCODE (c)] != NULL
      || filter_passes > 0 || random_max > 0.0)
    {
      TFM_WIDTH (*new_tfm_char)
        = PIXELS_TO_POINTS (CHAR_SET_WIDTH (c), dpi_real);
  
      TFM_HEIGHT (*new_tfm_char) 
        = PIXELS_TO_POINTS (CHAR_HEIGHT (c), dpi_real);
  
      TFM_DEPTH (*new_tfm_char) = PIXELS_TO_POINTS (CHAR_DEPTH (c), dpi_real);
      TFM_ITALIC_CORRECTION (*new_tfm_char) = 0.0;
    }
   else
    {
      TFM_WIDTH (*new_tfm_char) = TFM_WIDTH (*old_tfm_char);
      TFM_HEIGHT (*new_tfm_char) = TFM_HEIGHT (*old_tfm_char);
      TFM_DEPTH (*new_tfm_char) = TFM_DEPTH (*old_tfm_char);
      TFM_ITALIC_CORRECTION (*new_tfm_char) 
	= TFM_ITALIC_CORRECTION (*old_tfm_char);
    }

  /* Do ligatures and kerns when have gone through all the characters. */  
}


/* Output `tfm_chars' and close up.  */

void
tfm_finish_output ()
{
  tfm_do_chars_defaults ();
  tfm_put_chars (tfm_chars);
  tfm_convert_pl (verbose);
  tfm_close_input_file ();
  tfm_close_pl_output_file ();
}



static void
tfm_do_chars_defaults ()
{
  unsigned code;

  old_tfm_chars = tfm_get_chars ();
  
  for (code = 0; code < TFM_SIZE; code++)
    {
      tfm_char_type *new_tfm_char = &tfm_chars[code];
      
      if (TFM_CHAR_EXISTS (*new_tfm_char))
        {
	  tfm_char_type *old_tfm_char = &old_tfm_chars[code];
	  list_type old_kern_table = TFM_KERN (*old_tfm_char);
	  list_type old_lig_table = TFM_LIGATURE (*old_tfm_char);
          unsigned this_one;
          
          for (this_one = 0; this_one < LIST_SIZE (old_kern_table); this_one++)
            {
              tfm_kern_type *old_kern = LIST_ELT (old_kern_table, this_one);
	      tfm_char_type *old_kern_char
                = &tfm_chars[translate[old_kern->character]];
	      
              if (TFM_CHAR_EXISTS (*old_kern_char))
	        tfm_set_kern (new_tfm_char, CHARCODE (*old_kern_char),
		              old_kern->kern);
            }

          for (this_one = 0; this_one < LIST_SIZE (old_lig_table); this_one++)
	    {
              tfm_ligature_type *old_lig = LIST_ELT (old_lig_table, this_one);
              tfm_char_type *old_lig_char
                = &tfm_chars[translate[old_lig->character]];
              tfm_char_type *old_lig_ligature
                = &tfm_chars[translate[old_lig->ligature]];
	      
              if (TFM_CHAR_EXISTS (*old_lig_char) 
                  && TFM_CHAR_EXISTS (*old_lig_ligature))
	        tfm_set_ligature (new_tfm_char, CHARCODE (*old_lig_char),
				  CHARCODE (*old_lig_ligature));
            }
        }  /* if new TFM char exists.  */
    }
}
