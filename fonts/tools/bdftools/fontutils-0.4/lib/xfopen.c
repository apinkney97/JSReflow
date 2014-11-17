/* xfopen.c: fopen and fclose with error checking.

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


/* These routines just check the return status from standard library
   routines and abort if an error happens.  */

FILE *
xfopen (string filename, string mode)
{
  FILE *f = fopen (filename, mode);

  if (f == NULL)
    FATAL_PERROR (filename);

  return f;
}


void
xfclose (FILE *f, string filename)
{
  if (fclose (f) == EOF)
    FATAL_PERROR (filename);
}

