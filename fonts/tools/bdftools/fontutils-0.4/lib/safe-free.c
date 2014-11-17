/* safe-free.c: free with error checking.

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


/* `free' itself does not return an error status. So all we can do here
   is check for nulls.  */

void
safe_free (address *item)
{
  if (item == NULL || *item == NULL)
    {
      fprintf (stderr, "safe_free: Attempt to free a null item.\n");
      abort ();
    }

  free (*item);

  *item = NULL;
}
