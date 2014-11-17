/* Fontview.c: implementation of a Fontview widget.

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


#include "char.h"
#include "font.h"
#include "FontviewP.h"

/* Utility routines.  */
static Pixmap bitmap_to_pixmap (Display *, GC, bitmap_type);
static charcode_type find_keycode (XEvent *, boolean *);



/* Action routines.  */

static action_proc_type self_insert;

/* The mapping of action names to procedures.  */
static XtActionsRec fontview_actions[]
  = { { "SelfInsert", self_insert },
    };

/* This structure defines the default values of the resources specific
   to the Bitmap widget.  */
#define OFFSET(field) XtOffset (FontviewWidget, fontview.field)
static XtResource fontview_resources[]
  = { IMMEDIATE_RESOURCE (chars, CharInfo, Pointer, NULL),
      IMMEDIATE_RESOURCE2 (fontInfo, font_info, FontInfo, Pointer, NULL),
      IMMEDIATE_RESOURCE2 (lineSpace, line_space, Fontdimen, Dimension,
                           FONTVIEW_DEFAULT_LINE_SPACE),
      IMMEDIATE_RESOURCE2 (wordSpace, word_space, Fontdimen, Dimension,
                           FONTVIEW_DEFAULT_WORD_SPACE),
      IMMEDIATE_RESOURCE (resolution, Resolution, Dimension,
                          FONTVIEW_DEFAULT_RESOLUTION),
    };


/* Routines in the class record.  */
static void fontview_initialize (Widget, Widget, ArgList, Cardinal *);
static void fontview_resize (Widget);
static Boolean fontview_set_values (Widget, Widget, Widget, ArgList,
                                    Cardinal *);
static void fontview_destroy (Widget);

/* We can inherit most things in the instantiation of the class record.
   The operations `initialize', `set_values', and `destroy' are chained
   automatically by the toolkit, and need no `XtInherit...' constant.  */

FontviewClassRec fontviewClassRec
  = { /* Core class fields.  */
      { /* superclass	  	 */ (WidgetClass) &labelClassRec,
        /* class_name	  	 */ "Fontview",
        /* widget_size	  	 */ sizeof (FontviewRec),
        /* class_initialize   	 */ NULL,
        /* class_part_initialize */ NULL,
        /* class_inited       	 */ FALSE,
        /* initialize	  	 */ fontview_initialize,
        /* initialize_hook	 */ NULL,
        /* realize		 */ XtInheritRealize,
        /* actions		 */ fontview_actions,
        /* num_actions	  	 */ XtNumber (fontview_actions),
        /* resources	  	 */ fontview_resources,
        /* num_resources	 */ XtNumber (fontview_resources),
        /* xrm_class	  	 */ NULLQUARK,
        /* compress_motion	 */ TRUE,
        /* compress_exposure  	 */ TRUE,
        /* compress_enterleave	 */ TRUE,
        /* visible_interest	 */ FALSE,
        /* destroy		 */ fontview_destroy,
        /* resize		 */ fontview_resize,
        /* expose		 */ XtInheritExpose,
        /* set_values	  	 */ fontview_set_values,
        /* set_values_hook	 */ NULL,
        /* set_values_almost	 */ XtInheritSetValuesAlmost,
        /* get_values_hook	 */ NULL,
        /* accept_focus	 	 */ NULL,
        /* version		 */ XtVersion,
        /* callback_private   	 */ NULL,
        /* tm_table		 */ NULL,
        /* query_geometry	 */ XtInheritQueryGeometry,
        /* display_accelerator	 */ XtInheritDisplayAccelerator,
        /* extension		 */ NULL
      },

      /* Simple class fields.  */
      { /* change_sensitive */ XtInheritChangeSensitive },

      /* Label class fields.  */
      { 0 },

      /* Fontview class fields.  */
      { 0 }
    };

WidgetClass fontviewWidgetClass = (WidgetClass) &fontviewClassRec;



/* Class routines.  */

