/* kern.c: manipulate the kerning table.

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

#include <ctype.h>
#include "xt-common.h"
#include "Item.h"
#include "xmessage.h"

#include "char.h"
#include "kern.h"
#include "main.h"

static int find_char_in_kern_list (list_type, charcode_type);




/* Change the kern value for the current character in the font
   CLIENT_DATA, followed by the character given by W's label, to be
   CALL_DATA.  */

void
change_kern_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
  string value_string = call_data;

  /* Get the new kern value.  Since this is something the user typed, we
     have to be cautious about whether it's valid.  */
  string test = call_data;

  /* Allow leading `-' sign.  */
  if (*test == '-')
    test++;
  
  /* Skip decimal digits.  */
  while (isdigit (*test))
    test++;
  
  /* If a `.' follows, can have more digits.  */
  if (*test == '.')
    {
      test++;
      while (isdigit (*test))
        test++;
    }
  
  /* We should end up at the null terminator.  */
  if (*test != 0)
    {
      string s = concat3 ("`", value_string, "': invalid real");
      x_warning (XtParent (XtParent (w)), s);
      free (s);
    }
  else
    { /* The value they typed is ok.  */
      charcode_type kern_char;
      unsigned u;
      font_type *f = (font_type *) client_data;
      char_type *c = FONT_CURRENT_CHAR (*f);
  
      string label_string = ItemGetLabelString (w);

      real kern_value = PIXELS_TO_POINTS (atof (value_string), FONT_DPI (*f));

      /* Extract the character code out of the item label.  */
      assert (sscanf (label_string, "%u: ", &u) == 1);
      kern_char = u;
  
      /* Update an existing value, or add a new one.  */
      tfm_set_kern (&CHAR_TFM_INFO (*c), kern_char, kern_value);

      FONT_MODIFIED (*f) = true;
    }
}



/* Add a new kern between the current character and the one specified in
   CALL_DATA.  Give it initial value zero (the user can edit the new
   value as usual once it exists).  */

void
add_kern_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
  charcode_type code; 
  string value_string = call_data;
  
  if (XTPARSE_CHARCODE (code, value_string, w))
    {
      font_type *f = (font_type *) client_data;
      char_type *c = FONT_CURRENT_CHAR (*f);
      tfm_char_type tfm_char = CHAR_TFM_INFO (*c);
      list_type kern_list = tfm_char.kern;
      
      if (find_char_in_kern_list (kern_list, code) >= 0)
        x_warning (XtParent (w), "Already in kern list");
      else
        {
          Widget kern_box;
          
          /* Add the kern element to the font.  */
          tfm_set_kern (&CHAR_TFM_INFO (*c), code, 0.0);
          FONT_MODIFIED (*f) = true;
          
          /* Make the new widget.  */
          kern_box = XFIND_WIDGET (XtParent (w), "kern box");
          add_kern_widget (kern_box, f, code, 0.0);
        }

      /* Clear the character code we just processed from the Item.  */
      XtVaSetValues (w, XtNvalue, "", NULL, NULL);
    }
}


/* Create a new Item widget for editing a kern with character CODE in
   font F and value KERN points.  Add the new widget to BOX.  */

/* The length here is 1 for a possible sign, 2 for the integer part, the
   decimal point, and 1 for the fractional part.  We're assuming no kern
   will have magnitude > 99.9 pixels.  If this fails, the toolkit will
   give an error about the length not being long enough.  */
#define KERN_VALUE_WIDTH 5

void
add_kern_widget (Widget box, font_type *f, charcode_type code, real kern)
{
  char name[MAX_INT_LENGTH + 6];
  char val[MAX_INT_LENGTH + 4];
  string code_string = xmalloc (MAX_INT_LENGTH + 2);
  XtCallbackRec change_kern_callbacks[]
    = SINGLE_CALLBACK (change_kern_callback, f);
  Arg kern_args[]
    = { { XtNlabel,	NULL }, /* Assigned below.  */
        { XtNvalue, 	NULL },
        { XtNcallback,	(XtArgVal) change_kern_callbacks },
        { XtNlength, 	(XtArgVal) KERN_VALUE_WIDTH },
      };

  sprintf (code_string, "%u: ", code);
  XTASSIGN_ARG (kern_args[0], code_string);

  /* We can't use `POINTS_TO_PIXELS' here, since that rounds to the
     nearest whole number of pixels, and we want fractional pixels.  */
  sprintf (val, "%.1f", kern * FONT_DPI (*f) / POINTS_PER_INCH);
  XTASSIGN_ARG (kern_args[1], val);

  sprintf (name, "kern %u", code);
  (void) XtCreateManagedWidget (name, itemWidgetClass, box, XTARG (kern_args));
}



