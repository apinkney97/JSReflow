/* pk_opcodes.h: symbolic names for some of the GF commands and special
   values.

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

#ifndef PK_OPCODES_H
#define PK_OPCODES_H

/* Some of the command bytes.  */
#define PK_PRE 247
#define PK_ID 89
#define PK_XXX1 240
#define PK_XXX4 243
#define PK_YYY 244
#define PK_POST 245
#define PK_NO_OP 246

/* If the `dyn_f' value for a character is this, the bitmap is
   represented as a bitmap, instead of being packed.  */
#define PK_BITMAP_FLAG 14

/* These values are for the first nybble in packed numbers.  */
#define LONG_RUN_COUNT_FLAG 0
#define REPEAT_COUNT_FLAG 14
#define REPEAT_ONE 15

#endif /* not PK_OPCODES_H */
