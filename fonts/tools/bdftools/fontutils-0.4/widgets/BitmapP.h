/* BitmapP.h: private definitions for the Bitmap widget, which allows
   editing of a bitmap.

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

#ifndef BITMAP_WIDGET_PRIVATE_H
#define BITMAP_WIDGET_PRIVATE_H

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>
#include <X11/Xaw/LabelP.h>

#include "Bitmap.h"

/* New fields in the Bitmap widget class record, i.e., data applicable
   to every instance of the widget.  */
typedef struct
{
  int dummy;
} BitmapClassPart;

/* The full class record structure.  */
typedef struct _BitmapClassRec
{
  CoreClassPart core_class;
  SimpleClassPart simple_class;
  LabelClassPart label_class;
  BitmapClassPart bitmap_class;
} BitmapClassRec;

extern BitmapClassRec bitmapClassRec;


/* New fields in the Bitmap widget record, i.e., data specific to one
   instance of the widget.  */
typedef struct
{
  /* New resources; see Bitmap.h for documentation.  */
  Dimension expansion;
  XtPointer bits; /* Points to a `bitmap_type'.  */
  Boolean modified;
  Widget shadow;
  
  /* Private state.  */

    /* The GC for the editing.  */
  GC edit_gc;
  
    /* The current selection, or NULL.  */
  bitmap_type *selection;

    /* The initial corner of the selection, in window coordinates.  */
  coordinate_type initial_select;
  
    /* The current upper left corner of the selection, in window
       coordinates.  */ 
  coordinate_type select_ul;
  
    /* The current width and height of the selection, in window dimensions.  */
  unsigned select_width, select_height;
    
    /* The GC for showing the selection.  */
  GC select_gc;
} BitmapPart;


/* The full instance record structure.  */
typedef struct _BitmapRec
{
  CorePart core;
  SimplePart simple;
  LabelPart label;
  BitmapPart bitmap;
} BitmapRec;

#endif /* not BITMAP_WIDGET_PRIVATE_H */
