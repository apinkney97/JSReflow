/* output.c: use the `char_type's' information to output a new font.

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

#include "encoding.h"               
#include "filename.h"
#include "gf.h"
#include "libfile.h"
#include "logreport.h"
#include "report.h"
#include "tfm.h"

#include "font.h"
#undef CHAR_LSB
#undef CHAR_RSB
#include "char.h"

#include "main.h"
#include "output.h"


/* The assignments to the TFM fontdimen values.  */
string fontdimens = NULL;

/* The name of the output file specified by the user.  (-output-file)  */
string output_name = NULL;



static gf_char_type *find_widths
  (bitmap_font_type, char_type chars[MAX_CHARCODE + 1], 
   tfm_char_type *);
static void update_from_encoding (tfm_global_info_type *, tfm_char_type *);
static void write_files (string, tfm_global_info_type, tfm_char_type *,
                         bitmap_font_type, gf_char_type *);



/* Output a GF and TFM file with the spacings specified by
   INTERCHAR_SPACE.  Use the characters from BITMAP_FONT for the shapes.
   We use an existing TFM file, an encoding file, and command line
   arguments to determine the metric information.  */

void
output_font (bitmap_font_type bitmap_font, char_type chars[MAX_CHARCODE + 1])
{
  gf_char_type *gf_chars;		/* An array of gf chars.  */
  tfm_global_info_type tfm_info;
  tfm_char_type *tfm_chars;
  /* First see if we have a TFM file to go with BITMAP_FONT.  */
  string tfm_name = find_tfm_filename (font_name);

  if (tfm_name != NULL)
    {
      if (!tfm_open_input_file (tfm_name))
        FATAL1 ("%s: Could not open TFM file", tfm_name);
      tfm_info = tfm_get_global_info ();
      tfm_chars = tfm_get_chars ();
      
      tfm_close_input_file ();
    }
  else
    { /* No TFM file to be found, so initialize the structures
         ourselves.  */
      unsigned this_char;
      
      tfm_info = tfm_init_global_info ();
      TFM_DESIGN_SIZE (tfm_info) = BITMAP_FONT_DESIGN_SIZE (bitmap_font); 
      
      tfm_chars = tfm_new_chars ();
    
      for (this_char = starting_char; this_char <= ending_char; this_char++)
        {
          char_info_type *c = get_char (font_name, this_char);
          real dpi_real = atof (dpi);
          tfm_char_type *tc = &tfm_chars[this_char];
          
          if (c == NULL)
            continue;
            
          TFM_CHAR_EXISTS (*tc) = true;
          TFM_CHARCODE (*tc)= this_char;
          TFM_HEIGHT (*tc) = PIXELS_TO_POINTS (CHAR_HEIGHT (*c), dpi_real);
          TFM_DEPTH (*tc) = PIXELS_TO_POINTS (CHAR_DEPTH (*c), dpi_real);
          /* Leave the italic correction at zero, since I don't know how
             to compute a reasonable value.  And don't bother to compute
             the width, since we do so below.  */
        }
    }

  /* If an encoding file exists, read that.  */
  update_from_encoding (&tfm_info, tfm_chars);

  /* Set the global font parameters from what the user tells us.  */
  tfm_set_fontdimens (fontdimens, &tfm_info);

  /* Compute the side bearings and update the kerning table.  */
  gf_chars = find_widths (bitmap_font, chars, tfm_chars);
  
  /* Output the new bitmap font and the TFM file to go with it.  */
  write_files (tfm_name, tfm_info, tfm_chars, bitmap_font, gf_chars);
}



/* Update the ligature information in TFM_CHARS and the global
   information in TFM_INFO from the file `encoding_name'.enc -- if
   it exists.  */

#define IN_RANGE(code) (starting_char <= (code) && (code) <= ending_char)

