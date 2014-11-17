/* img-output.c: write an IMG file.

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

#include "img-output.h"


/* The file we are writing to.  */
static FILE *img_output_file = NULL;
static string img_output_filename;

/* Macros for convenient output.  */
#define IMG_PUT_TWO(t) put_two (t, img_output_file, img_output_filename)
#define IMG_PUT_FOUR(f) put_four (f, img_output_file, img_output_filename)


/* Routines to start and end writing a file.  (For the user to call.)
   We make sure the caller can't have two output files open
   simultaneously. */

boolean
open_img_output_file (string filename)
{
  assert (img_output_file == NULL);

  img_output_filename = filename;
  img_output_file = fopen (filename, "w");

  return img_output_file != NULL;
}


void
close_img_output_file ()
{
  assert (img_output_file != NULL);

  xfclose (img_output_file, img_output_filename);

  img_output_filename = NULL;
  img_output_file = NULL;
}



/* Write the header to an IMG file.  */

void
put_img_header (img_header_type h)
{
  /* First the magic number.  */
  putc (0211, img_output_file);
  putc ('O', img_output_file);
  putc ('P', img_output_file);
  putc ('S', img_output_file);
  putc (0, img_output_file);
  putc (2, img_output_file);
  
  IMG_PUT_TWO (h.hres);
  IMG_PUT_TWO (h.vres);
  IMG_PUT_FOUR (0); /* The flags.  */
  IMG_PUT_TWO (h.width);
  IMG_PUT_TWO (h.height);
  IMG_PUT_TWO (1);  /* The depth.  */
  IMG_PUT_TWO (0);  /* The format.  */
}



/* Writing routines used in other files..  */

void
img_put_byte (one_byte b)
{
  putc (b, img_output_file);
}
