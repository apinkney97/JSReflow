/* input-img.c: read Interleaf .img files.

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

#include "file-input.h"

#include "main.h"
#include "image-header.h"
#include "input-img.h"


/* Where the input comes from.  */
static FILE *img_input_file;
static string img_input_filename;

/* What we'll read the raw data for each scanline into.  */
static one_byte *line_in_bytes;

/* How big `line_in_bytes' is, in both bytes and bits.  */
static unsigned byte_width;
static unsigned bit_width;

/* Macros for convenient input.  The routines are defined in kbase.  */
#define IMG_MATCH_BYTE(v)  match_byte (v, img_input_file, img_input_filename)

#define IMG_GET_TWO()  get_two (img_input_file, img_input_filename)
#define IMG_GET_FOUR()  get_four (img_input_file, img_input_filename)



/* Only one file can be open at a time.  We do no path searching.  If
   FILENAME can't be opened, we quit.  */

void
img_open_input_file (string filename)
{
  assert (img_input_file == NULL);
  
  img_input_file = xfopen (filename, "r");
  img_input_filename = filename;
}


/* Close the input file.  If it hasn't been opened, we quit.  */

void
img_close_input_file ()
{
  assert (img_input_file != NULL);
  
  xfclose (img_input_file, img_input_filename);
  img_input_file = NULL;
}



/* Read the header information.  
   Modifies the global image_header in main.c.  */

void
img_get_header ()
{
  unsigned short_width;
  unsigned flags;
  
  /* The ``magic number''.  */
  IMG_MATCH_BYTE (0211);
  IMG_MATCH_BYTE ('O');
  IMG_MATCH_BYTE ('P');
  IMG_MATCH_BYTE ('S');
  
  /* The version number.  */
  IMG_MATCH_BYTE (0);
  IMG_MATCH_BYTE (2);
  
  image_header.hres = IMG_GET_TWO ();
  image_header.vres = IMG_GET_TWO ();
  flags = IMG_GET_FOUR ();
  if (flags != 0)
    FATAL1 ("img_get_header: Expected flags to be zero, not %u", flags);
  image_header.width = IMG_GET_TWO ();
  image_header.height = IMG_GET_TWO ();
  image_header.depth = IMG_GET_TWO ();
  if (image_header.depth != 1)
    FATAL1 ("img_get_header: I can't handle %u-bit deep images", 
	    image_header.depth);
  image_header.format = IMG_GET_TWO ();
  if (image_header.format != 0)
    FATAL1 ("img_get_header: Expected format to be zero, not %u", 
	    image_header.format);
  
  /* Allocate the space that we will read into (since that space is the
     same from scanline to scanline).  The width is derived from the
     header information, which must already have been read.
     Specifically, the width in the header is given in bits.  Each
     scanline is padded to a 16-bit boundary.  For example, suppose the
     header width = 1516, that is 94.75 shorts; we round up to 95,
     thus getting a scanline width of 1520 bits, or 190 bytes.  */

  bit_width = image_header.width;
  short_width = bit_width / 16 + (bit_width % 16 != 0);
  byte_width = short_width * 2;
  line_in_bytes = xmalloc (byte_width);
}



/* Read one scanline of the image, returning each bit of the scanline in
   a separate byte.  We use the `line_in_bytes' variable, allocated in
   `img_get_header', to read into.  */

boolean
img_get_scanline (one_byte *line_in_bits)
{
  unsigned this_bit, this_byte, mask;

  if (fread (line_in_bytes, byte_width, 1, img_input_file) != 1)
    {
      if (feof (img_input_file))
        return false;
      else
        FATAL1 ("img_get_scanline: fread of %u bytes failed", byte_width);
    }
  
  for (this_bit = 0, this_byte = 0, mask = 0x80;
       this_bit < bit_width; this_bit++)
    {
      /* Somewhere along the line, black and white got reversed.  */
      line_in_bits[this_bit] = (line_in_bytes[this_byte] & mask) == 0;
      if (this_bit % 8 == 0)
        {
          this_byte = this_bit / 8;
          mask = 0x80;
        }
      else
        mask >>= 1;
    }
  
  return true;
}

