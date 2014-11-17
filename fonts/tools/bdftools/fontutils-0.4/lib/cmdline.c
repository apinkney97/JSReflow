/* cmdline.c: routines to help in parsing command lines.

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

#include "cmdline.h"


/* This routine takes a string L consisting of unsigned integers
   separated by commas and returns a vector of the integers, as numbers.
   A element is appended to the vector.  */

int *
scan_unsigned_list (string l)
{
  string map;
  unsigned length = 1;
  int *vector = xmalloc (sizeof (int));
  
  for (map = strtok (l, ","); map != NULL; map = strtok (NULL, ","))
    {
      length++;
      vector = xrealloc (vector, length * sizeof (int));
      vector[length - 2] = atou (map);
      if (vector[length - 2] < 0)
        WARNING1 ("Unsigned number %u is too large", vector[length - 2]);
    }
  
  vector[length - 1] = -1;
  return vector;
}


/* Return the <number> substring in `<name>.<number><stuff>', if S has
   that form.  If it doesn't, return NULL.  */

string
find_dpi (string s)
{
  unsigned dpi_number;
  string extension = strrchr (s, '.');
  
  if (extension != NULL)
    if (sscanf (extension + 1, "%u", &dpi_number) == 1)
      return utoa (dpi_number);

  return NULL;
}
