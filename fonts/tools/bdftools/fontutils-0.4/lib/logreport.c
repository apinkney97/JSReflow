/* logreport.c: showing information to the user.

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

#include "logreport.h"


/* Says whether to output detailed progress reports, i.e., all the data
   on the fitting, as we run.  (-log)  */
boolean logging = false;

FILE *log_file = NULL;


void
flush_log_output ()
{
  if (logging)
    fflush (log_file);
}
