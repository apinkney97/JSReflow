/* numtoa.c: change numbers back to strings.

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

/* Declarations for these are in global.h.  */

string
itoa (int i)
{
  char a[MAX_INT_LENGTH];
  
  sprintf (a, "%d", i);
  return xstrdup (a);
}

string
utoa (unsigned u)
{
  char a[MAX_INT_LENGTH];
  
  sprintf (a, "%u", u);
  return xstrdup (a);
}

string
dtoa (double d)
{
  /* I'm not sure if this is really enough, but I also don't know how to
     compute what *would* be enough.  */
  char a[500];
  
  sprintf (a, "%f", d);
  return xstrdup (a);
}
