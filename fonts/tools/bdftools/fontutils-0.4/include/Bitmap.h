/* Bitmap.h: public header file for a bitmap widget, implemented as a
   subclass of Athena's Label widget.

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

#ifndef BITMAP_WIDGET_H
#define BITMAP_WIDGET_H

#include "xt-common.h"
#include <X11/Xaw/Label.h>

#include "bitmap.h"


/* Resources (in addition to those in Core, Simple, and Label):

Name		Class		RepType		Default Value
----		-----		-------		-------------
bits		Bitmap		Pointer		NULL
  A pointer to a `bitmap_type' structure (see `include/bitmap.h') that
  is what is actually displayed.  I didn't want to name this resource
  `bitmap', since there is already a `bitmap' resource in the Label
  widget, and the two are different.  (In fact, `bits' becomes the
  Label's `bitmap'.)

expansion	Expansion	Dimension	16
  How many times each pixel in the bitmap is replicated.

modified	Modified	Boolean		False
  This readonly resource says whether or not the bitmap has been modified.

shadow		Bitmap		BitmapWidget	NULL
  A replica of the widget, possibly with a different expansion
  (typically, in fact, the shadow bitmap has expansion one).  When a
  pixel in the widget is changed and repainted via the InvertPixel
  action (see the `translations' resource below), the corresponding
  pixel in the shadow widget is also changed and repainted.

translations	Translations	TranslationTable NULL
  The event bindings associated with this widget.  The Bitmap widget
  provides an action `InvertPixel', which inverts the value of the pixel
  the pointer is on.  `InvertPixel' takes one parameter, which must be
  one of the symbols `Discrete' or `Continuous' (case is significant).
  In the latter case, the pixel is only changed if the pointer was 1) not
  at that same pixel on the last invocation, and 2) on a pixel that is
  of the opposite color as the pixel at the time of the last `Discrete'
  invocation.  Why?  Because it is most useful to be able to move the
  pointer and change pixels to either black or white, not invert them.

  The Bitmap widget also provides two related sets of actions for copying
  parts of characters.  For the purposes of these actions, imagine that
  the ``hot spots'' of each pixel in the large bitmap are at the upper
  left of the visible square.  StartSelection() begins a selection
  operation at the nearest pixel above and to the left of the current
  point.  AdjustSelection() changes the selected rectangle according to
  the nearest pixel below and to the right of the current pixel; the
  new point can be in any relationship to the initial point.
  AcceptSelection() takes the current selection as the final one.
  
  Once a rectangle has been selected, you can paste it in the same or
  another character: StartPaste() shows the selected rectangle and
  MovePaste() moves it around.  AcceptPaste(Opaque) changes the bitmap
  destructively, i.e., each pixel in the selected rectangle overwrites
  the pixel in the bitmap.  AcceptPaste(Transparent) changes the bitmap
  nondestructively, i.e., a black pixel in the bitmap will remain black.
  
  Aside from pasting the selection, you can also fill it:
  FillSelection() fills the current selection with the color of the
  pixel the pointer is currently on.

  To abort any selection or paste operation, simply move the pointer
  outside the bitmap before finishing the operation.
  
  By default, no events are bound to the actions.  Default bindings
  should be put in the app-defaults file.
*/

#ifndef XtCBitmap
#define XtCBitmap	"Bitmap"
#endif
#define XtCExpansion	"Expansion"
#define XtCModified	"Modified"

#define XtNbits		"bits"

#define XtNexpansion	"expansion"
#define BITMAP_DEFAULT_EXPANSION 16

#define XtNmodified	"modified"

#define XtNshadow	"shadow"

/* Convenience procedures to get the interesting resources.  */
extern unsigned BitmapExpansion (Widget);
extern bitmap_type *BitmapBits (Widget);
extern Boolean BitmapModified (Widget);



/* The class variable, for arguments to XtCreateWidget et al.  */
extern WidgetClass bitmapWidgetClass;

/* The class record and instance record types.  */
typedef struct _BitmapClassRec *BitmapWidgetClass;
typedef struct _BitmapRec *BitmapWidget;

#endif /* not BITMAP_WIDGET_H */