/* This routine is called at widget creation time by the toolkit, after
   our superclasses have been initialized.  REQUEST is the widget that
   was originally requested by the user; NEW is the widget that has been
   created by our superclasses in response to the requests.
   
   We want to tell our Label superclass that it should deal with a pixmap,
   not with a string.  */

#define DEPTH 1 /* No grayscale or color bitmaps.  */
#define HEIGHT_PERCENT .65 /* Percent of line_space above baseline.  */

static void
fontview_initialize (Widget request, Widget new, ArgList args,
                     Cardinal *n_args) 
{
  XGCValues gc_values;
  FontviewWidget fw = (FontviewWidget) new;
  Display *d = XtDisplay (fw);
  
  /* Assume the width and height resources in the label have been set to
     reflect how big the canvas is supposed to be.  */
  unsigned width = fw->core.width;
  unsigned height = fw->core.height;
  
  /* What we'll draw in.  */
  Pixmap pixmap
    = XCreatePixmap (d, DefaultRootWindow (d), width, height, DEPTH);
  
  Arg label_args[]
    = { { XtNbitmap, pixmap },
        { XtNheight, height },
        { XtNwidth, width },
      };

  if (pixmap == None)
    XtErrorMsg ("noPixmap", "FontviewWidget", "Error",
                "Can't allocate pixmap to show character", NULL, 0);

  XtSetValues (fw, XTARG (label_args));
  
  fw->fontview.current_char = NULL;
  fw->fontview.current_pos.x = 0;
  fw->fontview.current_pos.y = HEIGHT_PERCENT * fw->fontview.line_space;

  /* Leave the foreground as zero, since we always clear before we draw
     something.  */
  gc_values.background = 0L;
  fw->fontview.gc
    = XCreateGC (d, fw->label.pixmap, GCBackground, &gc_values);
  
  XFillRectangle (d, fw->label.pixmap, fw->fontview.gc, 0, 0, width, height);
}


/* When we are resized, we have to change the size of the pixmap.  */

static void
fontview_resize (Widget w)
{
  FontviewWidget fw = (FontviewWidget) w;
  Display *d = XtDisplay (fw);
  
  /* The width and height of W have been set.  */
  unsigned new_width = fw->core.width - fw->label.internal_width * 2;
  unsigned new_height = fw->core.height - fw->label.internal_height * 2;
  Pixmap pixmap;
  
  /* Get the old width and height of the Label's pixmap.  */
  unsigned old_width, old_height, bw, depth;
  Window root;
  int x, y;

  /* The Label's initialize procedure calls resize directly.  I don't
     know whether it should or not, but since it does, we might get
     called before the pixmap is set up.  If so, we have nothing to do.  */
  if (fw->label.pixmap != None)
    {
      assert (XGetGeometry (d, fw->label.pixmap, &root, &x, &y, &old_width,
                            &old_height, &bw, &depth));

      /* Since the SetValues call below winds up resizing the Label, we
         are going to be called again.  So only do any resizing if the
         size is actually different.  */
      if (new_width != old_width || new_height != old_height)
        {
          pixmap = XCreatePixmap (d, DefaultRootWindow (d), new_width,
                                  new_height, DEPTH);

          /* Copy the contents of the old pixmap to the new.  */
          XCopyArea (d, fw->label.pixmap, pixmap, fw->fontview.gc, 0, 0,
                     MIN (old_width, new_width), MIN (old_height, new_height),
                     0, 0);

          /* Free the old pixmap, and set the Label's pixmap to the new.  */
          XFreePixmap (d, fw->label.pixmap);
          XtVaSetValues (w, XtNbitmap, pixmap, NULL, NULL);
        }
    }
}


/* This routine is called when one of the resources in the widget
   changes; for example, in response to an XtSetValues call.  */

static Boolean
fontview_set_values (Widget current, Widget request, Widget new,
                     ArgList args, Cardinal *n_args)
{
  return False;
}


/* This routine is called when the widget is destroyed.  */

static void
fontview_destroy (Widget w)
{
  FontviewWidget fw = (FontviewWidget) w;
  
  XFreeGC (XtDisplay (fw), fw->fontview.gc);
}



/* These convenience procedures save clients the trouble of constructing
   an ArgList to get the resources in the widget.  (None yet.)  */



