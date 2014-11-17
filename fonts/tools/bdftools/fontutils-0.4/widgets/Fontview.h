/* Fontview.h: public header file for a font-viewing widget, implemented
   as a subclass of Athena's Label widget.

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

#ifndef FONTVIEW_WIDGET_H
#define FONTVIEW_WIDGET_H

#include "xt-common.h"
#include <X11/Xaw/Label.h>


/* Resources (in addition to those in Core, Simple, and Label):

Name		Class		RepType		Default Value
----		-----		-------		-------------
chars		CharInfo	Pointer		NULL
  A pointer to an array of 256 pointers to `char_type' structures
  (see `char.h').  This gives the bitmap and TFM information for each
  character, or, if the corresponding element is NULL, says the
  character doesn't exist.

fontInfo        FontInfo	Pointer		NULL
  A pointer to a `font_type' structure.

lineSpace	Fontdimen	Dimension	65
  The normal interline space from baseline to baseline, in pixels.  The
  NextLine action uses this.

resolution	Resolution	Dimension	300
  The resolution of this font, in pixels per inch.

translations	Translations	TranslationTable xx
  The event bindings associated with xx widget.

wordSpace	Fontdimen	Dimension	10
  The normal interword space, in pixels.  The InsertWordSpace
        action moves right by this much.
*/

#define XtCCharInfo	"CharInfo"
#define XtCFontdimen	"Fontdimen"

#define XtNchars	"charInfo"

#define XtNfontInfo	"fontInfo"
#define XtCFontInfo	"FontInfo"

#define XtNlineSpace	"lineSpace"
#define FONTVIEW_DEFAULT_LINE_SPACE 65

#define XtNresolution	"resolution"
#define XtCResolution	"Resolution"
#define FONTVIEW_DEFAULT_RESOLUTION 300

#define XtNwordSpace	"wordSpace"
#define FONTVIEW_DEFAULT_WORD_SPACE 10



/* The class variable, for arguments to XtCreateWidget et al.  */
extern WidgetClass fontviewWidgetClass;

/* The class record and instance record types.  */
typedef struct _FontviewClassRec *FontviewWidgetClass;
typedef struct _FontviewRec *FontviewWidget;

#endif /* not FONTVIEW_WIDGET_H */
