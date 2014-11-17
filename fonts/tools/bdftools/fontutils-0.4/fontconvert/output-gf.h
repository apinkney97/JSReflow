/* output-gf.h: declarations for writing a GF file.

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

#ifndef OUTPUT_GF_H
#define OUTPUT_GF_H

#include "types.h"
#include "font.h"


extern void gf_start_output
  (string input_name, string base_name, string dpi, string comment);
extern void gf_output_char (char_info_type, real design_size_ratio);
extern void gf_finish_output (real design_size, real dpi);

#endif /* not OUTPUT_GF_H */
