/* output-epsf.c: output EPS files.

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
#include "output-epsf.h"


/* The base part of the fontname to output to.  */
static string output_name;


/* Remember OUTPUT_NAME, so we can use it to construct the output files.
   It's a fatal error if NAME has a suffix, since that could mean
   overwriting the same file several times with different information.  */

void
epsf_start_output (string name)
{
  if (find_suffix (name) != NULL)
    FATAL ("You can't specify all the EPS files' suffices to be the same");

  output_name = name;
}


/* Write a single character to the file `output_name-CODE.eps',
   where CODE is the character code.  */

/* Output the lower eight bits of VALUE to FILE as two hex digits.  For
   reasons I don't understand, when I write these same expressions in
   the arguments to the fprintf, values less than 128 are copied into
   the high nybble; that is, 1 is output as `11'.  */
#define OUTPUT_HEX(file, value)						\
  do									\
    {  one_byte v1 = (value) & 0xf0;					\
       one_byte v2 = (value) & 0xf;					\
       fprintf (file, "%x%x", v1 >> 4, v2);				\
    }									\
  while (0)

void
epsf_output_char (char_info_type c)
{
  unsigned row;
  bitmap_type b = CHAR_BITMAP (c);
  unsigned height = BITMAP_HEIGHT (b);
  unsigned width = BITMAP_WIDTH (b);
  charcode_type code = CHARCODE (c);
  string title = concat3 (output_name, "-", itoa (code));
  string eps_name = concat (title, ".eps");
  FILE *eps = xfopen (eps_name, "w");
  
  /* Write the header.  */
  fputs ("%!PS-Adobe-3.0 EPSF-3.0\n", eps);
  fprintf (eps, "%%%%BoundingBox: 0 0 %u %u\n",
           BITMAP_WIDTH (b), BITMAP_HEIGHT (b));
  fprintf (eps, "%%%%Title: (%s)\n", title);
  fputs ("%%Creator: fontconvert -epsf\n", eps);
  fprintf (eps, "%%%%CreationDate: %s\n", now ());
  fputs ("%%EndComments\n", eps);
  
  /* Write the bitmap as an image.  */
  
  /* First arrange to scale the image to its original size.  */
  fprintf (eps, "%u %u scale\n", width, height);
  
  /* Push the imagemask parameters.  */
  fprintf (eps, "%u %u  true  [%u 0 0 -%u 0 %u]\n",
           width, height, width, height, height);

  /* Output the bits.  */
  fputs ("{<", eps);
  for (row = 0; row < height; row++)
    {
      unsigned col;
      one_byte byte = 0;
      unsigned bit = 7;

      for (col = 0; col < width; col++)
        {
          byte |= BITMAP_PIXEL (b, row, col) << bit;
          
          /* Output eight bits at a time, since imagemask expects that.  */
          if (bit == 0)
            {
              OUTPUT_HEX (eps, byte);
              byte = 0;
              bit = 8;
            }
          
          /* Decrement `bit' after, so that the test `bit == 0' above is
             true at the right time.  */
          bit--;
        }
      
      /* If the width isn't a multiple of eight, have to output the
         remainder.  */
      if (bit < 7)
        OUTPUT_HEX (eps, byte);
      
      /* In any case, output a newline, just to make the output easier
         to read, in case somebody wants to read it.  */
      fputs ("\n", eps);
    }
  fputs (">}\nimagemask\n", eps);
  
  /* Write the trailer.  */
  fputs ("showpage\n", eps);
  fputs ("%%EOF\n", eps);
  
  xfclose (eps, eps_name);
  free (eps_name);
  free (title);
}


/* We don't have anything to do at the end.  */

void
epsf_finish_output (void)
{
}
