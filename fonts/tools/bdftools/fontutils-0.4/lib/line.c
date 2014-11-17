/* line.c: read an arbitrary-length line from a file, returning the result
   as a string.

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

#include "line.h"
#include "varstring.h"


/* Return the next line of IN_FILE, or NULL if we are at EOF.  The
   string is allocated with malloc.  */

string
read_line (FILE *in_file)
{
  int c;
  variable_string answer = vs_init ();

  /* Have to check for EOF before appending the character.  */
  while ((c = getc (in_file)) != EOF && c != '\n')
    {
      vs_append_char (&answer, c);
    }
  
  /* Append the null (assuming here that null characters don't appear in
     the text files), if we found any characters.  */
  if (VS_USED (answer) > 0)
    {
      vs_append_char (&answer, 0);
      return VS_CHARS (answer);
    }
  else
    return NULL;
}
