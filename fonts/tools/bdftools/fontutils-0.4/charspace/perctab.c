/* perctab.c: create and update a percentage definition table.

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
#include "perctab.h"
#include "symtab.h"


typedef struct perc_def_struct
{
  string key;  
  real percent;
  string percent_of;  
  struct perc_def_struct *next;
} perc_def_type;

#define PERC_DEF_PTR_KEY(s)  ((s)->key)
#define PERC_DEF_PTR_PERCENT(s)  ((s)->percent)
#define PERC_DEF_PTR_PERCENT_OF(s)  ((s)->percent_of)
#define PERC_DEF_PTR_NEXT(s)  ((s)->next)


typedef perc_def_type*  perc_def_table_type;
static perc_def_table_type perc_def_table = NULL;




static perc_def_type *new_perc_def (string key, real percent, 
				    string percent_of);
static perc_def_type *find_perc_def (string key);



void 
add_to_perc_def_table (string key, real percent, string percent_of)
{
  perc_def_type *found_perc_def = find_perc_def (key);
  string percent_of_copy = xstrdup (percent_of);
  
  if (found_perc_def != NULL)
    {
      PERC_DEF_PTR_PERCENT (found_perc_def) = percent;
      PERC_DEF_PTR_PERCENT_OF (found_perc_def) = percent_of_copy;
    }
  else
    {
      perc_def_type *new = new_perc_def (key, percent, percent_of_copy);
      
      PERC_DEF_PTR_NEXT (new) = perc_def_table;
      perc_def_table = new;
      DEBUG_PRINT4 ("Just added key `%s', percent `%f' and percent of `%s'\
to perc_def table.\n", key, percent, percent_of);
    }
}


static perc_def_type *
find_perc_def (string key)
{
  perc_def_type *this_perc_def = perc_def_table;

  while (this_perc_def != NULL)
    {
      if (STREQ (PERC_DEF_PTR_KEY (this_perc_def), key))
        break;
      this_perc_def = PERC_DEF_PTR_NEXT (this_perc_def);
    }

  return this_perc_def;
}


static perc_def_type *
new_perc_def (string key, real percent, string percent_of)
{
  perc_def_type *new = xmalloc (sizeof (perc_def_type));
  
  
  PERC_DEF_PTR_KEY (new) = key;
  PERC_DEF_PTR_PERCENT (new) = percent;
  PERC_DEF_PTR_PERCENT_OF (new) = percent_of;
  PERC_DEF_PTR_NEXT (new) = NULL;
  
  return new;
}



void
add_perc_defs_to_symbol_table ()
{
  perc_def_type *this_perc_def = perc_def_table;

  while (this_perc_def != NULL)
    {
      add_to_symbol_table (PERC_DEF_PTR_KEY (this_perc_def),
	PERC_DEF_PTR_PERCENT (this_perc_def)
	* get_symbol_value (PERC_DEF_PTR_PERCENT_OF (this_perc_def)));

      this_perc_def = PERC_DEF_PTR_NEXT (this_perc_def);
    }
}



