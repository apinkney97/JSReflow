/* img-output.h: declarations for writing an IMG file.

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

#ifndef IMG_OUTPUT_H
#define IMG_OUTPUT_H

#include "img.h"
#include "types.h"


/* At most one IMG file can be open for writing at a time.  */
extern boolean open_img_output_file (string filename);
extern void close_img_output_file ();

/* Write the header H.  */
extern void put_img_header (img_header_type h);

/* Write the single byte B.  */
extern void img_put_byte (one_byte b);

#endif /* not IMG_OUTPUT_H */
