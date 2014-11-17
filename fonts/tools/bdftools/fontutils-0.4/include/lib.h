/* lib.h: declarations for shared routines.

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

#ifndef LIB_H
#define LIB_H

#include "types.h"

/* Return a fresh copy of S1 followed by S2, et al.  */
extern string concat P2H(string s1, string s2);
extern string concat3 P3H(string, string, string);
extern string concat4 P4H(string, string, string, string);
extern string concat5 P5H(string, string, string, string, string);


/* A fresh copy of just S.  */
extern string xstrdup P1H(string s);


/* True if FILENAME1 and FILENAME2 are the same file.  If stat fails on
   either name, returns false, with no error message.  */
extern boolean same_file_p P2H(string filename1, string filename2);


/* Memory allocation functions. */

/* These call the corresponding function in the standard library, and
   abort if those routines fail.  Also, `xrealloc' calls `xmalloc' if
   OLD_ADDRESS is null.  */
extern address xmalloc P1H(unsigned size);
extern address xrealloc P2H(address old_address, unsigned new_size);
extern address xcalloc P2H(unsigned nelem, unsigned elsize);

/* (Re)Allocate N items of type T using malloc, or fail.  */
#define XTALLOC(n, t) (t *) xmalloc ((n) * sizeof (t))
#define XRETALLOC(addr, n, t) ((addr) = (t *) xrealloc (addr, (n) * sizeof(t)))

#endif /* not LIB_H */
