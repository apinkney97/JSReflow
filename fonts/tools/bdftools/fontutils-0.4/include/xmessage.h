/* message.h: temporarily display a message under X11.

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

#ifndef MESSAGE_H
#define MESSAGE_H

#include "xt-common.h"
#include "types.h"


/* Using the widget W as the parent, display the string S in a popup
   window, using the Label widget.  The ARGS and N_ARGS parameters are
   used when the Label is created.  */
extern void x_message
  (Widget w, string s, ArgList args, Cardinal n_args);

/* Equivalent to `x_message (W, s ".", NULL, 0)'.  */
extern void x_warning (Widget w, string s);

#endif /* not MESSAGE_H */
