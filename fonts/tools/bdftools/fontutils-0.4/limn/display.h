/* display.h: declarations for online output.

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

#ifndef DISPLAY_H
#define DISPLAY_H

#include "bounding-box.h"
#include "font.h"
#include "spline.h"
#include "types.h"


/* See display.c.  */
extern boolean wants_display;
extern int display_grid_size;
extern int display_pixel_size;
extern int display_rectangle_size;
extern boolean display_stop;

/* Open the connection to the X server, etc.  */
extern void init_display (bitmap_font_type);

/* Set up to write the character C.  */
extern void x_start_char (char_info_type c);

/* Display the character C after it has been fitted to the splines S.  */
extern void x_output_char (char_info_type c, spline_list_array_type s);

/* Display a pixel at position P.  */
extern void display_pixel (real_coordinate_type p);

/* Display a knot in the splines.  */
extern void display_corner (coordinate_type);

/* Display a point at which we subdivided.  */
extern void display_subdivision (real_coordinate_type);

/* Finish up.  */
extern void close_display (void);

#endif /* not DISPLAY_H */