/* Action procedures.  */

/* Display the character specified in the (presumably Key) event EVENT.
   Kern, if appropriate, between that character and the previous current
   character (but don't do ligature processing, and don't worry about
   the ligtable ``cursor advancing'' commands).  Then set the new
   current character.  */

static void
self_insert (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  char_info_type bitmap_info;
  char_type *c;
  boolean found;
  int kern;
  Pixmap pixmap;
  int final_y;
  FontviewWidget fw = (FontviewWidget) w;
  Display *d = XtDisplay (fw);
  char_type **chars = (char_type **) fw->fontview.chars;
  char_type *current = fw->fontview.current_char;
  coordinate_type current_pos = fw->fontview.current_pos;
  font_type *font = (font_type *) fw->fontview.font_info;
  GC gc = fw->fontview.gc; /* Assume the function is GXclear.  */
  int test_x = fw->fontview.current_pos.x;
  unsigned height = fw->core.height;
  unsigned width = fw->core.width;
  charcode_type code = find_keycode (event, &found);
  
  /* If no character code in the key event, do nothing.  */
  if (!found)
    return;
    
  if (chars == NULL)
    XtErrorMsg ("noChars", "FontviewWidget", "Error",
                "Can't typeset a character with `chars' null", NULL, 0);

  /* We have to trust that `chars' really is an array that is long
     enough for all possible character codes.  */
  c = chars[code] ? : read_char (*font, code);
  
  /* If the character doesn't exist, return.  */
  if (c == NULL)
    return;

  bitmap_info = CHAR_BITMAP_INFO (*c);

  /* Figure out where we would be horizontally if we put this character
     on the same line as the current one.  */
  test_x += CHAR_SET_WIDTH (bitmap_info);
  kern = current == NULL
         ? 0.0 
         : POINTS_TO_PIXELS (tfm_get_kern (CHAR_TFM_INFO (*current), code),
                             fw->fontview.resolution);
  test_x += kern;

  /* If that's too far, put it on the next line.  */
  if (test_x >= width)
    {
      current_pos.x = 0;
      current_pos.y += fw->fontview.line_space;
      
      /* We've moved from the baseline of the old line (the one that's
         full) to the baseline of the new one (that we're about to start
         typesetting on).  Check if the descender fits in the window.  */
      if (current_pos.y + (1 - HEIGHT_PERCENT) * fw->fontview.line_space
          >= height)
        {
          /* Go back to the top of the window.  */
          current_pos.y = HEIGHT_PERCENT * fw->fontview.line_space;
          
          /* We want to set the pixmap to all zero bits, since we use
             XCopyPlane to do the drawing.  We assume the foreground
             pixel is zero when we come in.  */
          XFillRectangle (d, fw->label.pixmap, gc, 0, 0, width, height);
          
          /* Update the display if necessary.  */
          if (XtIsRealized (fw))
            XCopyPlane (d, fw->label.pixmap, XtWindow (fw),
                        fw->label.normal_GC,
                        0, 0, width, height, 0, 0, 1L);
        }
    }
  
  /* OK, draw the new character with its origin at the current position.  */
  current_pos.x += kern + CHAR_LSB (bitmap_info);

  pixmap = bitmap_to_pixmap (d, gc, CHAR_BITMAP (bitmap_info));

  /* Update the parent pixmap and, if necessary, the display.  */
  final_y = MAX (current_pos.y - CHAR_MAX_ROW (bitmap_info), 0);

  XCopyArea (d, pixmap, fw->label.pixmap, gc, 0, 0,
             CHAR_BITMAP_WIDTH (bitmap_info),
             CHAR_BITMAP_HEIGHT (bitmap_info),
             current_pos.x, final_y);
  if (XtIsRealized (fw))
    XCopyPlane (d, pixmap, XtWindow (fw), fw->label.normal_GC, 0, 0,
                CHAR_BITMAP_WIDTH (bitmap_info),
                CHAR_BITMAP_HEIGHT (bitmap_info),
                current_pos.x, final_y, 1L);

  XSetForeground (d, gc, 0L);
  XFreePixmap (d, pixmap);

  current_pos.x += CHAR_BITMAP_WIDTH (bitmap_info) + CHAR_RSB (bitmap_info);
  fw->fontview.current_pos = current_pos;
  fw->fontview.current_char = c;
}



