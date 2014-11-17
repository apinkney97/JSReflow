/* scaled-num.c: conversions on ``scaled'' numbers, which are a 32-bit
   word with 16 bits of fraction.

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

#include "scaled-num.h"


#define SCALED_UNITY (1 << 16)	/* 2^16, represents 1.00000.  */


/* This prints a scaled number, rounded to five digits.  */

void
print_scaled (scaled s)
{
  scaled delta;

  if (s < 0)
    {				/* Print sign, if negative. */
      putchar ('-');
      s = -s;
    }

  printf ("%u", s / SCALED_UNITY);   /* Print integer part. */
  putchar ('.');

  s = 10 * (s % SCALED_UNITY) + 5;
  delta = 10;

  do
    {
      if (delta > SCALED_UNITY)
        s += 0100000 - (delta / 2);	/* Round the last digit. */

      printf ("%c", '0' + (s / SCALED_UNITY));
      s = 10 * (s % SCALED_UNITY);
      delta *= 10;
    }
  while (s > delta);
}


const real
scaled_to_real (scaled s)
{
  real r = s / SCALED_UNITY +  (s % SCALED_UNITY) / (real) SCALED_UNITY;

  return r;
}


const scaled
real_to_scaled (real r)
{
  scaled s = floor (r) * SCALED_UNITY + (r - floor (r)) * SCALED_UNITY;

  return s;
}
