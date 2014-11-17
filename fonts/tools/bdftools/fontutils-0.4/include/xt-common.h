/* xt-common.h: declarations that all programs using Xt need.

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

#ifndef XT_COMMON_H
#define XT_COMMON_H

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>


/* Our own definitions.  */

/* It is convenient to have this type for declaring action procedures.  */
typedef void action_proc_type (Widget, XEvent *, String *, Cardinal *);

/* We virtually always want an arg array and its length together in
   parameter lists.  */
#define XTARG(arg_array) (arg_array), XtNumber (arg_array)

/* Assign a value to an Xt argument.  */
#define XTASSIGN_ARG(arg, val) (arg).value = (XtArgVal) (val)

/* We use command buttons the same way in all programs, so here are
   macros to make it easy to use them.  */

/* Our buttons only have one procedure associated with them.  This macro
   expands into an XtCallbackRec array.  */
#define SINGLE_CALLBACK(proc, data)					\
  { { (XtCallbackProc) proc, (XtPointer) (data) },			\
    { NULL, NULL }							\
  }

/* This macro declares a command widget.  The routine that performs the
   action should be named `NAME_command'; it will be passed the
   CLIENT_DATA argument.  The string TITLE appears in the window.  */
#define DECLARE_BUTTON(name, title, client_data)			\
  XtCallbackRec name##_callback_list[]					\
    = SINGLE_CALLBACK (name##_command, client_data);			\
  Arg name##_args[]							\
    = { { XtNfromHoriz,	NULL }, /* We assign to this below.  */		\
        { XtNlabel,	(XtArgVal) title },				\
        { XtNcallback,	(XtArgVal) name##_callback_list },		\
      };								\
  Widget name##_widget /* The invoker supplies the semicolon.  */
  

/* This macro defines a command button NAME, by using the variables
   that DECLARE_BUTTON creates.  It also keeps track of the widget that
   should be to the left of the new one, by assuming a variable
   `widget_to_the_left'.  */

#define DEFINE_BUTTON(name, parent)					\
  name##_args[0].value = (XtArgVal) widget_to_the_left;			\
  name##_widget = XtCreateManagedWidget (#name, commandWidgetClass,	\
					 parent, XTARG (name##_args));	\
  widget_to_the_left = name##_widget
  /* The invoker supplies the semicolon.  */


/* Parse a character code in STR; if invalid, give a warning.  If valid,
   assign the value to CODE.  Return whether it was valid.  */
#define XTPARSE_CHARCODE(code, str, widget)				\
  ({									\
    boolean valid;							\
    charcode_type test = parse_charcode (str, &valid);			\
									\
    if (valid)								\
      code = test;							\
    else								\
      {									\
        string s = concat3 ("`", str, "': invalid character code");	\
        x_warning (XtParent (widget), s);				\
        free (s);							\
      };								\
    valid;								\
  })


/* Find the widget whose name is NAME in the widget tree rooted at
   TOP.  If no such widget can be found, give a fatal error.
   
   We use the extra variable `root' in case `top' is an expression
   involving some other widget named `w'.  */
#define XFIND_WIDGET(top, name)						\
    ({									\
      Widget root = top;						\
      Widget w = XtNameToWidget (root, name);				\
      if (w == NULL)							\
        {								\
          string s = concat3 ("Cannot find widget `", name, "'."); 	\
          XtErrorMsg ("noWidget", "FindWidget", "Error", s, NULL, 0);	\
        }								\
      w;								\
    })


/* For consistency, so we can give `Pointer' as a `type' below.  */
typedef XtPointer Pointer;

/* We define our widget resources in a predictable way.  We assume an
   `OFFSET' macro has been defined as, e.g.,
   #define OFFSET(field) XtOffset (BitmapWidget, bitmap.field)
*/
#define DEFINE_RESOURCE(name, field_name, class_name, type,		\
                        default_type, default)				\
  { XtN##name, XtC##class_name, XtR##type, sizeof (type),		\
    OFFSET (field_name), XtR##default_type, (XtPointer) (default) }

#define IMMEDIATE_RESOURCE2(name, field_name, class_name, type, default)\
  DEFINE_RESOURCE (name, field_name, class_name, type, Immediate, default)

#define IMMEDIATE_RESOURCE(name, class_name, type, default)		\
  IMMEDIATE_RESOURCE2 (name, name, class_name, type, default)

#define STRING_RESOURCE(name, class_name, default)			\
  DEFINE_RESOURCE (name, name, class_name, String, String, default)

#endif /* not XT_COMMON_H */
