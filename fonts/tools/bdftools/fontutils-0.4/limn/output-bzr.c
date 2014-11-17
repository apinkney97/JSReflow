/* output-bzr.c: write the BZR output file.

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
#include "main.h"
#include "output-bzr.h"

/* The name of the output file.  (-output-file)  */
string output_name = NULL;


/* Most of the data is coordinates, so we make changing that convenient.  */
#define POINT_FROM_PIXELS_TO_POINTS(p)					\
  (p).x = PIXELS_TO_POINTS ((p).x, dpi_real);				\
  (p).y = PIXELS_TO_POINTS ((p).y, dpi_real)



/* Open the output file and write the preamble information.  */

void
bzr_start_output (string output_name, bitmap_font_type font)
{
  bzr_preamble_type p;
  string bzr_name = make_output_filename (output_name, "bzr");
  
  bzr_open_output_file (bzr_name);
  
  BZR_DESIGN_SIZE (p) = BITMAP_FONT_DESIGN_SIZE (font);
  
  /* We don't need the day of the week in the comment, hence the `+4'.  */
  BZR_COMMENT (p) = concat4 ("limn output ", now () + 4,
                             BITMAP_FONT_COMMENT (font) ? " from " : "",
                             BITMAP_FONT_COMMENT (font));
  
  bzr_put_preamble (p);
  free (bzr_name);
}



/* Write the information for one character.  */

void
bzr_output_char (char_info_type c, spline_list_array_type in_splines)
{
  bzr_char_type bzr_char;
  unsigned this_list;
  spline_list_array_type out_splines = new_spline_list_array ();
  real dpi_real = atof (dpi);
  
  CHARCODE (bzr_char) = CHARCODE (c);
  CHAR_SET_WIDTH (bzr_char) = PIXELS_TO_POINTS (CHAR_SET_WIDTH (c), dpi_real);

  BZR_CHAR_MIN_COL (bzr_char) = PIXELS_TO_POINTS (CHAR_MIN_COL (c), dpi_real);
  BZR_CHAR_MAX_COL (bzr_char) = PIXELS_TO_POINTS (CHAR_MAX_COL (c), dpi_real);
  BZR_CHAR_MIN_ROW (bzr_char) = PIXELS_TO_POINTS (CHAR_MIN_ROW (c), dpi_real);
  BZR_CHAR_MAX_ROW (bzr_char) = PIXELS_TO_POINTS (CHAR_MAX_ROW (c), dpi_real);

  /* We have to change all the values in the spline list to be in
     points, instead of pixels.  */
  
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (in_splines);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type in_list = SPLINE_LIST_ARRAY_ELT (in_splines, this_list);
      spline_list_type out_list = *new_spline_list ();
      
      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (in_list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (in_list, this_spline);
          
          if (SPLINE_DEGREE (s) == LINEAR)
            {
              POINT_FROM_PIXELS_TO_POINTS (START_POINT (s));
              POINT_FROM_PIXELS_TO_POINTS (END_POINT (s));
            }
          else
            { /* It's a cubic.  */
              POINT_FROM_PIXELS_TO_POINTS (START_POINT (s));
              POINT_FROM_PIXELS_TO_POINTS (CONTROL1 (s));
              POINT_FROM_PIXELS_TO_POINTS (CONTROL2 (s));
              POINT_FROM_PIXELS_TO_POINTS (END_POINT (s));
            }

          append_spline (&out_list, s);
        }
      
      append_spline_list (&out_splines, out_list);
    }
    
  BZR_SHAPE (bzr_char) = out_splines;
  
  bzr_put_char (bzr_char);
}



/* Finish off the output file, and close it.  */

void
bzr_finish_output ()
{
  bzr_put_postamble ();
  bzr_close_output_file ();
}
