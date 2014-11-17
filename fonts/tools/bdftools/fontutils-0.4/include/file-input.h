/* file-input.h: declarations for file reading.

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

#ifndef FILE_INPUT_H
#define FILE_INPUT_H

#include <stdio.h>
#include "types.h"


/* Read some number of bytes from the file F.  The FILENAME argument
   is passed to perror(3), if the read fails, and then the program is
   halted.  The routines that get more than one byte assume the value is
   stored in the file in BigEndian order, regardless of the host
   architecture.  */

extern one_byte get_byte (FILE *f, string filename);
extern two_bytes get_two (FILE *, string);
extern four_bytes get_four (FILE *, string);
extern signed_4_bytes get_signed_four (FILE *, string);
extern address get_n_bytes (unsigned n, FILE *, string);

/* Back up some number of bytes, then read.  */
extern one_byte get_previous_byte (FILE *, string);
extern two_bytes get_previous_two (FILE *, string);
extern four_bytes get_previous_four (FILE *, string);

/* Abort if the next or previous byte is not EXPECTED.  */
extern void match_byte (one_byte expected, FILE *, string);
extern void match_previous_byte (one_byte expected, FILE *, string);

/* Abort if string of bytes is not STRING.  */
extern void match_string_of_bytes (string, FILE *, string);

#endif /* not FILE_INPUT_H */
