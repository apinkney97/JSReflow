/* input-img.h: declarations for reading an IMG file.

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

#ifndef INPUT_IMG_H
#define INPUT_IMG_H

#include "types.h"
#include "image-header.h"


/* Prepare to read FILENAME.  */
extern void img_open_input_file (string filename);

/* Close up gracefully.  */
extern void img_close_input_file (void);


/* Read the header.  */
extern void img_get_header (void);


/* Read a single scanline into P.  P must point to a large-enough block
   for one entire scanline (the width of the image is given in the
   header).  If at the end of the input file, returns false, else
   returns true.  */
extern boolean img_get_scanline (one_byte *p);

#endif /* not INPUT_IMG_H */
