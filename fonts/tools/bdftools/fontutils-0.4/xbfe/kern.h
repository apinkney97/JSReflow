/* kern.h: declarations for manipulating the kerning table.

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

#ifndef KERN_H
#define KERN_H

#include "xt-common.h"


/* Change the kern for the current character in FONT with the character
   specified by W to be V pixels.  */
extern void change_kern_callback (Widget w, XtPointer font, XtPointer v);

/* Add a kern (with initial value zero) between the current character in
   FONT and the character C.  */
extern void add_kern_callback (Widget w, XtPointer font, XtPointer c);

/* Make a new widget for the character CODE in the font F with value K
   points and add it to the widget W.  */
extern void add_kern_widget (Widget w, font_type *f, charcode_type code, real k);

/* Remove an existing kern between the current character in FONT and the
   character C.  */
extern void remove_kern_callback (Widget w, XtPointer font, XtPointer c);

/* Return the maximum width of a kern item.  */
extern unsigned find_kern_item_width (Widget top);

#endif /* not KERN_H */
