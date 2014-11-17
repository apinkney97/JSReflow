/* text.c: translate the binary BZR font to human-oriented text.

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

#include <ctype.h>
#include "text.h"
#include "bzr.h"


/* Print the preamble.  */

void
text_start_output (string font_name, bzr_preamble_type p)
{
  printf ("Font file: %s.\n", font_name);
  printf ("{%s}\n", BZR_COMMENT (p));
  printf ("Design size: %fpt.\n", BZR_DESIGN_SIZE (p));
}



/* Print one character in the file.  The iteration through the spline
   list isn't quite typical here, since we don't need to find the places
   where a new outline starts.  */

void
text_output_char (bzr_char_type c)
{
  unsigned this_list;
  spline_list_array_type shape = BZR_SHAPE (c);
  
  printf ("Character: 0x%x=%u", CHARCODE (c), CHARCODE (c));
  if (isprint (CHARCODE (c)))
    printf (" (%c)", CHARCODE (c));
  puts (":");

  printf ("  set width: %.3f.\n", CHAR_SET_WIDTH (c));
  printf ("  min/max col: %.3f/%.3f; min/max row: %.3f/%.3f.\n",
          BZR_CHAR_MIN_COL (c), BZR_CHAR_MAX_COL (c),
          BZR_CHAR_MIN_ROW (c), BZR_CHAR_MAX_ROW (c));

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      
      printf ("  Outline #%u:\n", this_list);
      
      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == LINEAR)
            printf ("    (%.3f,%.3f)--(%.3f,%.3f)",
                    START_POINT (s).x, START_POINT (s).y,
                    END_POINT (s).x, END_POINT (s).y);

          else
            printf ("    (%.3f,%.3f)..ctrls(%.3f,%.3f)&(%.3f,%.3f)..(%.3f,%.3f)",
                    START_POINT (s).x, START_POINT (s).y,
                    CONTROL1 (s).x, CONTROL1 (s).y,
                    CONTROL2 (s).x, CONTROL2 (s).y,
                    END_POINT (s).x, END_POINT(s).y);

          puts ("");
        }
    }

  puts ("");
}



/* Print the parts of the postamble (as the library returns it -- some
   more information is in the file, but not returned, because it is only
   interesting for programs that read the file).  */

void
text_finish_output (bzr_postamble_type p)
{
  printf ("Font bounding box: min/max col = %.3f/%.3f, \
min/max row = %.3f/%.3f.\n",
          MIN_COL (BZR_FONT_BB (p)), MAX_COL (BZR_FONT_BB (p)),
          MIN_ROW (BZR_FONT_BB (p)), MAX_ROW (BZR_FONT_BB (p)));
  printf ("Total number of characters: %u.\n", BZR_NCHARS (p));
}
