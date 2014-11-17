/* input-pbm.c: read PBM files.

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

#include "bitmap.h"
#include "file-input.h"
#include "pbm.h"

/* From main.c.  */
extern boolean trace_scanlines;


/* Where the input comes from.  */
static FILE *pbm_input_file;
static string pbm_input_filename;




/* Only one file can be open at a time.  We do no path searching.  If
   FILENAME can't be opened, we quit.  */

void
pbm_open_input_file (string filename)
{
  assert (pbm_input_file == NULL);
  
  pbm_input_file = xfopen (filename, "r");
  pbm_input_filename = filename;
}


/* Close the input file.  If it hasn't been opened, we quit.  */

void
pbm_close_input_file ()
{
  assert (pbm_input_file != NULL);
  
  xfclose (pbm_input_file, pbm_input_filename);
  pbm_input_file = NULL;
}



/* Read a block HEIGHT scanlines high from the image.  The last few
   scanlines must all be blank (we don't bother to return them).  If
   they aren't, then a rounding error has caused Ghostscript to give us
   fewer lines than expected.  Just return a short block, and save that
   line for next time.  */

#define BLANK_COUNT 4

bitmap_type *
pbm_get_block (unsigned height)
{
  static int image_format;
  static int image_height;
  static int image_width = -1;
  static unsigned scanline_count = 0;
  static one_byte *saved_scanline = NULL;
  dimensions_type d;
  unsigned row;
  boolean row_has_black;
  bitmap_type *b = XTALLOC (1, bitmap_type);
  boolean found_black = false;
  int c = getc (pbm_input_file);
  
  if (c == EOF)
    return NULL;
  ungetc (c, pbm_input_file);
  
  if (image_width == -1)
    pbm_readpbminit (pbm_input_file, &image_width, &image_height,
                     &image_format);

  DIMENSIONS_WIDTH (d) = image_width;
  DIMENSIONS_HEIGHT (d) = height;
  *b = new_bitmap (d);
  
  for (row = 0; row < height; row += found_black)
    {
      if (scanline_count == image_height)
        FATAL2 ("%s: Tried to read image row %d", pbm_input_filename,
                scanline_count);

      if (saved_scanline)
        {
          memcpy (BITMAP_ROW (*b, row), saved_scanline, image_width);
          saved_scanline = NULL;
        }
      else
        {
          pbm_readpbmrow (pbm_input_file, BITMAP_ROW (*b, row),
                          image_width, image_format);
          scanline_count++;

          if (trace_scanlines)
            {
              printf ("%5d:", scanline_count);
              for (c = 0; c < image_width; c++)
                putchar (BITMAP_ROW (*b, row)[c] ? '*' : ' ');
              putchar ('\n');
            }
        }
      
      /* Ignore this row if it was all-white and we haven't seen any
         black ones yet.  */
      row_has_black
        = memchr (BITMAP_ROW (*b, row), BLACK, image_width) != NULL;
      if (!found_black && row_has_black)
        { /* Our first non-blank row.  */
          found_black = true;
        }

      if (row >= height - BLANK_COUNT && row_has_black)
        { /* We've hit a nonblank row where we shouldn't have.  Save
             this row and return.  */
          saved_scanline = BITMAP_ROW (*b, row);
          break;
        }
    }

  BITMAP_HEIGHT (*b) = height - BLANK_COUNT;

  return b;
}
