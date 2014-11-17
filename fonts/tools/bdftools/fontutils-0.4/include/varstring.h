/* varstring.h: variable-length strings.

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

#ifndef VARSTRING_H
#define VARSTRING_H

#include "types.h"


/* Initialize a variable-length string.  */
typedef struct
{
  unsigned allocated;
  unsigned used;
  string data;
} variable_string;

/* The data characters.  */
#define VS_CHARS(vs) ((vs).data)

/* This is the number of bytes allocated for the string.  */
#define VS_ALLOCATED(vs) ((vs).allocated)

/* This is the number of bytes used.  */
#define VS_USED(vs) ((vs).used)


/* Create a new structure, initializing the data to a null byte.  */
extern variable_string vs_init (void);

/* Free the string.  */
extern void vs_free (variable_string *);


/* Put NEW_CHAR at position POS in S.  POS may be beyond the current
   length of S.  You are responsible for putting a null at the end of
   the string when you are done constructing it, if you want one.  */
extern void vs_set_char (variable_string *s, unsigned pos, char new_char);

/* Put NEW_CHAR at the end of V.  As with `vs_set_char', no null is
   appended.  */
extern void vs_append_char (variable_string *v, char new_char);

#endif /* not VARSTRING_H */
