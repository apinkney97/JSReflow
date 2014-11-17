/* strips.c: cut the entire image into strips.

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

#include "gf.h"

#include "input-img.h"
#include "input-pbm.h"
#include "main.h"
#include "strips.h"

extern boolean verbose;  /* Defined in main.c.  */



void
write_chars_as_strips (image_header_type image_header)
{
  dimensions_type char_dimens;
  gf_char_type gf_char;
  unsigned lines_per_char;
  one_byte *scanline;
  real width_in_points;
  boolean first_char = true;
  unsigned gf_row = 0;
  unsigned scanline_count = 0;

  /* Set up for the first character.  We divide the image into 256 parts,
     each of which will turn into one ``character''.  */
  lines_per_char = ROUND (image_header.height / 254.0);
  GF_CHARCODE (gf_char) = 0;
  DIMENSIONS_WIDTH (char_dimens) = image_header.width;
  DIMENSIONS_HEIGHT (char_dimens) = lines_per_char;
  GF_BITMAP (gf_char) = new_bitmap (char_dimens);
  GF_CHAR_MIN_COL (gf_char) = GF_CHAR_MIN_ROW (gf_char) = 0;
  GF_CHAR_MAX_COL (gf_char) = DIMENSIONS_WIDTH (char_dimens);
  GF_CHAR_MAX_ROW (gf_char) = DIMENSIONS_HEIGHT (char_dimens) - 1;
  
  /* We aren't going to have any side bearings.  */
  GF_H_ESCAPEMENT (gf_char) = DIMENSIONS_WIDTH (char_dimens);
  width_in_points = DIMENSIONS_WIDTH (char_dimens) * POINTS_PER_INCH
                    / (real) image_header.hres;
  GF_TFM_WIDTH (gf_char) = real_to_fix (width_in_points / design_size);
  
  
  /* Read the image.  */
  while (true)
    {
      if (scanline_count % lines_per_char == 0)
        {
          /* We get here when scanline_count == 0, and we haven't read
             anything, so we can't write anything.  */
          if (!first_char)
            {
              gf_put_char (gf_char);
              fprintf (stderr, "[%u]%c", GF_CHARCODE (gf_char),
                               (GF_CHARCODE (gf_char) + 1) % 13 ? ' ' : '\n');
              fflush (stderr);
              GF_CHARCODE (gf_char)++;
            }
          else
            first_char = false;
          gf_row = 0;
        }

      scanline = BITMAP_BITS (GF_BITMAP (gf_char))
                 + gf_row * DIMENSIONS_WIDTH (char_dimens); 
      if (!(*image_get_scanline) (scanline)) break;

      scanline_count++;
      gf_row++;
    }
  
  if (scanline_count != image_header.height)
    WARNING2 ("Expected %u scanlines, read %u", image_header.height,
              scanline_count); 
}
