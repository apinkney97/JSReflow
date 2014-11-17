/* sbi.h: declarations for inputting font spacing information.

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

#ifndef INPUT_H
#define INPUT_H


/* See sbi.c.  */
extern string input_name;


/* Read a .sbi file; set global variables and SB_APPORTIONMENTS 
   accordingly.  */

extern void read_sbi_file (boolean give_warnings, 
			   string input_name, string suffix_dpi,
			   char_type chars[MAX_CHARCODE + 1]);


#endif /* not INPUT_H */


