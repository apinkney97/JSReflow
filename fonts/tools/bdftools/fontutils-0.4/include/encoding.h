/* encoding.h: parse a font encoding (.enc) file.

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

#ifndef ENCODING_H
#define ENCODING_H

#include "font.h"
#include "list.h"
#include "types.h"


/* A single character from the encoding file.  Since we usually want to
   deal with the entire collection of characters as a group, we don't
   define any accessor macros for this structure, but rather the next.  */
typedef struct
{
  string name;
  list_type ligature;
} encoding_char_type;

/* The size of font's encoding vector (PostScript defines this).  */
#define ENCODING_VECTOR_SIZE  256

/* The collection of all the information from the file.  */
typedef struct
{
  string coding_scheme;
  encoding_char_type encoding_char[ENCODING_VECTOR_SIZE];
} encoding_info_type;

/* The name of the encoding scheme in E_I.  */
#define ENCODING_SCHEME_NAME(e_i) ((e_i).coding_scheme)

/* The Nth encoding character in E_I.  */
#define ENCODING_CHAR_ELT(e_i, n) ((e_i).encoding_char[n])

/* The name of the character CODE in the encoding structure E_I, or NULL
   if the character doesn't exist.  */
#define ENCODING_CHAR_NAME(e_i, code) (ENCODING_CHAR_ELT (e_i, code).name)

/* The ligature table for the character code in the encoding structure
   E_I.  Each element of the list is a `tfm_ligature_type'.  The list is
   garbage if the character doesn't exist.  */
#define ENCODING_CHAR_LIG(e_i, code) (ENCODING_CHAR_ELT (e_i, code).ligature)


/* Returns the number for the character named NAME in E_I, or -1 if NAME
   is not present.  */
extern int encoding_number (encoding_info_type e_i, string name);


/* Read the library file (see `libfile.h') `FILENAME.enc', and return
   the information it contains.  If the file cannot be opened, give a
   fatal error.  */
extern encoding_info_type read_encoding_file (string filename);

#endif /* not ENCODING_H */