static void
update_from_encoding (tfm_global_info_type *tfm_info, tfm_char_type *tfm_chars)
{
  unsigned this_char;
  

  if (encoding_name == NULL)
    return;
    
  /* Assume the encoding file has been read.  */
  

  for (this_char = starting_char; this_char <= ending_char; this_char++)
    {
      if TFM_CHAR_EXISTS (tfm_chars[this_char])
        {
          unsigned this_lig;
          /* Remove any existing ligature information present, and insert
             the new.  */
          list_type old_lig = tfm_chars[this_char].ligature;
          list_type new_lig = ENCODING_CHAR_LIG (encoding_info, this_char);
          list_type checked_lig = new_list ();

          if (LIST_SIZE (old_lig) > 0)
            free (LIST_DATA (old_lig));

          /* Make sure all the ligatures use and yield only characters that
             will be in the font.  */
          if (ENCODING_CHAR_NAME (encoding_info, this_char) != NULL)
            {
              for (this_lig = 0; this_lig < LIST_SIZE (new_lig); this_lig++)
                {
                  tfm_ligature_type *l = LIST_ELT (new_lig, this_lig);

                  if (IN_RANGE (l->character) && IN_RANGE (l->ligature)
                      && TFM_CHAR_EXISTS (tfm_chars[l->character])
                      && TFM_CHAR_EXISTS (tfm_chars[l->ligature]))
                    {
                      tfm_ligature_type *new_l 
                        = append_element (&checked_lig,
			                  sizeof (tfm_ligature_type));
                      *new_l = *l;
                    }
                }

              free (LIST_DATA (new_lig));
            }

          tfm_chars[this_char].ligature = checked_lig;
       }
    }
  
  TFM_CODING_SCHEME (*tfm_info) = ENCODING_SCHEME_NAME (encoding_info);
}



/* Using an array of `char_type's, return an array of `gf_char_type's
   with the new sidebearings and set widths.  Update TFM_CHARS with the
   new set widths.  */
   
static gf_char_type *
find_widths (bitmap_font_type font, 
	     char_type chars[MAX_CHARCODE + 1],
             tfm_char_type *tfm_chars)
{
  unsigned this_char;
  unsigned char_count = 0;
  real design_size = BITMAP_FONT_DESIGN_SIZE (font);
  real dpi_real = atof (dpi);
  /* We waste a little memory for each character that doesn't exist
     here; but since fonts tend to be quite full in practice, it doesn't
     really matter.  */
  gf_char_type *gf_chars = XTALLOC (MAX_CHARCODE + 1, gf_char_type);
  
  /* Report the side bearings.  */
  REPORT ("Side bearings: ");
  LOG1 ("\n\n\f\nOne pixel = %.3lfpt.\n", PIXELS_TO_POINTS (1, dpi_real));
  
  for (this_char = starting_char; this_char <= ending_char; this_char++)
    if (TFM_CHAR_EXISTS (tfm_chars[this_char]))
      {
        int pixel_width;
        real point_width, real_width;
        char_info_type *c = get_char (font_name, this_char);
	unsigned this_kern;
        
        if (c == NULL)
          {
            WARNING1 ("Character %d exists in the TFM file, \
but not the bitmap file", this_char);
            continue;
          }
        LOG1 ("\nChar %d", this_char);

        if (encoding_name != NULL)
          LOG1 (" (%s): ", ENCODING_CHAR_NAME (encoding_info, this_char));
        else
          LOG (": ");
        
        real_width = (CHAR_LSB (chars[this_char]) 
		      + CHAR_BITMAP_WIDTH (*c)
		      + CHAR_RSB (chars[this_char]));

        pixel_width = ROUND (real_width);
        point_width = PIXELS_TO_POINTS (real_width, dpi_real);

        /* Construct the character structure we will output.  */
        GF_CHARCODE (gf_chars[this_char]) = this_char;
        GF_BITMAP (gf_chars[this_char]) = CHAR_BITMAP (*c);
        GF_CHAR_MIN_ROW (gf_chars[this_char]) = CHAR_MIN_ROW (*c);
        GF_CHAR_MAX_ROW (gf_chars[this_char]) = CHAR_MAX_ROW (*c);
        GF_CHAR_MIN_COL (gf_chars[this_char]) = CHAR_LSB (chars[this_char]);
        GF_CHAR_MAX_COL (gf_chars[this_char])
          = CHAR_LSB (chars[this_char]) + CHAR_BITMAP_WIDTH (*c);
        GF_H_ESCAPEMENT (gf_chars[this_char]) = pixel_width;
        
        /* We want the same `fix_word' width to end up in the GF and TFM
           files.  The TFM output routines do the scaling by the design
           size, for us, hence we have to assign different values here.  */
        TFM_WIDTH (tfm_chars[this_char]) = point_width;
        GF_TFM_WIDTH (gf_chars[this_char])
          = real_to_fix (point_width / design_size);
        
        REPORT1 (".%s", ++char_count % 64 == 0 ? "\n" : "");
        LOG3 ("\n  lsb=%.3f, rsb=%.3f, width=%.3f\n",
              CHAR_LSB (chars[this_char]), 
              CHAR_RSB (chars[this_char]), real_width);
	
        LOG1 ("Char %d kerns:", this_char);

        /* Do the kerns.  */

        /* Throw away the old kerns.  */
        if (LIST_SIZE (TFM_KERN (tfm_chars[this_char])) > 0)
          {
            free (LIST_DATA (TFM_KERN (tfm_chars[this_char])));
	    /* So don't try to reallocate with freed space.  */
            LIST_DATA (TFM_KERN (tfm_chars[this_char])) = NULL;
            LIST_SIZE (TFM_KERN (tfm_chars[this_char])) = 0;
          }

         for (this_kern = 0; 
	      this_kern < CHAR_KERNS_SIZE (chars[this_char]);
	      this_kern++)
	   {
	     real kern_in_points 
               = PIXELS_TO_POINTS (
                 get_kern_element (chars[this_char], this_kern), dpi_real);

               tfm_set_kern (&tfm_chars[this_char], 
	       get_kern_character_element (chars[this_char], this_kern),
               kern_in_points);

             LOG2 (" %d/%.3f", 
	       get_kern_character_element (chars[this_char], this_kern),
               get_kern_element (chars[this_char], this_kern));
           }
      }

  if (char_count % 64 != 0)
    REPORT ("\n");

  return gf_chars;
}



