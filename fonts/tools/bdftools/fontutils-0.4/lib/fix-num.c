/* fix-num.c: conversions on ``fixnums'', which are a 32-bit
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

#include "config.h"

#include "fix-num.h"


#define FIX_UNITY (1 << 20)


const real
fix_to_real (fix_word f)
{
  real r = f / FIX_UNITY + ((real) (f % FIX_UNITY) / (real) FIX_UNITY);

  return r;
}


const fix_word
real_to_fix (real r)
{
  fix_word f = floor (r) * FIX_UNITY + (r - floor (r)) * FIX_UNITY;

  return f;
}
