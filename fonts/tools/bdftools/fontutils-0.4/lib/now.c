/* time.c: the current date and time.

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


/* Get the current date and time as a string.  The 26 here is defined in
   the manual page for ctime(3).  */

string
now ()
{
  extern time_t time (long *);
  extern string ctime (time_t *);

  string time_string = xmalloc (26);
  time_t t = time (0);

  strcpy (time_string, ctime (&t));
  time_string[24] = 0;		/* No newline. */

  return time_string;
}
