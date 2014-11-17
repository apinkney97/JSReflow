/* report.h: status reporting routines.

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

#ifndef REPORT_H
#define REPORT_H

#include <stdio.h>
#include "types.h"


/* Only print anything if this is true.  */
extern boolean verbose;


#define REPORT(s)							\
  do if (verbose) { fprintf (stderr, s); fflush (stderr); }		\
  while (0)

#define REPORT1(s, e)							\
  do if (verbose) { fprintf (stderr, s, e); fflush (stderr); }		\
  while (0)

#define REPORT2(s, e1, e2)						\
  do if (verbose) { fprintf (stderr, s, e1, e2); fflush (stderr); }	\
  while (0)

#define REPORT3(s, e1, e2, e3)						\
  do if (verbose) { fprintf (stderr, s, e1, e2, e3); fflush (stderr); }	\
  while (0)

#define REPORT4(s, e1, e2, e3, e4)					\
  do if (verbose) { fprintf (stderr, s, e1, e2, e3, e4);		\
		    fflush (stderr); } 					\
  while (0)

#endif /* not REPORT_H */
