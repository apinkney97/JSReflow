/* libfile.h: read auxiliary data files.

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

#ifndef LIBFILE_H
#define LIBFILE_H

#include "types.h"


/* Open (close) the file `HEAD.SUFFIX' using the library path.  Give a
   fatal error if the file cannot be (has not been) opened.  Only one
   file can be open at a time.  */
extern void libfile_start (string head, string suffix);
extern void libfile_close (void);


/* The name (line number) of the currently open library file, or NULL (0).  */
extern string libfilename (void);
extern unsigned libfile_linenumber (void);


/* An abbreviation for common error messages.  */
#define LIBFILE_ERROR1(fmt, e1) \
  FATAL3 ("%s:%u: " fmt, libfilename (), libfile_linenumber (), e1);


/* Return the next non-blank non-comment line in the currently open
   library file, or NULL if at EOF.  The terminating newline and any
   trailing comment are also removed.  */
extern string libfile_line (void);

#endif /* not LIBFILE_H */