/* Output new GF and TFM files with the information in TFM_INFO,
   TFM_CHARS, and GF_CHARS.  Don't overwrite existing files named TFM_NAME
   or `BITMAP_FONT_FILENAME (FONT)'.  */

static void
write_files (string tfm_name, tfm_global_info_type tfm_info,
             tfm_char_type *tfm_chars, bitmap_font_type font,
             gf_char_type *gf_chars)
{
  unsigned this_char;
  unsigned char_count = 0;
  /* We depend upon `output_name' being set when we are called.  */
  string gf_output_name = make_output_filename (output_name,
						concat (dpi, "gf"));
  string pl_output_name = make_output_filename (output_name, "pl");
  string tfm_output_name = make_output_filename (output_name, "tfm");
  

  if (same_file_p (BITMAP_FONT_FILENAME (font), gf_output_name))
    {
      gf_output_name = make_prefix ("x", gf_output_name);
      WARNING2 ("charspace: The output file (%s) is also the \
the input file, so I am writing to `x%s'", gf_output_name, gf_output_name);
    }

  if (same_file_p (BITMAP_FONT_FILENAME (font), tfm_output_name))
    {
      tfm_output_name = make_prefix ("x", tfm_output_name);
      WARNING2 ("charspace: The output file (%s) is also the \
the input file, so I am writing to `x%s'", tfm_output_name, tfm_output_name);
    }


  /* Let's write the TFM file first, but to a PL file, which is easier.  */
  if (!tfm_open_pl_output_file (pl_output_name))
    FATAL_PERROR (pl_output_name);
  tfm_put_global_info (tfm_info);

  /* Throw away the characters that aren't in the user's range.  */
  for (this_char = 0; this_char <= MAX_CHARCODE; this_char++)
    if (this_char < starting_char || this_char > ending_char)
      TFM_CHAR_EXISTS (tfm_chars[this_char]) = false;

  REPORT ("\nWriting PL file:\n");
  for (this_char = 0; this_char < TFM_SIZE; this_char++)
    {
      if (TFM_CHAR_EXISTS (tfm_chars[this_char]))
 	{
          tfm_put_char (tfm_chars[this_char]);
          REPORT2 ("[%d]%c", this_char, ++char_count % 13 == 0 ? '\n' : ' ');
        }
    }
  if (char_count % 13 != 0)
    REPORT ("\n");

  tfm_convert_pl (verbose);	      /* Make the TFM file from the PL one.  */
  tfm_close_pl_output_file ();
  
  if (!no_gf)
    {
      /* Now the GF file.  */
      if (!gf_open_output_file (gf_output_name))
        FATAL_PERROR (gf_output_name);

      REPORT ("\nWriting GF file:\n");
      gf_put_preamble (concat4 ("charspace output ", now () + 4, " from ",
                                BITMAP_FONT_COMMENT (font)));

      for (this_char = starting_char, char_count = 0; 
           this_char <= ending_char; 
           this_char++)
        {
          if (TFM_CHAR_EXISTS (tfm_chars[this_char]))
            {
              gf_put_char (gf_chars[this_char]);
              REPORT2 ("[%d]%c", this_char, ++char_count % 13 == 0 ? '\n' : ' ');
            }
        }

      gf_put_postamble (real_to_fix (BITMAP_FONT_DESIGN_SIZE (font)),
                        atof (dpi), atof (dpi));

      gf_close_output_file ();

      if (char_count % 13 != 0)
        REPORT ("\n");
    }
}
