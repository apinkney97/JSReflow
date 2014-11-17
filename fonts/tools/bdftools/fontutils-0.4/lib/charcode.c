/* charcode.c: parse character code strings into their values.

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


/* This routine parses the string CHARCODE as a character code (see the
   .h file).  If CHARCODE cannot be parsed, *VALID is set to false and
   the value returned is unpredictable; otherwise, it is set to true and
   the value returned is the character code.  */

one_byte
parse_charcode (string charcode, boolean *valid)
{
  int code = 75; /* OK, it's not *really* unpredictable.  */
  unsigned length = strlen (charcode);
  
  if (charcode == NULL || length == 0)
    *valid = false;
  else
    {
      /* If a single character, return its value.  */
      if (length == 1)
        {
          *valid = true;
          code = *charcode;
        }

      else if (*charcode == '0')
        { /* Either octal or hex.  */
          if (*(charcode + 1) == 'x' || *(charcode + 1) == 'X')
            /* Apparently not all scanf's ignore a leading `0x'.  */
            *valid = sscanf (charcode + 2, "%x", &code) == 1;
          else
            /* octal */
            *valid = sscanf (charcode, "%o", &code) == 1;
        }

      else
        /* decimal */
        *valid = sscanf (charcode, "%d", &code) == 1;

      /* Since our return type can only hold numbers up to 255 (typically),
         we are justified in using the numeric constant here, instead of
         `MAX_CHARCODE'.  */
      if (*valid)
        *valid = 0 <= code && code <= 255;

    }

  return code;
}


/* This routine calls `parse_charcode', and, if the character code is
   invalid, gives a fatal error.  */

one_byte
xparse_charcode (string charcode)
{
  boolean valid;
  one_byte code = parse_charcode (charcode, &valid);
  
  if (!valid)
    FATAL1 ("xparse_charcode: Invalid character code `%s'", charcode);
    
  return code;
}



