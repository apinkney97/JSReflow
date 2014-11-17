/* symtab.c: create and update a symbol table.

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

#include "main.h"
#include "symtab.h"


typedef struct symbol_struct
{
  string key;  
  real value;
  struct symbol_struct *next;
} symbol_type;

#define SYM_PTR_KEY(s)  ((s)->key)
#define SYM_PTR_VALUE(s)  ((s)->value)
#define SYM_PTR_NEXT(s)  ((s)->next)


typedef symbol_type*  symbol_table_type;
static symbol_table_type symbol_table = NULL;




static symbol_type *new_symbol (string key, real value);
static symbol_type *find_symbol (string key);



void 
add_to_symbol_table (string key, real value)
{
  symbol_type *found_symbol = find_symbol (key);
  
  if (found_symbol != NULL)
    SYM_PTR_VALUE (found_symbol) = value;
  else
    {
      symbol_type *new = new_symbol (key, value);
      
      SYM_PTR_NEXT (new) = symbol_table;
      symbol_table = new;
      DEBUG_PRINT3 ("Just added key `%s' and value `%f' to symbol table.\n",
		    key, value);
    }
}


static symbol_type *
new_symbol (string key, real value)
{
  symbol_type *new = xmalloc (sizeof (symbol_type));
  
  
  SYM_PTR_KEY (new) = key;
  SYM_PTR_VALUE (new) = value;
  SYM_PTR_NEXT (new) = NULL;
  
  return new;
}



real 
get_symbol_value (string key)
{
  symbol_type *found_symbol = find_symbol (key);
  
  DEBUG_PRINT2 ("Get KEY: `%s'.\n", key);

  if (found_symbol == NULL)
    FATAL1 ("Cannot find symbol `%s' in symbol table", key);

  return SYM_PTR_VALUE (found_symbol);
}



static symbol_type *
find_symbol (string key)
{
  symbol_type *this_symbol = symbol_table;

  while (this_symbol != NULL)
    {
      if (STREQ (SYM_PTR_KEY (this_symbol), key))
        break;
      this_symbol = SYM_PTR_NEXT (this_symbol);
    }

  return this_symbol;
}
