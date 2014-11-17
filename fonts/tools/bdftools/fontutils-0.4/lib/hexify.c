/* hexify.c: change a binary string to ASCII hex characters.

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

#include "hexify.h"


/* Convert the number N (assumed to be small enough) to a lowercase hex
   character.  */
#define HEX_DIGIT(n) ((n) < 10 ? (n) + '0' : (n) - 10 + 'a')

string
hexify (variable_string v)
{
  unsigned i;
  /* Each byte in S turns into two hex chars, plus the terminating null.  */
  string h = xmalloc (VS_USED (v) * 2 + 1);
  
  for (i = 0; i < VS_USED (v); i++)
    {
      unsigned char n = ((unsigned char) VS_CHARS (v)[i] & 0xf0) >> 4;
      h[i * 2]     = HEX_DIGIT (n);
      n = (unsigned char) VS_CHARS (v)[i] & 0x0f;
      h[i * 2 + 1] = HEX_DIGIT (n);
    }
  h[i * 2] = 0;
  
  return h;
}


