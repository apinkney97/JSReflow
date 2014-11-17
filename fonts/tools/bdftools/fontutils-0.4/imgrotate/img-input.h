/* img-input.h: declarations for reading an IMG file.

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

#ifndef IMG_INPUT_H
#define IMG_INPUT_H

#include "img.h"
#include "types.h"


/* Prepare to read FILENAME.  */
extern FILE *open_img_input_file (string filename);

/* Close up gracefully.  */
extern void close_img_input_file (void);

/* Read the header.  */
extern img_header_type get_img_header (void);

/* Read all the data (packed eight bits per byte) into memory.  */
extern one_byte *read_data (void);

#endif /* not IMG_INPUT_H */
