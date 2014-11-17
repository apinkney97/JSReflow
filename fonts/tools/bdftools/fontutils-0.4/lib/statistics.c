/* statistics.c: find various statistics.

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

#include "statistics.h"


/* The mean.  The array A might have been filtered (for example), in
   which case it will have many zeros in it that shouldn't be counted
   when computing the statistics.  PERTINENT is the number of elements
   that are actually valid.  COUNT, on the other hand, is the number of
   elements in the array.  */

real
mean (real *a, unsigned count, unsigned pertinent)
{
  unsigned this_variable;
  real sum = 0.0;

  assert (pertinent > 0);

  for (this_variable = 0; this_variable < count; this_variable++)
    sum += a[this_variable];

  return sum / pertinent;
}


/* The standard deviation.  We have to subtract one from the number of
   samples, because we lost a degree of freedom because of the mean.  */

real
standard_deviation (real *a, real mean, unsigned count, unsigned pertinent)
{
  real sum = 0.0;
  int this_variable;

  assert (pertinent > 0);

  for (this_variable = 0; this_variable < count; this_variable++)
    {
      real x = a[this_variable] - mean;
      sum += x * x;
    }

  return sqrt (sum / (pertinent > 1 ? (pertinent - 1) : 1));
}


/* Here's a routine to fill up the statistics structure.  */

statistics_type
statistics (real *data, unsigned data_length, unsigned pertinent)
{
  statistics_type s;

  s.mean = mean (data, data_length, pertinent);

  s.variance = standard_deviation (data, s.mean, data_length, pertinent);
  s.variance *= s.variance;

  return s;
}
