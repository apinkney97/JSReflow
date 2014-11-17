/* fix-num.h: declarations for ``fix'' numbers, which are a 32-bit
   word with 20 bits of fraction.

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

#ifndef FIX_NUM_H
#define FIX_NUM_H

#include "types.h"


/* The type.  */
typedef long fix_word;

/* Conversions to and from C floating-point numbers.  The conversions
   may not be exact, but since we use this to represent physical
   quantities, a tiny loss of accuracy does not matter.  */
extern const real fix_to_real (fix_word);
extern const fix_word real_to_fix (real);

#endif /* not FIX_NUM_H */
