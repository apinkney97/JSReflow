/* file-output.h: declarations for file reading.

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

#ifndef FILE_OUTPUT_H
#define FILE_OUTPUT_H

#include <stdio.h>
#include "types.h"


/* Write some number of bytes to the file F.  The FILENAME argument
   is passed to perror(3), if the write fails, and then the program is
   halted.  */
extern void put_byte (one_byte, FILE *f, string filename);
extern void put_two (two_bytes, FILE *, string);
extern void put_three (four_bytes, FILE *, string);
extern void put_four (four_bytes, FILE *, string);
extern void put_n_bytes (unsigned n, address, FILE *, string);

/* We assume the number is already in two's complement, and so we merely 
   have to output the bits.  */
#define put_signed_byte put_byte
#define put_signed_two put_two
#define put_signed_three put_three
#define put_signed_four put_four

#endif /* not FILE_OUTPUT_H */
