/* Item.c: implementation of an Item widget, for simple user
   interaction.

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
#include <X11/IntrinsicP.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/LabelP.h>
#include <X11/Xaw/AsciiText.h>


#include "ItemP.h"


/* These routines perform the actions.  The actions are specified in the
   app-defaults file; they take place in the AsciiText widget we create,
   rather than the Item widget itself.  */
static action_proc_type accept_value;

/* The mapping of action names to procedures.  */
static XtActionsRec item_actions[]
  = { { "AcceptValue", accept_value },
    };


/* This structure defines the default values of the resources specific
   to the Item widget.  */
#define OFFSET(field) XtOffset (ItemWidget, item.field)
static XtResource item_resources[]
  = { { XtNcallback, XtCCallback, XtRCallback, sizeof (XtCallbackList),
          OFFSET (accept_callback), XtRCallback, NULL },
      STRING_RESOURCE (label, Label, NULL),
      STRING_RESOURCE (value, Value, ""),
      IMMEDIATE_RESOURCE (length, Length, Dimension, 1),
    };

/* Routines in the class record.  */
static void item_initialize (Widget, Widget, ArgList, Cardinal *);
static void item_realize (Widget, XtValueMask *, XSetWindowAttributes *);
static Boolean item_set_values (Widget, Widget, Widget, ArgList, Cardinal *);
static void item_expose (Widget, XEvent *, Region);
static void item_destroy (Widget);

/* We can inherit most things in the instantiation of the class record.  */
ItemClassRec itemClassRec
  = { /* Core class fields.  */
      { /* superclass	  	*/ (WidgetClass) &simpleClassRec,
        /* class_name	  	*/ "Item",
        /* widget_size	  	*/ sizeof (ItemRec),
        /* class_initialize   	*/ NULL,
        /* class_part_initialize*/ NULL,
        /* class_inited       	*/ FALSE,
        /* initialize	  	*/ item_initialize,
        /* initialize_hook	*/ NULL,
        /* realize		*/ item_realize,
        /* actions		*/ item_actions,
        /* num_actions	  	*/ XtNumber (item_actions),
        /* resources	  	*/ item_resources,
        /* num_resources	*/ XtNumber (item_resources),
        /* xrm_class	  	*/ NULLQUARK,
        /* compress_motion	*/ TRUE,
        /* compress_exposure  	*/ TRUE,
        /* compress_enterleave	*/ TRUE,
        /* visible_interest	*/ FALSE,
        /* destroy		*/ item_destroy,
        /* resize		*/ XtInheritResize,
        /* expose		*/ item_expose,
        /* set_values	  	*/ item_set_values,
        /* set_values_hook	*/ NULL,
        /* set_values_almost	*/ XtInheritSetValuesAlmost,
        /* get_values_hook	*/ NULL,
        /* accept_focus	 	*/ NULL,
        /* version		*/ XtVersion,
        /* callback_private   	*/ NULL,
        /* tm_table		*/ NULL,
        /* query_geometry	*/ XtInheritQueryGeometry,
        /* display_accelerator	*/ XtInheritDisplayAccelerator,
        /* extension		*/ NULL
      },

      /* Simple class fields.  */
      { /* change_sensitive */ XtInheritChangeSensitive },

      /* Item class fields.  */
      { 0 }
    };

WidgetClass itemWidgetClass = (WidgetClass) &itemClassRec;



/* Class routines.  */

/* This routine is called at widget creation time by the toolkit, after
   our superclasses have been initialized.  REQUEST is the widget that
   was originally requested by the user; NEW is the widget that has been
   created in response to the requests by our superclasses.  We do not
   use the parameters ARGS and N_ARGS.
   
   We create two children, a Label and an AsciiText widget.  Because we
   are not a subclass of Composite, we must realize and otherwise
   operate on those widgets ourselves.
   
   Since it is the AsciiText widget that handles the events, we must
   replace its event bindings with our own, where they conflict.  The
   Intrinsics have already converted the string representation of the
   translation table in the class record into the compiled form. 
   (Incidentally, perhaps accelerators would be a better way to do this.
   I'm not sure.)   */

