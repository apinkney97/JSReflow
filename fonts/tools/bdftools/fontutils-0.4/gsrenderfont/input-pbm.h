/* input-pbm.h: declarations for reading an PBM file.

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

#ifndef INPUT_PBM_H
#define INPUT_PBM_H

#include "types.h"


/* Prepare to read FILENAME.  */
extern void pbm_open_input_file (string filename);

/* Close up gracefully.  */
extern void pbm_close_input_file (void);


/* Return the next HEIGHT scanlines as a bitmap, or NULL if we are at
   the end of the file.  */
extern bitmap_type *pbm_get_block (unsigned height);

#endif /* not INPUT_PBM_H */

