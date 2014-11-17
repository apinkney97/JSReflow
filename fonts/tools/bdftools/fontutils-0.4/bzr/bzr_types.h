/* bzr_types.h: declare information that's the same for input and output.

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

#ifndef BZR_TYPES_H
#define BZR_TYPES_H

#include "types.h"


/* The character locators just have a back pointer.  */
typedef struct
{
  byte_count_type ptr;
} char_locator_type;

/* Where the BOC for this character is in the file (or NULL_BYTE_PTR).  */
#define CHAR_POINTER(l)  ((l).ptr)

#endif /* not BZR_TYPES_H */
