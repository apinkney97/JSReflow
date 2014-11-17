/* file-output.c: file writing routines for binary files in BigEndian
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

#include "file-output.h"


/* Output in BigEndian order.  We could do these more efficiently by
   checking if the hardware already uses BigEndian order, but it's not
   worth it.  These routines are never the bottleneck (in the ways
   they've been used so far, anyway).  We assume that we write negative
   numbers in 2's complement, also.  */

void
put_byte (one_byte b, FILE *f, string filename)
{
  if (fwrite (&b, 1, 1, f) != 1)
    FATAL2 ("%s: Could not write byte %u", filename, b);
}


void
put_two (two_bytes b, FILE *f, string filename)
{
  put_byte ((b & 0xff00) >> 8, f, filename); /* High-order byte.  */
  put_byte (b & 0x00ff, f, filename);        /* And low-order.   */
}


void
put_three (four_bytes b, FILE *f, string filename)
{
  put_byte ((b & 0x00ff0000) >> 16, f, filename);
  put_byte ((b & 0x0000ff00) >> 8, f, filename);
  put_byte (b & 0x000000ff, f, filename);
}


void
put_four (four_bytes b, FILE *f, string filename)
{
  put_byte ((b & 0xff000000) >> 24, f, filename);
  put_byte ((b & 0x00ff0000) >> 16, f, filename);
  put_byte ((b & 0x0000ff00) >> 8, f, filename);
  put_byte (b & 0x000000ff, f, filename);
}


void
put_n_bytes (unsigned n, address b, FILE *f, string filename)
{
  one_byte *data_b = b;  /* We can't dereference a pure `address'.  */

  if (fwrite (data_b, n, 1, f) != 1)
    FATAL2 ("%s: Could not write %u-byte block", filename, n);
}
