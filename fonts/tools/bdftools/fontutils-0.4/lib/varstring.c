/* varstring.c: variable-length strings.

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

#include "varstring.h"


/* You use variable-length stringsd by calling `init_string' first, then
   assigning to particular elements via `set_string_element'. The string
   will initially be `INITIAL_SIZE' characters, and be incremented in
   blocks of size `INCREMENT_SIZE'.  */

#define INITIAL_SIZE 16
#define INCREMENT_SIZE 64

variable_string
vs_init ()
{
  variable_string vs;

  VS_CHARS (vs) = xmalloc (INITIAL_SIZE);
  *VS_CHARS (vs) = 0;
  VS_ALLOCATED (vs) = INITIAL_SIZE;
  VS_USED (vs) = 0;

  return vs;
}


/* Free the data we've allocated for VS.  */

void
vs_free (variable_string *vs)
{
  free (VS_CHARS (*vs));
  VS_CHARS (*vs) = NULL;
}


/* We do not put a NULL after we insert NEW_CHAR at the (zero-based)
   index LOC.  The caller is responsible for that.  */

void
vs_set_char (variable_string *vs, unsigned loc, char new_char)
{
  /* Do we need more space?  */
  if (loc >= VS_ALLOCATED (*vs))
    { /* Yes.  Make sure to allocate enough.  */
      unsigned extra = MAX (INCREMENT_SIZE, loc - VS_ALLOCATED (*vs) + 1);
      VS_CHARS (*vs) = xrealloc (VS_CHARS (*vs), VS_ALLOCATED (*vs) + extra);
      VS_ALLOCATED (*vs) += extra;
    }

  VS_CHARS (*vs)[loc] = new_char;
  VS_USED (*vs) = loc + 1;
}


/* Append NEW_CHAR to VS.  */

void
vs_append_char (variable_string *vs, char new_char)
{
  vs_set_char (vs, VS_USED (*vs), new_char);
}
