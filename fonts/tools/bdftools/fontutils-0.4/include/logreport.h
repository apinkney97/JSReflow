/* logreport.h: status reporting routines.

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

#ifndef LOGREPORT_H
#define LOGREPORT_H

#include <stdio.h>
#include "types.h"


/* report.h is a separate file because the programs which use it are a
   proper subset of the programs which use us.  */
#include "report.h"

/* The file we write information to.  */
extern FILE *log_file;

/* Only write if this is true.  */
extern boolean logging;

#define LOG(s)								\
  do { if (logging) fprintf (log_file, s); } while (0)
#define LOG1(s, e)							\
  do { if (logging) fprintf (log_file, s, e); } while (0)
#define LOG2(s, e1, e2)							\
  do { if (logging) fprintf (log_file, s, e1, e2); } while (0)
#define LOG3(s, e1, e2, e3)						\
  do { if (logging) fprintf (log_file, s, e1, e2, e3); } while (0)
#define LOG4(s, e1, e2, e3, e4)						\
  do { if (logging) fprintf (log_file, s, e1, e2, e3, e4); } while (0)
#define LOG5(s, e1, e2, e3, e4, e5)					\
  do { if (logging) fprintf (log_file, s, e1, e2, e3, e4, e5); } while (0)

extern void flush_log_output (void);

#endif /* not LOGREPORT_H */
