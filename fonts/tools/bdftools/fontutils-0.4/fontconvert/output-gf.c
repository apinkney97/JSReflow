/* output-gf.c: write a GF file.

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
#include "gf.h"

#include "output-gf.h"


/* Open the output file and write the preamble.  We don't bother to
   change the comment to identify ourselves, since the changes we will
   make aren't likely to be drastic.  We have to ensure that we do not
   overwrite the input file; if INPUT_NAME and our constructed output
   name do refer to the same file, we simply prepend an `x' to the
   output name.  Not very elegant, but it does solve the problem.  */

void
gf_start_output (string input_name, string output_name, string dpi,
                 string comment)
{
  string gf_name = make_output_filename (output_name, concat (dpi, "gf"));
  
  if (same_file_p (input_name, gf_name))
    {
      WARNING2 ("fontconvert: The output file (%s) is also the \
the input file, so I am writing to `x%s'", gf_name, gf_name);
      gf_name = make_prefix ("x", gf_name);
    }

  if (!gf_open_output_file (gf_name))
    FATAL_PERROR (gf_name);
  
  gf_put_preamble (comment);
}


/* Write a single character.   */

void
gf_output_char (char_info_type c, real design_size_ratio)
{
  gf_char_type gf_char;
  
  GF_CHARCODE (gf_char) = CHARCODE (c);
  GF_BITMAP (gf_char) = CHAR_BITMAP (c);
  GF_CHAR_BB (gf_char) = CHAR_BB (c);
  GF_H_ESCAPEMENT (gf_char) = CHAR_SET_WIDTH (c);
  GF_TFM_WIDTH (gf_char) = design_size_ratio * CHAR_TFM_WIDTH (c);
  
  gf_put_char (gf_char);
}


/* Write the postamble and close the file.  */

void
gf_finish_output (real design_size, real dpi)
{
  gf_put_postamble (real_to_fix (design_size), dpi, dpi);
  gf_close_output_file ();
}

