/* char.c: character utilities.

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
#include "report.h"
#include "list.h"

#include "char.h"
#include "main.h"
#include "symtab.h"


/* Initialize the sidebearings apportionments and the displacement
   information of a character to CHAR_UNSET_VALUE.  */
void
init_char (char_type *c)
{
  CHAR_LSB_APP (*c) = NULL;
  CHAR_RSB_APP (*c) = NULL;
  CHAR_LSB_PERCENT (*c) = CHAR_UNSET_VALUE;
  CHAR_DISPLACEMENT (*c) = NULL;
}


/* Collect the necessary information for the character CODE in the
   structure C.  */

boolean
get_char_info (charcode_type code, char_type *c)
{
  char_info_type *char_info = get_char (font_name, code);

  if (char_info == NULL) 
    return false;
  
  CHAR_INFO (*c) = *char_info;

  return true;
}



static real sym_string_to_value (string sym_string);


/* Calculate the sidebearings of C using its sidebearing apportionments.  */
void
do_sidebearings (char_type *c)
{
  CHAR_LSB (*c) = sym_string_to_value (CHAR_LSB_APP (*c));
  CHAR_RSB (*c) = sym_string_to_value (CHAR_RSB_APP (*c));
}


/* Calculate the sidebearings of C using the left sidebearing percentage
   and the character's displacement.  */
void 
do_displacement (char_type *c)
{  
  int sidebearings_amount
    = sym_string_to_value (CHAR_DISPLACEMENT (*c)) 
      - CHAR_BITMAP_WIDTH (CHAR_INFO (*c));
  
  if (sidebearings_amount < 0)
    {
      WARNING1 ("Character %c displacement too small; \
setting sidebearings to 0", CHAR_CHARCODE (*c));
      CHAR_LSB (*c) = 0;
      CHAR_RSB (*c) = 0;
    }
  else
    {
      CHAR_LSB (*c) = CHAR_LSB_PERCENT (*c) * sidebearings_amount;
      CHAR_RSB (*c) = sidebearings_amount - CHAR_LSB (*c);
    }
}



/* Get the value of a sidebearing string.  */

static real
sym_string_to_value (string sym_string)
{
  /* If it's a number, just convert it.  */
  
  if (isdigit (sym_string[0]) || sym_string[0] == '.' || sym_string[0] == '-')
    return atof (sym_string);
  
  /* Otherwise, look it up in the symbol table.  */
  else
    return get_symbol_value (sym_string);
}



/* The character RIGHT may or may not be in the list of kerns already.  */

void
set_kern (char_type *left, charcode_type right, int k)
{
  unsigned this_right;
  kern_type *new_kern;
  list_type kern_table;
  
  kern_table = left->kerns;

  for (this_right = 0; this_right < LIST_SIZE (kern_table); this_right++)
    {
      kern_type *kern = LIST_ELT (kern_table, this_right);

      if (kern->character == right)
	{			/* Already there.  */
	  kern->kern = k;
	  return;
	}
    }

  /* RIGHT wasn't in the existing list.  Add it to the end.  */
  new_kern = append_element (&(left->kerns), sizeof (kern_type));
  new_kern->character = right;
  new_kern->kern = k;
}


/* Find the kern between the characters LEFT and RIGHT.  (Return zero if
   none such.)  */ 

int
get_kern (char_type left, charcode_type right)
{
  list_type kern_table;
  unsigned r;
  
  kern_table = left.kerns;

  for (r = 0; r < LIST_SIZE (kern_table); r++)
    {
      kern_type *kern = LIST_ELT (kern_table, r);

      if (kern->character == right)
	return kern->kern;
    }

  return 0;
}


/* Find the INDEX-th kern of character C.  (Return zero if none such.)  */ 

int
get_kern_element (char_type c, int index)
{
  list_type kern_table = c.kerns;
  kern_type *kern;

  if (index < 0 || index >= LIST_SIZE (kern_table))
    return 0;

  kern = LIST_ELT (kern_table, index);

  return kern->kern;
}


/* Find the INDEX-th kern character of character C.  (Return zero if
   none such.)  */ 

int
get_kern_character_element (char_type c, int index)
{
  list_type kern_table = c.kerns;
  kern_type *kern;
  
  if (index < 0 || index >= LIST_SIZE (kern_table))
    return 0;

  kern = LIST_ELT (kern_table, index);

  return kern->character;
}



