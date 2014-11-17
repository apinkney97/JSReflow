/* oblique.h: declarations for slanting the characters.

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

#ifndef OBLIQUE_H
#define OBLIQUE_H

#include "types.h"
#include "spline.h"


/* See oblique.c.  */
extern real oblique_angle;

extern spline_list_array_type oblique_splines (spline_list_array_type);

#endif /* not OBLIQUE_H */