static void
item_initialize (Widget request, Widget new, ArgList args, Cardinal *n_args)
{
  Widget label_widget, value_widget;
  XFontStruct *value_font;
  unsigned value_width;
  ItemWidget iw = (ItemWidget) new;
  Arg label_args[]
    = { { XtNlabel,	(XtArgVal) (iw->item.label ? : iw->core.name) },
      };

  string temp = xmalloc (iw->item.length + 1);
  Arg text_args[]
    = { { XtNstring,		(XtArgVal) temp },
        { XtNlength,		iw->item.length },
      };
  
  /* Create the widget to handle the label.  */
  iw->item.label_widget = label_widget
    = XtCreateWidget ("item label", labelWidgetClass, new, XTARG (label_args));
  label_widget->core.x = 0;
  label_widget->core.y = 0;

  if (strlen (iw->item.value) > iw->item.length)
    {
      string error_params[]
        = { itoa (iw->item.length), iw->item.value };
      *n_args = XtNumber (error_params);
      XtErrorMsg ("invalidParameter", "itemInitialize", "ItemError",
                  "Length resource of %s too short for value `%s'",
                  error_params, n_args);
    }
                    
  /* Save the string in our own storage.  */
  strcpy (temp, iw->item.value);
  iw->item.value = temp;
  
  /* Create the widget to handle the editing.  */
  iw->item.value_widget = value_widget
    = XtCreateWidget ("item value", asciiTextWidgetClass, new,
                      XTARG (text_args));

  /* Since the default width for Text objects is a constant number
     (100), we compute the longest width we expect here, based on the
     Text's font and our own length resource. */
  XtVaGetValues (value_widget, XtNfont, (XtArgVal) &value_font, NULL, NULL);
  value_width = iw->item.length * (value_font->max_bounds.rbearing
                                   - value_font->min_bounds.lbearing);
  XtVaSetValues (value_widget, XtNwidth, value_width, NULL, NULL);

  /* Put the value widget next to the label widget.  */
  value_widget->core.x = label_widget->core.width;
  value_widget->core.y = 0;

  /* Key events should go to the AsciiText widget.  */
  XtSetKeyboardFocus ((Widget) iw, value_widget);
  
  /* If our own width and/or height are zero, set them appropriately.  */
  if (iw->core.width == 0)
    iw->core.width = label_widget->core.width + value_widget->core.width; 

  if (iw->core.height == 0)
    iw->core.height
      = MAX (label_widget->core.height, value_widget->core.height);

  /* Set up normal and inverse GC's for highlighting.  */
  {
    XGCValues values;
    
    iw->item.normal_GC = ((LabelWidget) label_widget)->label.normal_GC;
    
    values.foreground = iw->core.background_pixel;
    values.background = ((LabelWidget) label_widget)->label.foreground;
    iw->item.highlight_GC = XtGetGC ((Widget) iw, GCForeground | GCBackground,
                                     &values);
  }
}


/* Because we have children, and are not a subclass of Composite, we
   must realize the children ourselves.  (See p.41 of the Xt manual.) */

static void
item_realize (Widget w, XtValueMask *value_mask, XSetWindowAttributes *attrs)
{
  ItemWidget iw = (ItemWidget) w;
  
  /* Create our own window in the same way as Core's realize procedure.  */
  XtCreateWindow (w, CopyFromParent, CopyFromParent, *value_mask, attrs);
  
  /* Create the windows for our children.  */
  XtRealizeWidget (iw->item.label_widget);
  XtRealizeWidget (iw->item.value_widget);

  /* Map those windows.  */
  XtMapWidget (iw->item.label_widget);
  XtMapWidget (iw->item.value_widget);
}


/* This routine is called when one of the resources in the widget
   changes; for example, in response to an XtSetValues call.  The
   toolkit has already modified our resources.  OLD is the widget before
   any resources were changed; REQUEST is the widget before any class
   `set_values' procedures have been called; NEW is the widget as
   updated by the superclasses.  We don't handle cases where the new
   values should cause resizes.  */

