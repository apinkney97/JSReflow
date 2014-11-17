/* image-char.c: manipulate information about the characters in the image.

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


#include "image-char.h"


/* Return an initialized, empty list.  */
image_char_list_type
new_image_char_list ()
{
  image_char_list_type l;
  
  IMAGE_CHAR_LIST_LENGTH (l) = 0;
  IMAGE_CHAR_LIST_DATA (l) = NULL;
  
  return l;
}


/* Append the character C to the list L.  */

void
append_image_char (image_char_list_type *l, image_char_type c)
{
  IMAGE_CHAR_LIST_LENGTH (*l)++;
  IMAGE_CHAR_LIST_DATA (*l)
    = xrealloc (IMAGE_CHAR_LIST_DATA (*l),
                  IMAGE_CHAR_LIST_LENGTH (*l) * sizeof (image_char_type));
  IMAGE_CHAR (*l, IMAGE_CHAR_LIST_LENGTH (*l) - 1) = c;
}


/* Return false if the box BOX_COUNT boxes beyond CURRENT_CHAR in LIST
   is in the middle of a character, true otherwise.  To do this, we must
   add up all the box counts for characters starting at FIRST_CHAR,
   until we see where we land.  */

boolean
box_at_char_boundary_p (image_char_list_type list, unsigned current_char,
                        unsigned box_count)
{
  unsigned count = 0;
  
  while (count < box_count && current_char < IMAGE_CHAR_LIST_LENGTH (list))
    {
      image_char_type c = IMAGE_CHAR (list, current_char);
      count += IMAGE_CHAR_BB_COUNT (c);
      current_char++;
    }
    
  return count == box_count;
}
