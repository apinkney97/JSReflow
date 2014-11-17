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
#include "xstat.h"

#include "img-input.h"


/* Where the input comes from.  */
static FILE *img_input_file;
static string img_input_filename;

/* Macros for convenient input.  The routines are defined in kbase.  */
#define IMG_MATCH_BYTE(v)  match_byte (v, img_input_file, img_input_filename)
#define IMG_GET_TWO()  get_two (img_input_file, img_input_filename)
#define IMG_GET_FOUR()  get_four (img_input_file, img_input_filename)



/* Only one file can be open at a time.  We do no path searching.  If
   FILENAME can't be opened, we quit.  */

FILE *
open_img_input_file (string filename)
{
  assert (img_input_file == NULL);
  
  img_input_file = xfopen (filename, "r");
  img_input_filename = filename;
  
  return img_input_file;
}


/* Close the input file.  If it hasn't been opened, we quit.  */

void
close_img_input_file ()
{
  assert (img_input_file != NULL);
  
  xfclose (img_input_file, img_input_filename);
  img_input_file = NULL;
}



/* Read the header information.  HEADER_SIZE is the total size in bytes
   before the data, not the size of the structure.  */

#define HEADER_SIZE 22

img_header_type
get_img_header ()
{
  img_header_type h;
  
  /* The ``magic number''.  */
  IMG_MATCH_BYTE (0211);
  IMG_MATCH_BYTE ('O');
  IMG_MATCH_BYTE ('P');
  IMG_MATCH_BYTE ('S');
  
  /* The version number.  */
  IMG_MATCH_BYTE (0);
  IMG_MATCH_BYTE (2);
  
  h.hres = IMG_GET_TWO ();
  h.vres = IMG_GET_TWO ();
  h.flags = IMG_GET_FOUR ();
  h.width = IMG_GET_TWO ();
  h.height = IMG_GET_TWO ();
  h.d = IMG_GET_TWO ();
  h.format = IMG_GET_TWO ();

  return h;
}



/* Read the data.  */

one_byte *
read_data ()
{
  one_byte *m;
  unsigned data_size;
  struct stat stats = xstat (img_input_filename);
  
  data_size = stats.st_size - HEADER_SIZE;
  
  m = xmalloc (data_size);
  if (fread (m, data_size, 1, img_input_file) != 1)
    FATAL1 ("read_data: fread of %u bytes failed", data_size);
  
  return m;
}