static Boolean
item_set_values (Widget old, Widget request, Widget new, ArgList args,
                 Cardinal *n_args) 
{
  Boolean redisplay = False;
  ItemWidget old_iw = (ItemWidget) old;
  ItemWidget iw = (ItemWidget) new;
  Arg label_args[] = { { XtNlabel, (XtArgVal) iw->item.label } };
  boolean needs_length = false;
  
  /* It is safe to assume that the `label_widget' field is initialized,
     since `item_initialize' must have been called before us, and the
     application is not supposed to have direct access to the Label widget.  */
  if (old_iw->item.label != iw->item.label)
    {
      XtSetValues (iw->item.label_widget, XTARG (label_args));
      redisplay = True;
    }

  if (old_iw->item.length != iw->item.length)
    { /* They changed the length.  Make sure it's long enough.  */
      if (strlen (iw->item.value) > iw->item.length)
        {
          string error_params[]
            = { itoa (iw->item.length), iw->item.value };
          *n_args = XtNumber (error_params);
          XtErrorMsg ("invalidParameter", "itemSetValues", "ItemError",
                      "Length resource of %s too short for value `%s'",
                      error_params, n_args);
        }
      needs_length = true;
    }

  if (needs_length || old_iw->item.value != iw->item.value)
    { /* The string to edit has changed.  */
      string temp = xmalloc (iw->item.length + 1);
      Arg text_args[] = { { XtNstring,		(XtArgVal) temp },
                          { XtNlength,		iw->item.length },
                        };

      strcpy (temp, iw->item.value);
      free (old_iw->item.value);
      iw->item.value = temp;

      XtSetValues (iw->item.value_widget, XTARG (text_args));
      redisplay = true;
    }
    
  return redisplay;
}


/* This routine is called whenever part of the widget needs to be
   redisplayed.  We simply pass on our arguments to our child widgets,
   since we don't have any information ourselves.  */

static void
item_expose (Widget w, XEvent *event, Region region)
{
  ItemWidget iw = (ItemWidget) w;
  Widget lw = iw->item.label_widget;
  Widget tw = iw->item.value_widget;
  
  labelWidgetClass->core_class.expose (lw, event, region);
  asciiTextWidgetClass->core_class.expose (tw, event, region);
}


/* This routine is called when the widget is destroyed.  We are not a
   subclass of Composite, so we must destroy our children ourselves.  */

static void
item_destroy (Widget w)
{
  ItemWidget iw = (ItemWidget) w;
  
  XtDestroyWidget (iw->item.label_widget);
  XtDestroyWidget (iw->item.value_widget);  
  free (iw->item.value);
}



/* This convenience procedure saves clients the trouble of constructing
   an ArgList to get the value string from the Item widget.  Since we
   use the string in place, we don't have to get the current value from
   our Text widget.  */

string
ItemGetValueString (Widget w)
{
  ItemWidget iw = (ItemWidget) w;
  return iw->item.value;
}


/* A similar routine, for the label string.  */
string
ItemGetLabelString (Widget w)
{
  ItemWidget iw = (ItemWidget) w;
  return iw->item.label;
}


/* This can't be duplicated with a resource request, but it's pretty
   similar: a client might need to get the value widget itself.  */

Widget
ItemGetValueWidget (Widget w)
{
  ItemWidget iw = (ItemWidget) w;
  return iw->item.value_widget;
}



/* Call the routines on the callback list.  W is the AsciiText widget we
   created, rather than an Item widget, because the events actually
   take place in the former.  (Because we didn't use accelerators.)  */

static void
accept_value (Widget w, XEvent *event, String *s, Cardinal *c)
{
  ItemWidget iw = (ItemWidget) XtParent (w);
  Display *display = XtDisplay (iw);
  LabelWidget lw = (LabelWidget) iw->item.label_widget;
  
  lw->label.normal_GC = iw->item.highlight_GC;
  XClearArea (display, XtWindow (iw), 0, 0,
              iw->core.width, iw->core.height, True);
  XtCallCallbacks ((Widget) iw, XtNcallback, iw->item.value);
  lw->label.normal_GC = iw->item.normal_GC;
}
