/* ItemP.h: private definitions for the Item widget.

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

#ifndef ITEM_WIDGET_PRIVATE_H
#define ITEM_WIDGET_PRIVATE_H

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>

#include "Item.h"


/* New fields in the class record, i.e., data applicable to every
   instance of the widget.  */ 
typedef struct
{
  int dummy;
} ItemClassPart;

/* The full class record structure.  */
typedef struct _ItemClassRec
{
  CoreClassPart core_class;
  SimpleClassPart simple_class;
  ItemClassPart item_class;
} ItemClassRec;

extern ItemClassRec itemClassRec;


/* New fields in the Item widget record, i.e., data specific to one
   instance of the widget.  */
typedef struct
{
  /* New resources.  */
  string label;
  string value;
  Dimension length;
  XtCallbackList accept_callback;
  
  /* Private state.  */
  GC normal_GC, highlight_GC;
  Widget label_widget;
  Widget value_widget;
} ItemPart;


/* The full instance record structure.  */
typedef struct _ItemRec
{
  CorePart core;
  SimplePart simple;
  ItemPart item;
} ItemRec;

#endif /* not ITEM_WIDGET_PRIVATE_H */
