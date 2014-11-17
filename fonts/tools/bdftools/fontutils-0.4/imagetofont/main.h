/* main.h: global variable declarations.

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

#ifndef MAIN_H
#define MAIN_H

#include "types.h"
#include "image-header.h"


/* See main.c for explanations of these globals.  */
extern real design_size;
extern boolean trace_scanlines, verbose;
extern image_header_type image_header;


extern void (*image_open_input_file)(string filename);
extern void (*image_close_input_file)(void);
extern void (*image_get_header)(void);
extern boolean (*image_get_scanline)(one_byte *p);

#endif /* not MAIN_H */
