/* pstype1.h: declarations for translating the BZR file to a Type 1
   PostScript font.

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

#ifndef PSTYPE1_H
#define PSTYPE1_H

#include "bzr.h"
#include "tfm.h"
#include "types.h"


/* This is called first.  */
extern void pstype1_start_output (string font_name, string output_name,
				  bzr_preamble_type, bzr_postamble_type,
                                  tfm_global_info_type);

/* Then this is called for each character.  */
extern void pstype1_output_char (bzr_char_type);

/* And finally this is called at the end.  */
extern void pstype1_finish_output (void);

#endif /* not PSTYPE1_H */
