/* output-epsf.h: declarations for writing EPS files.

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

#ifndef OUTPUT_EPSF_H
#define OUTPUT_EPSF_H

#include "font.h"
#include "types.h"


extern void epsf_start_output (string base_name);
extern void epsf_output_char (char_info_type);
extern void epsf_finish_output (void);

#endif /* not OUTPUT_EPSF_H */