/* Remove an existing kern between the current character and the one
   specified in CALL_DATA.  */

void
remove_kern_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
  charcode_type code; 
  string value_string = call_data;
  
  if (XTPARSE_CHARCODE (code, value_string, w))
    {
      font_type *f = (font_type *) client_data;
      char_type *c = FONT_CURRENT_CHAR (*f);
      list_type *kern_list = &(CHAR_TFM_INFO (*c).kern);
      int loc = find_char_in_kern_list (*kern_list, code);
      
      if (loc < 0)
        x_warning (XtParent (w), "Not in kern list");
      else
        {
          unsigned e;
          char kern_name[7 + MAX_INT_LENGTH];
          
          /* Remove the kern from the list in the font.  */
          free (LIST_ELT (*kern_list, loc));
          for (e = loc + 1; e < LIST_SIZE (*kern_list); e++)
            LIST_ELT (*kern_list, e - 1) = LIST_ELT (*kern_list, e);
          LIST_SIZE (*kern_list)--;
          
          /* Get rid of the widget.  We need the `*' to find the name,
             since the kern items aren't children of the Form, they are
             children of a Box which is a child of the Form.  */
          sprintf (kern_name, "*kern %u", code);
          XtDestroyWidget (XFIND_WIDGET (XtParent (w), kern_name));

          /* Clear the character code we just processed from the Item.  */
          XtVaSetValues (w, XtNvalue, "", NULL, NULL);
	
          FONT_MODIFIED (*f) = true;
        }
    }
}


/* Return the index of the element in KERN_LIST which has code CODE, or
   -1 if no such kern is in KERN_LIST.  */

static int
find_char_in_kern_list (list_type kern_list, charcode_type code)
{
  unsigned e;
  
  for (e = 0; e < LIST_SIZE (kern_list); e++)
    {
      tfm_kern_type *k = LIST_ELT (kern_list, e);
      
      if (k->character == code)
        return e;
    }
  
  return -1;
}



/* Return the maximum width of a kern item.  Since there may not be any
   kern items extant, we can't just find one and ask for its widget
   width.  Instead, we look up the font resources in the database
   (using the widget TOP as a handle) and then compute the width
   with the X text routines.  */

unsigned
find_kern_item_width (Widget top)
{
  XrmValue db_value;
  XFontStruct *f;
  string font_name;
  int kern_width, label_width;
  Dimension label_internal_width;
  string rep_type;
  Display *d = XtDisplay (top);
  XrmDatabase db = XtDatabase (d);

  string class = CLASS_NAME ".Box.Item.Label.Font";
  string name = concat (XtName (top), ".kern box.kern 0.item label.font");
  
  /* Find the max width of the label.  The string here is what is used
     in `add_kern_widget', above.  We assume that 0 is as wide as any
     other number.  This assertion and the one below are ok because we
     should never be using the server default font for these items.  To
     be absolutely safe, we should look up XtDefaultFont if this fails,
     but it hardly seems worth it.  */
  assert (XrmGetResource (db, name, class, &rep_type, &db_value));
  
  /* Retrieve the font structure.  */
  font_name = db_value.addr;
  f = XLoadQueryFont (d, font_name);
  
  /* Find the width.  Assume no nulls in TEXT.  */
  label_width = XTextWidth (f, "000: ", 5);

  /* Free the memory used.  */
  XFreeFont (d, f);
  free (name);
  
  
  /* Find the max width of the value.  Here we must use the same
     algorithm the Item does for its value subwidget: multiply the
     length of the text by the maximum width from the font bounding box.  */
  class = CLASS_NAME ".Box.Item.Text.Font";
  name = concat (XtName (top), ".kern box.kern 0.item value.font");
  assert (XrmGetResource (db, name, class, &rep_type, &db_value));
  
  /* Retrieve the font structure.  */
  font_name = db_value.addr;
  f = XLoadQueryFont (d, font_name);
  kern_width
    = KERN_VALUE_WIDTH * (f->max_bounds.rbearing - f->min_bounds.lbearing);
  
  /* Free the memory used.  */
  XFreeFont (d, f);
  free (name);
  
  
  /* Last thing: look up the internal width of the label subwidget of
     the Item, so we can add that in.  We assume the margins on the
     value subwidget are zero, and that the borders are zero.  */
  class = CLASS_NAME ".Box.Item.Label.Width";
  name = concat (XtName (top), ".kern box.kern 0.item label.internalWidth");
  label_internal_width
    = XrmGetResource (db, name, class, &rep_type, &db_value)
      ? atoi (db_value.addr)
      : 4; /* The Label widget's default.  */
  free (name);

  return label_width + label_internal_width * 2 + kern_width;
}
