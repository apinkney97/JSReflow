/* file-input.c: file reading routines for binary files in BigEndian
   order, 2's complement representation.

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


one_byte
get_byte (FILE *f, string filename)
{
  one_byte b;

  if (fread (&b, 1, 1, f) != 1)
    FATAL_PERROR (concat3 ("get_byte (", filename, ")"));

  return b;
}


two_bytes
get_two (FILE *f, string filename)
{
  two_bytes b;

  b = get_byte (f, filename) << 8;
  b |= get_byte (f, filename);

  return b;
}


four_bytes
get_four (FILE *f, string filename)
{
  four_bytes b;

  b = get_byte (f, filename) << 24;
  b |= get_byte (f, filename) << 16;
  b |= get_byte (f, filename) << 8;
  b |= get_byte (f, filename);

  return b;
}


/* In order to get a signed value, we merely read an unsigned value and
   cast it; the value in the file is assumed to be 2's complement.  */

signed_4_bytes
get_signed_four (FILE *f, string filename)
{
  return (signed_4_bytes) get_four (f, filename);
}


/* Return the next N bytes in the file F as an array.  */

address
get_n_bytes (unsigned n, FILE *f, string filename)
{
  one_byte *b;

  if (n == 0)
    FATAL1 ("get_n_bytes (%s): can't get zero bytes", filename);

  b = xmalloc (n);

  if (fread (b, n, 1, f) != 1)
    FATAL2 ("get_n_bytes (%s): fread of %u bytes failed", n, filename);

  return b;
}



/* Reading backwards. This macro is shared among all the routines by assuming
   the name `f' for the file pointer.  */

#define MOVE_BACK(size) xfseek (f, (long) -size, SEEK_CUR, filename)

one_byte
get_previous_byte (FILE *f, string filename)
{
  one_byte b;

  MOVE_BACK (1);
  b = get_byte (f, filename);
  MOVE_BACK (1);
  return b;
}


two_bytes
get_previous_two (FILE *f, string filename)
{
  two_bytes b;

  MOVE_BACK (2);
  b = get_two (f, filename);
  MOVE_BACK (2);

  return b;
}



four_bytes
get_previous_four (FILE *f, string filename)
{
  four_bytes b;

  MOVE_BACK (4);
  b = get_four (f, filename);
  MOVE_BACK (4);

  return b;
}



/* Looking for specific values in the input.  */

void 
match_byte (one_byte expected, FILE *f, string filename)
{
  one_byte b = get_byte (f, filename);

  if (b != expected)
    FATAL3 ("%s: Expected byte value %u, found %u", filename, expected, b); 
}


void
match_previous_byte (one_byte expected, FILE *f, string filename)
{
  one_byte b = get_previous_byte (f, filename);

  if (b != expected)
    FATAL3 ("%s: Expected previous byte value %u, found %u", filename, 
            expected, b);
}
