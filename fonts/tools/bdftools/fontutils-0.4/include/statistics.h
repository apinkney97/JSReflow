/* statistics.h: find the first and second moments.

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

#ifndef STATISTICS_H
#define STATISTICS_H

#include "types.h"


typedef struct
{
  real mean;
  real variance;
} statistics_type;

/* Compute the statistics on A.  */
extern statistics_type statistics (real *a, unsigned length,
                                   unsigned pertinent);

/* Return the mean of the array A, assumed to have length LENGTH -- but
   only PERTINENT of the entries really count.  */
extern real mean (real *a, unsigned length, unsigned pertinent);

/* Similarly, for the standard deviation.  */
extern real standard_deviation (real *, real mean, unsigned length,
                                unsigned pertinent);

#endif /* not STATISTICS_H */
