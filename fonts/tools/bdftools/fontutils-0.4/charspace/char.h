/* char.h: character declarations.

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

#ifndef CHAR_H
#define CHAR_H

#include "font.h"
#include "list.h"


/* We want these to mean the values in our own structure.  */
#undef CHAR_LSB
#undef CHAR_RSB


/* How sidebearings are apportioned relative to control letters.  */

typedef struct {
  string left;
  string right;
} sb_apportionments_type;

typedef struct {
  real left;
  real right;
} sidebearings_type;

typedef struct {
  real lsb_percent;
  string displacement;			/* In pixels.  */
} displacement_type;

typedef struct
{
  charcode_type character;
  int kern;
} kern_type;


/* The information about each character in which we're interested.  */
typedef struct
{
  char_info_type char_info;
  sb_apportionments_type sb_apportionments;
  sidebearings_type sidebearings;	
  displacement_type displacement;
  list_type ligatures;
  list_type kerns;			/* In pixels, so are type int.  */
} char_type;


/* The information from the font.  */
#define CHAR_CHARCODE(c)		(CHARCODE (CHAR_INFO (c)))
#define CHAR_INFO(c)	 		((c).char_info)
#define CHAR_LSB_APP(c)			((c).sb_apportionments.left)
#define CHAR_RSB_APP(c)			((c).sb_apportionments.right)
#define CHAR_LSB(c)	 		((c).sidebearings.left)
#define CHAR_RSB(c)	 		((c).sidebearings.right)
#define CHAR_LSB_PERCENT(c)		((c).displacement.lsb_percent)
#define CHAR_DISPLACEMENT(c)		((c).displacement.displacement)
#define CHAR_KERNS_SIZE(c)		(LIST_SIZE ((c).kerns))


#define CHAR_UNSET_VALUE -1

/* Initialize the sidebearings apportionments and the displacement
   information of a character to be unset.  */
extern void init_char (char_type *c);

extern boolean get_char_info (charcode_type, char_type *);

/* Calculate the sidebearings of C using its sidebearing 
   apportionments.  */
extern void do_sidebearings (char_type *);

/* Calculate the sidebearings of C using the left sidebearing percentage
   and the character's displacement.  */
extern void do_displacement (char_type *);

/* Return the kern between the characters LEFT and RIGHT; if no such
   kern exists, return zero.  */
extern int get_kern (char_type left, charcode_type right);

/* Find the INDEX-th kern of character C.  (Return zero if none such.)  */ 
extern int get_kern_element (char_type C, int index);

/* Find the INDEX-th kern character of character C.  (Return zero if
   none such.)  */ 
extern int get_kern_character_element (char_type C, int index);

/* Make the kern between the characters LEFT and RIGHT be K pixels,
   replacing any kern already present.  */
extern void set_kern (char_type *left, charcode_type right, int k);

#endif /* not CHAR_H */
