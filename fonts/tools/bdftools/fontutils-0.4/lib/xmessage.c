/* message.c: pop up a message for five seconds and then go away.

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

#include "xt-common.h"
#include <X11/Shell.h>
#include <X11/Xaw/Label.h>

#include "xmessage.h"

static void message_over (XtPointer, XtIntervalId *);



/* Make a popup child of the widget PARENT displaying the string S.  We
   pass the ARGS and N_ARGS parameters to the creation routine.  */
   
void
x_message (Widget parent, string s, ArgList args, Cardinal n_args)
{
  Dimension parent_height;
  Position popup_x, popup_y;
  Widget popup;
  
  XtAppContext app_con = XtWidgetToApplicationContext (parent);
  Arg parent_args[]
    = { { XtNheight,	(XtArgVal) &parent_height },
      };
  Arg popup_args[]
    = { { XtNx,		0 }, /* We assign to the position below.  */
        { XtNy,		0 },
        { XtNgeometry,	NULL }, /* Don't use the parent's geometry.  */
      };

  Arg default_args[] = { { XtNlabel, (XtArgVal) s } };
  ArgList all_args
    = XtMergeArgLists (args, n_args, default_args, XtNumber (default_args));

  /* Put the message at the left edge of and about halfway down PARENT's
     window.  We pass the address of true Position variables rather
     than the address of the Arg values; in the latter case, the
     endianness of the computer determines which half of the word the
     answers get stored in, which is clearly bad.  */
  XtGetValues (parent, parent_args, XtNumber (parent_args));
  XtTranslateCoords (parent, 0, parent_height / 2, &popup_x, &popup_y);
  popup_args[0].value = popup_x;
  popup_args[1].value = popup_y;
  
  popup = XtCreatePopupShell ("message shell", transientShellWidgetClass,
                              parent, popup_args, XtNumber (popup_args));

  /* We can't use XtNumber on `all_args', since it's a pointer.  */
  (void)
    XtCreateManagedWidget ("message", labelWidgetClass, popup, all_args,
                           n_args + XtNumber (default_args)); 
  
  /* XtPopup realizes the window, etc.  */
  XtPopup (popup, XtGrabNone);
  
  /* Leave the message there for five seconds.  */
  (void) XtAppAddTimeOut (app_con, 5000, message_over, popup); 
}


/* Just a convenient interface to the above.  */

void
x_warning (Widget parent, string s)
{
  x_message (parent, concat (s, "."), NULL, 0);
}



static void
message_over (XtPointer client_data, XtIntervalId *interval_id)
{
  Widget popup = (Widget) client_data;
  
  XtPopdown (popup);
}