/* Utilities.  */

/* Convert the bitmap B to a Pixmap on the display D, and return it.  We
   use the graphics context GC for drawing.  We assume its initial
   foreground is 0.  When we exit, it's 1.  */

static Pixmap
bitmap_to_pixmap (Display *d, GC gc, bitmap_type b)
{
  unsigned row;
  XSegment *save_segment_list;
  XSegment *segment_list = NULL;
  unsigned nsegments = 0;
  unsigned height = BITMAP_HEIGHT (b);
  unsigned width = BITMAP_WIDTH (b);
  Pixmap pixmap
    = XCreatePixmap (d, DefaultRootWindow (d), width, height, DEPTH);

  if (pixmap == None)
    XtErrorMsg ("noPixmap", "FontviewWidget", "Error",
                "Can't allocate pixmap to show character", NULL, 0);

  /* Clear the new pixmap to all zeros.  */
  XFillRectangle (d, pixmap, gc, 0, 0, width, height);
  
  /* Draw lines through consecutive black pixels on each scanline.  This
     is a simplification of the code in `gnuw/Bitmap.c'.  */
  for (row = 0; row < BITMAP_HEIGHT (b); row++)
    {
      unsigned start;
      boolean done = false;
      one_byte *row_data = BITMAP_ROW (b, row);
      unsigned *transitions = bitmap_find_transitions (row_data, width);
      unsigned *save_transitions = transitions;
      
      /* We must perform the test on `done' before dereferencing
         `transitions' in the increment step of the loop, because if we
         are indeed done, `transitions' may point to garbage.  */
      for (start = *transitions++;
           start != width + 1;
           start = done ? width + 1 : *transitions++)
        {
          XSegment *s;
          unsigned end = *transitions++;
          
          /* This test is true if the row ends with black.  */
          if (end == width + 1) 
            {
              end--;
              done = true;
            }
          
          XRETALLOC (segment_list, ++nsegments, XSegment);
          s = &segment_list[nsegments - 1];
          s->x1 = start;
          s->x2 = end - 1; /* `end' is at a white pixel; don't draw it.  */
          s->y1 = s->y2 = row;
        }
      free (save_transitions);
    }

  /* It's unlikely we'll ever exceed this, but just to be safe.  */
  #define MAX_NSEGMENTS 4000
  save_segment_list = segment_list;
  XSetForeground (d, gc, 1L);
  while (nsegments > MAX_NSEGMENTS)
    {
      XDrawSegments (d, pixmap, gc, segment_list, MAX_NSEGMENTS);
      nsegments -= MAX_NSEGMENTS;
      segment_list += MAX_NSEGMENTS;
    }
  XDrawSegments (d, pixmap, gc, segment_list, nsegments);
  
  free (save_segment_list);
  
  return pixmap;
}


/* Extract the character code of a pressed key from the event E.  I
   can't see any way to tell how big the buffer might be that
   XLookupString returns, so we just have to hope that what we allocate
   is enough.  We set FOUND if we actually found a character code, as
   opposed to just, say, a modifier key being pressed.  */

#define KEY_BUFFER_SIZE 128


static charcode_type
find_keycode (XEvent *e, boolean *found)
{
  char key_buffer[KEY_BUFFER_SIZE];
  charcode_type ret = 75; /* Default random return value.  */
  int length 
     = XLookupString (&e->xkey, key_buffer, KEY_BUFFER_SIZE, NULL, NULL);

  switch (length)
    {
    case 0:
      /* We get here if the user presses just a modifier key.  */
      *found = false;
      break;
      
    case 1:
      *found = true;
      ret = *key_buffer;
      break;
    
    default:
      XtErrorMsg ("strangeKey", "FontviewWidget", "Error",
                  "Can't handle multicharacter key string", NULL, 0);
    }

  return ret;
}
