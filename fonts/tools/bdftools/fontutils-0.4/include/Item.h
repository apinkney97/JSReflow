/* Item.h: public header file for an ``item widget'': a composite widget
   consisting of a label and a (string) value.  Both subwidgets are
   created by the Item itself, and geometry requests from them are
   ignored.

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

#ifndef ITEM_WIDGET_H
#define ITEM_WIDGET_H

#include "xt-common.h"

#include "types.h"


/* Resources (in addition to those in Core and Composite):

Name		Class		RepType		Default Value
----		-----		-------		-------------
callback	Callback	Callback	NULL
  These routines are called when the accept_value action (see below) is
  performed.  The call_data passed is the contents of the `value'
  resource.

label		Label		String		name of widget
  A constant string displayed before the editable text.  The characters
  in this string are copied by the Item widget.

length		Length		Dimension	1
  The longest `value' allowed, in characters.

translations	Translations	TranslationTable NULL
  The event bindings associated with this widget.  The Item
  provides an action `AcceptValue', which calls the routines in the
  `callback' resource.  `AcceptValue' does not take any parameters.

  By default, no events are bound to `AcceptValue'.  Default bindings
  (perhaps to RET and CTRL-M) should be put in the app-defaults file.

value		Value		String		empty string
  The editable text.  The longest value allowed is the value of `length'.
*/

/* Convenience procedures.  */
extern string ItemGetLabelString (Widget);
extern string ItemGetValueString (Widget);
extern Widget ItemGetValueWidget (Widget);



/* The class variable, for arguments to XtCreateWidget et al.  */
extern WidgetClass itemWidgetClass;

/* The class and instance record types.  */
typedef struct _ItemClassRec *ItemWidgetClass;
typedef struct _ItemRec *ItemWidget;

#endif /* not ITEM_WIDGET_H */
