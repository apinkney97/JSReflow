/* oblique.c: definitions for slanting the characters.

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

#include "spline.h"

#include "oblique.h"

/* Angle from the vertical by which to slant the shapes, in radians.  */
real oblique_angle = 0.0;

static real_coordinate_type slant_point (real_coordinate_type);



/* Transform all the coordinates in each spline in the list.  */

spline_list_array_type
oblique_splines (spline_list_array_type shape)
{
  unsigned this_list;
  spline_list_array_type answer = new_spline_list_array ();

  if (oblique_angle == 0.0) return shape;
  
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      spline_list_type *new = new_spline_list ();

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          START_POINT (s) = slant_point (START_POINT (s));
          END_POINT (s) = slant_point (END_POINT (s));
          
          if (SPLINE_DEGREE (s) == CUBIC)
            {
              CONTROL1 (s) = slant_point (CONTROL1 (s));
              CONTROL2 (s) = slant_point (CONTROL2 (s));
            }

          append_spline (new, s);
        }
      
      append_spline_list (&answer, *new);
    }

  return answer;
}


/* Slant the single point P.  This is just trigonometry.  */

static real_coordinate_type
slant_point (real_coordinate_type p)
{
  real_coordinate_type slanted_p;
  
  slanted_p.y = p.y;
  slanted_p.x = p.x + tan (oblique_angle) * p.y;
  
  return slanted_p;
}
