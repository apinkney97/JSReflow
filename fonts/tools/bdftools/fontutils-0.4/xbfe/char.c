/* char.c: manipulate a TFM and bitmap character simultaneously.

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
#include <X11/Xaw/Form.h>

#include "Bitmap.h"
#include "font.h"
#include "Item.h"
#include "tfm.h"

#include "char.h"
#include "kern.h"
#include "main.h"


static char_type *font_read_char (string, charcode_type);
static void show_bitmap (Widget top, font_type *f, bitmap_type *b);



/* Read the TFM and bitmap character CODE from the font FONT_NAME, and
   return a pointer to a filled-in character structure.  If the
   character doesn't exist in either the TFM or bitmap file, return
   NULL.  If we've already read the character don't read it again.  */

char_type *
read_char (font_type f, charcode_type code)
{
  return FONT_CHAR (f, code)
    ? 
    : font_read_char (FONT_NAME (f), code);
}  


/* Return a pointer to the character CODE from the font named FONT.  If
   the character doesn't exist in either the TFM or the bitmap file,
   return NULL.  */

static char_type *
font_read_char (string font, charcode_type code)
{
  char_type *font_char;
  char_info_type *bitmap_info = get_char (font, code);
  tfm_char_type *tfm_info = tfm_get_char (code);

  if (bitmap_info == NULL || tfm_info == NULL)
    return NULL;

  /* OK, we'll have a character to return.  Fill in the `char_type'
     structure.  */
  font_char = XTALLOC (1, char_type);
  
  CHAR_BITMAP_INFO (*font_char) = *bitmap_info;
  CHAR_TFM_INFO (*font_char) = *tfm_info;

  /* Copy the bitmap into fresh storage, so the original, which the
     library routines store, isn't corrupted.  */
  BCHAR_BITMAP (*font_char) = copy_bitmap (BCHAR_BITMAP (*font_char));
  
  return font_char;
}



/* Make C the current character in the font F, and update the display. 
   The widget TOP is the Form that manages all the other widgets.  */

void
show_char (Widget top, font_type *f, char_type *c)
{
  Widget box_widget, item_widget, lsb_widget, rsb_widget;
  WidgetList old_kerns;
  Cardinal old_kern_count;
  unsigned k;
  list_type kern_list;
  char value[MAX_INT_LENGTH];
  char_info_type bitmap_info = CHAR_BITMAP_INFO (*c);
  charcode_type code = CHARCODE (CHAR_BITMAP_INFO (*c));
  Arg item_args[] = { { XtNvalue, NULL } };  /* We assign to this below.  */

  /* Change the string displayed in the current character widget.  */
  item_widget = XFIND_WIDGET (top, "current character item");
  sprintf (value, "%u", code);
  XTASSIGN_ARG (item_args[0], value);
  XtSetValues (item_widget, XTARG (item_args));
  
  lsb_widget = XFIND_WIDGET (top, "lsb item");
  sprintf (value, "%d", CHAR_LSB (bitmap_info));
  XTASSIGN_ARG (item_args[0], value);
  XtSetValues (lsb_widget, XTARG (item_args));
  
  rsb_widget = XFIND_WIDGET (top, "rsb item");
  sprintf (value, "%d", CHAR_RSB (bitmap_info));
  XTASSIGN_ARG (item_args[0], value);
  XtSetValues (rsb_widget, XTARG (item_args));

  /* Show the bitmap on the display.  */
  show_bitmap (top, f, &BCHAR_BITMAP (*c));

  box_widget = XtNameToWidget (top, "kern box");

  /* Destroy the old kern items.  */
  XtVaGetValues (box_widget, XtNchildren, &old_kerns,
                 XtNnumChildren, &old_kern_count, NULL, NULL);

  /* Unmanage the children explicitly, so that the Box can get rid of
     the old ones before making the new ones.  (XtDestroyWidget doesn't
     actually destroy anything until after all procedures---that's
     us---have finished.)  */
  XtUnmanageChildren (old_kerns, old_kern_count);
  for (k = 0; k < old_kern_count; k++)
    XtDestroyWidget (old_kerns[k]);
  
  /* Add one Item widget for each of the kerns.  */
  kern_list = CHAR_TFM_INFO (*c).kern;
  for (k = 0; k < LIST_SIZE (kern_list); k++)
    {
      tfm_kern_type *kern = LIST_ELT (kern_list, k);
      add_kern_widget (box_widget, f, kern->character, kern->kern);
    }
    
  FONT_CURRENT_CHARCODE (*f) = code;
  FONT_CURRENT_CHAR (*f) = c;
}


/* Display the bitmap B in the font F in both the editing and viewing
   windows.  The widget TOP is the Form that manages all the other
   widgets.  */

static void
show_bitmap (Widget top, font_type *f, bitmap_type *b)
{
  Arg bitmap_args[] = { { XtNbits, (XtArgVal) b } };

  Widget bw = XFIND_WIDGET (top, "*bitmap");
  Widget true_bw = XFIND_WIDGET (top, "true bitmap");

  /* Update the font structure.  */
  if (BitmapModified (bw))
    FONT_MODIFIED (*f) = true;
    
  /* We do want to reconfigure the form after changing `true_bw'---it
     might shrink, thus allowing more space for the big bitmap.  */
  XtSetValues (true_bw, XTARG (bitmap_args));
  XtSetValues (bw, XTARG (bitmap_args));
}
