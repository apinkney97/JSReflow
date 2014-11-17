/* FontviewP.h: private definitions for the Fontview widget, which
   allows viewing of a bitmap font.

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

#ifndef FONTVIEW_WIDGET_PRIVATE_H
#define FONTVIEW_WIDGET_PRIVATE_H

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>
#include <X11/Xaw/LabelP.h>

#include "Fontview.h"


/* New fields in the Fontview widget class record, i.e., data applicable
   to every instance of the widget.  */
typedef struct
{
  int dummy;
} FontviewClassPart;

/* The full class record structure.  */
typedef struct _FontviewClassRec
{
  CoreClassPart core_class;
  SimpleClassPart simple_class;
  LabelClassPart label_class;
  FontviewClassPart fontview_class;
} FontviewClassRec;

extern FontviewClassRec fontviewClassRec;


/* New fields in the Fontview widget record, i.e., data specific to one
   instance of the widget.  */
typedef struct
{
  /* New resources; see Fontview.h for documentation.  */
  XtPointer chars;	/* Points to an array of `char_type's.  */
  XtPointer font_info;	/* Points to a `font_type'.  */
  Dimension line_space;
  Dimension resolution;
  Dimension word_space;

  /* Private state.  */

    /* The `current character', or NULL.  */
  char_type *current_char;
  
    /* Where we are, in window coordinates, relative to the upper-left
       corner of the pixmap.  */
  coordinate_type current_pos;
  
    /* A GC for drawing things.  */
  GC gc;
} FontviewPart;


/* The full instance record structure.  */
typedef struct _FontviewRec
{
  CorePart core;
  SimplePart simple;
  LabelPart label;
  FontviewPart fontview;
} FontviewRec;

#endif /* not FONTVIEW_WIDGET_PRIVATE_H */
