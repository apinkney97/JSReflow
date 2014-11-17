/* substring.c: copy a chunk from a string.

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


/* Return a fresh copy of SOURCE[START..LIMIT], or NULL if LIMIT<START.
   If LIMIT>strlen(START), it is reassigned. */

string
substring (string source, const unsigned start, const unsigned limit)
{
  string result;
  unsigned this_char;
  unsigned length = strlen (source);
  unsigned lim = limit;

  /* Upper bound out of range? */
  if (lim >= length)
    lim = length - 1;

  /* Null substring? */
  if (start > limit)
    return "";

  /* The `2' here is one for the null and one for limit - start inclusive. */
  result = xmalloc (limit - start + 2);

  for (this_char = start; this_char <= limit; this_char++)
    result[this_char - start] = source[this_char];

  result[this_char - start] = 0;

  return result;
}
