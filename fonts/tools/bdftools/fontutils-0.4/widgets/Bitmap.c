/* Bitmap.c: implementation of a Bitmap widget as a subclass of Athena's
   Label widget.

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

#include "bitmap.h"

#include "BitmapP.h"

/* Says whether the rectangle we draw to represent the temporary
   selection is visible, i.e., if we've erased it.  */
static boolean selection_showing = false;

/* Utility routines.  */
static Pixmap bitmap_to_pixmap (Display *, bitmap_type, unsigned);
static void check_action_parameters (string, BitmapWidget, Cardinal, Cardinal);
static boolean find_bitmap_position (XEvent *, BitmapWidget, int *, int *);
static coordinate_type get_pointer_position (XEvent *);
static unsigned param_value (string, string []);
static void redraw_pixel (BitmapWidget, one_byte, long, int, int);

/* Subroutines for handling selections.  */
static bounding_box_type find_selection_bb (BitmapWidget);
static void redraw_selection_bitmap (BitmapWidget, bitmap_type);
static void redraw_selection (BitmapWidget, bitmap_type);
static void show_selection (BitmapWidget);
static void unshow_selection (BitmapWidget);
static void update_selection (BitmapWidget, unsigned, unsigned);



/* Action routines.  */
static action_proc_type
  invert_pixel, fill_selection,
  start_selection, adjust_selection, accept_selection,
  start_paste, move_paste, accept_paste;
                  
/* The mapping of action names to procedures.  */
static XtActionsRec bitmap_actions[]
  = { { "InvertPixel",	       	invert_pixel },
      { "FillSelection",	fill_selection },
      { "StartSelection",      	start_selection },
      { "AdjustSelection",     	adjust_selection },
      { "AcceptSelection",     	accept_selection },
      { "StartPaste",	       	start_paste },
      { "MovePaste",	       	move_paste },
      { "AcceptPaste",		accept_paste },
    };


/* This structure defines the default values of the resources specific
   to the Bitmap widget.  */
#define OFFSET(field) XtOffset (BitmapWidget, bitmap.field)
static XtResource bitmap_resources[]
  = { IMMEDIATE_RESOURCE (expansion, Expansion, Dimension,
                          BITMAP_DEFAULT_EXPANSION),
      IMMEDIATE_RESOURCE (bits, Bitmap, Pointer, NULL),
      IMMEDIATE_RESOURCE (modified, Modified, Boolean, False),
      IMMEDIATE_RESOURCE (shadow, Bitmap, Widget, NULL),
    };


/* Routines in the class record.  */
static void bitmap_initialize (Widget, Widget, ArgList, Cardinal *);
static Boolean bitmap_set_values (Widget, Widget, Widget, ArgList, Cardinal *);
static void bitmap_destroy (Widget);

/* We can inherit most things in the instantiation of the class record.
   The operations `initialize', `set_values', and `destroy' are chained
   automatically by the toolkit, and need no `XtInherit...' constant
   (Xt manual, p.21).  */
BitmapClassRec bitmapClassRec
  = { /* Core class fields.  */
      { /* superclass	  	 */ (WidgetClass) &labelClassRec,
        /* class_name	  	 */ "Bitmap",
        /* widget_size	  	 */ sizeof (BitmapRec),
        /* class_initialize   	 */ NULL,
        /* class_part_initialize */ NULL,
        /* class_inited       	 */ FALSE,
        /* initialize	  	 */ bitmap_initialize,
        /* initialize_hook	 */ NULL,
        /* realize		 */ XtInheritRealize,
        /* actions		 */ bitmap_actions,
        /* num_actions	  	 */ XtNumber (bitmap_actions),
        /* resources	  	 */ bitmap_resources,
        /* num_resources	 */ XtNumber (bitmap_resources),
        /* xrm_class	  	 */ NULLQUARK,
        /* compress_motion	 */ TRUE,
        /* compress_exposure  	 */ TRUE,
        /* compress_enterleave	 */ TRUE,
        /* visible_interest	 */ FALSE,
        /* destroy		 */ bitmap_destroy,
        /* resize		 */ XtInheritResize,
        /* expose		 */ XtInheritExpose,
        /* set_values	  	 */ bitmap_set_values,
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

      /* Bitmap class fields.  */
      { 0 }
    };

WidgetClass bitmapWidgetClass = (WidgetClass) &bitmapClassRec;



/* Class routines.  */

/* This routine is called at widget creation time by the toolkit, after
   our superclasses have been initialized.  REQUEST is the widget that
   was originally requested by the user; NEW is the widget that has been
   created by our superclasses in response to the requests.
   
   We want to tell our Label superclass that it should deal with a pixmap,
   not with a string.
   
   We do not use the parameters ARGS and N_ARGS.  */

static void
bitmap_initialize (Widget request, Widget new, ArgList args, Cardinal *n_args)
{
  XGCValues select_gc_values;
  BitmapWidget old = xmalloc (sizeof (*old));
  BitmapWidget bw = (BitmapWidget) new;
  bitmap_type *bitmap_ptr = (bitmap_type *) bw->bitmap.bits;
  Arg bitmap_args[] = { { XtNbits, (XtArgVal) bitmap_ptr } };
  Cardinal n = XtNumber (bitmap_args);
  Display *d = XtDisplay (bw);

  if (bitmap_ptr == NULL)
    XtErrorMsg ("noData", "bitmapCreate", "BitmapError",
                "No bitmap specified.", NULL, 0);

  /* Convert the bitmap to a pixmap.  We have to make sure that we
     really do create the pixmap here, by forcing the old and new
     bitmaps to be different.  */
  *old = *(BitmapWidget) request;
  old->bitmap.bits = NULL;
  (void) bitmap_set_values ((Widget) old, request, new, bitmap_args, &n);

  /* Make GC's to use in editing the bits and doing the selection.  */
  bw->bitmap.edit_gc = XCreateGC (XtDisplay (bw), bw->label.pixmap, 0, NULL);
  select_gc_values.function = GXxor;
  select_gc_values.foreground = AllPlanes;
  bw->bitmap.select_gc = XCreateGC (d, bw->label.pixmap,
                                    GCFunction | GCForeground,
                                    &select_gc_values);

  /* If we have a shadow bitmap, we want to use the same GC and bitmap
     for it as we do for the current one.  We should probably do this in
     bitmap_set_values somewhere, also.  */
  if (bw->bitmap.shadow != NULL)
    {
      BitmapWidget shadow_bw = (BitmapWidget) bw->bitmap.shadow;
      XFreeGC (d, shadow_bw->bitmap.edit_gc);
      shadow_bw->bitmap.edit_gc = bw->bitmap.edit_gc;
    }

  /* Initially, we're unmodified, and have no selection.  */
  bw->bitmap.modified = false;
  bw->bitmap.selection = NULL;
}


/* This routine is called when one of the resources in the widget
   changes; for example, in response to an XtSetValues call.  It is also
   called by the `bitmap_initialize' routine.  The toolkit has already
   modified our resources.  CURRENT is the widget before any resources
   were changed; REQUEST is the widget before any class `set_values'
   procedures have been called; NEW is the widget as updated by the
   superclasses.

   We do not use the parameters ARGS and N_ARGS.  */

static Boolean
bitmap_set_values (Widget current, Widget request, Widget new,
                   ArgList args, Cardinal *n_args)
{
  BitmapWidget old_bw = (BitmapWidget) current;
  bitmap_type *old_bitmap_ptr = (bitmap_type *) old_bw->bitmap.bits;
  unsigned old_expansion = old_bw->bitmap.expansion;
  BitmapWidget bw = (BitmapWidget) new;
  bitmap_type *bitmap_ptr = (bitmap_type *) bw->bitmap.bits;
  unsigned expansion = bw->bitmap.expansion;

  if (bitmap_ptr == NULL)
    XtErrorMsg ("noData", "bitmapSetValues", "BitmapError",
                "No bitmap specified.", NULL, 0);

  if (bitmap_ptr == old_bitmap_ptr && expansion == old_expansion)
    return False; /* No redisplay needed.  */

  {
    Display *display = XtDisplay (bw);

    /* Our new width and height, in window coordinates.  */
    unsigned w = BITMAP_WIDTH (*bitmap_ptr) * expansion;
    unsigned h = BITMAP_HEIGHT (*bitmap_ptr) * expansion;

    /* The new image.  */
    Pixmap pixmap = bitmap_to_pixmap (display, *bitmap_ptr, expansion);

    Arg label_args[]
      = { { XtNbitmap,	pixmap },
          { XtNwidth,	w },
          { XtNheight,	h },
        };

    /* If we were already displaying a pixmap, free it.  Perhaps it
       would be better to keep it around, in case the user goes back
       to this character.  */
    if (bw->label.pixmap != None)
      XFreePixmap (display, bw->label.pixmap);

    /* The bitmap has changed, so we should clear the `modified' field.  */
    bw->bitmap.modified = false;

    /* Make the changes.  */
    XtSetValues ((Widget) bw, XTARG (label_args));

    /* We want to be redisplayed now.  */
    return True;
  }
}


/* This routine is called when the widget is destroyed.  We deallocate
   the GC resources we have explicitly created.  */

static void
bitmap_destroy (Widget w)
{
  BitmapWidget bw = (BitmapWidget) w;
  
  XFreeGC (XtDisplay (bw), bw->bitmap.edit_gc);
  XFreeGC (XtDisplay (bw), bw->bitmap.select_gc);
}  



/* These convenience procedures save clients the trouble of constructing
   an ArgList to get the resources in the Bitmap widget.  */

unsigned
BitmapExpansion (Widget w)
{
  BitmapWidget bw = (BitmapWidget) w;
  return bw->bitmap.expansion;
}


bitmap_type *
BitmapBits (Widget w)
{
  BitmapWidget bw = (BitmapWidget) w;
  return (bitmap_type *) bw->bitmap.bits;
}


Boolean
BitmapModified (Widget w)
{
  BitmapWidget bw = (BitmapWidget) w;
  return bw->bitmap.modified;
}



/* This utility routine converts the bitmap B into a Pixmap on the
   display DISPLAY.  Each pixel in B becomes EXPANSION pixels in the
   Pixmap.  If the pixmap creation fails, it gives a fatal error.  */

#define DEPTH 1  /* We don't need grayscale or color bitmaps.  */

static Pixmap
bitmap_to_pixmap (Display *display, bitmap_type b, unsigned expansion)
{
  GC gc;
  XGCValues gc_values;
  unsigned row;
  XRectangle *save_rectangle_list;
  XRectangle *rectangle_list = NULL;
  unsigned nrectangles = 0;
  
  unsigned height = BITMAP_HEIGHT (b) * expansion;
  unsigned w = BITMAP_WIDTH (b);
  unsigned width = BITMAP_WIDTH (b) * expansion;
  
  /* We need a drawable to create the pixmap on.  Since we know we have
     a root window, we don't have to be passed one.  */
  Window root_window = DefaultRootWindow (display);
  Pixmap pixmap = XCreatePixmap (display, root_window, width, height, DEPTH);

  if (pixmap == None)
    FATAL ("bitmap_to_pixmap: Could not allocate pixmap");

  /* Start with the foreground zero (which is the default), since we
     want to clear the pixmap before writing anything.  We want the
     background zero so that the gc will correspond to the bitmap.  */
  gc_values.background = 0;
  gc = XCreateGC (display, pixmap, GCBackground, &gc_values);

  /* Write the bitmap to the pixmap.  We want the one bits in the bitmap
     to end up as one bits in the pixmap, regardless of the actual
     foreground and background pixels in the application.  Why?  Because
     we (and the Label widget) use XCopyPlane to draw the pixmap, and
     XCopyPlane turns one bits into the foreground color, and zero bits
     in the background color.  */
  XFillRectangle (display, pixmap, gc, 0, 0, width, height);
  XSetForeground (display, gc, 1L);

  /* Instead of creating a separate rectangle for each pixel, we
     combine adjacent pixels in each scanline into one rectangle.  We
     could go further, and combine identical adjacent scanlines
     (analysis has shown this happens about 1/3 of the time).  Or we
     could go further still, and use some theory about decomposition
     into a minimal set of rectangles, as cited in `Rectangular
     Convolution for Fast Filtering of Characters', by Avi Naiman and
     Alain Fournier, in the July 1987 SIGGRAPH Proceedings.  But doing
     this simple job seems to make it fast enough.  */
  for (row = 0; row < BITMAP_HEIGHT (b); row++)
    {
      unsigned start;
      boolean done = false;
      one_byte *row_data = BITMAP_ROW (b, row);
      unsigned *transitions = bitmap_find_transitions (row_data, w);
      unsigned *save_transitions = transitions;
      
      /* We must perform the test on `done' before dereferencing
         `transitions' in the increment step of the loop, because if we
         are indeed done, `transitions' may point to garbage.
         bitmap_find_transitions guarantees an even number of
         transitions.  */
      for (start = *transitions++; start != w + 1;
           start = done ? w + 1 : *transitions++)
        {
          XRectangle *r;
          unsigned end = *transitions++;

          XRETALLOC (rectangle_list, ++nrectangles, XRectangle);
          r = &rectangle_list[nrectangles - 1];
          r->x = start * expansion;
          r->y = row * expansion;
          r->width = (end - start) * expansion;
          r->height = expansion;
        }
      free (save_transitions);
    }

  /* The protocol puts some limit on the number of rectangles that can
     be filled in a single request.  It is not uncommon for us to have
     thousands of rectangles in the list, either because of a
     large font (e.g., cminch), or a high resolution.  */
  #define MAX_NRECTANGLES 4000
  save_rectangle_list = rectangle_list;
  while (nrectangles > MAX_NRECTANGLES)
    {
      XFillRectangles (display, pixmap, gc, rectangle_list, MAX_NRECTANGLES);
      nrectangles -= MAX_NRECTANGLES;
      rectangle_list += MAX_NRECTANGLES;
    }
  XFillRectangles (display, pixmap, gc, rectangle_list, nrectangles);
  XFreeGC (display, gc);
  
  free (save_rectangle_list);
  
  return pixmap;
}


/* Redraw the pixel at (COL,ROW) in the bitmap widget BW.  Update the
   pixmap in the widget, and, if realized, the window.  */ 

static void
redraw_pixel (BitmapWidget bw, one_byte bitmap_color,
              long foreground_color, int row, int col)
{
  static long last_foreground = 0; /* XCreateGC's default.  */
  Display *display = XtDisplay (bw);
  unsigned expansion = bw->bitmap.expansion;
  int expanded_row = row * expansion,
      expanded_col = col * expansion;
  bitmap_type *bitmap = (bitmap_type *) bw->bitmap.bits;
  Pixmap label_pixmap = bw->label.pixmap;

  /* Update the bitmap structure.  */
  BITMAP_PIXEL (*bitmap, row, col) = bitmap_color;

  /* Only set the foreground if it has changed, for efficiency.  */
  if (foreground_color != last_foreground)
    {
      XSetForeground (display, bw->bitmap.edit_gc, foreground_color);
      last_foreground = foreground_color;
    }
  XFillRectangle (display, label_pixmap, bw->bitmap.edit_gc,
                  expanded_col, expanded_row, expansion, expansion);
  bw->bitmap.modified = true;
  
  /* We may as well call XCopyPlane ourselves, instead of generating an
     Expose event, since we have all the information necessary.  */
  if (XtIsRealized ((Widget) bw))
    XCopyPlane (display, label_pixmap, XtWindow (bw), bw->label.normal_GC,
                expanded_col, expanded_row, expansion, expansion,
                expanded_col, expanded_row, 1L);
}


/* Return the position of the pointer as recorded in EVENT.  */

static coordinate_type
get_pointer_position (XEvent *event)
{
  coordinate_type c;
  
  switch (event->type)
    {
    case MotionNotify:
      c.x = event->xmotion.x;
      c.y = event->xmotion.y;
      break;
    
    case ButtonPress:
    case ButtonRelease:
      c.x = event->xbutton.x;
      c.y = event->xbutton.y;
      break;
    
    case KeyPress:
    case KeyRelease:
      c.x = event->xkey.x;
      c.y = event->xkey.y;
      break;
    
    case EnterNotify:
    case LeaveNotify:
      c.x = event->xcrossing.x;
      c.y = event->xcrossing.y;
      break;
    
    default:
      c.x = 0;
      c.y = 0;
    }
  
  return c;
}


/* If SEEN != EXPECTED, or if there is no bitmap in BW to operate on,
   report an error for the action NAME.  */

static void
check_action_parameters (string name, BitmapWidget bw, Cardinal seen,
                         Cardinal expected)
{
  if (bw->bitmap.bits == NULL || bw->label.pixmap == None)
    XtErrorMsg ("noData", name, "BitmapError",
                "No bitmap specified", NULL, 0);
  
  if (seen != expected)
    {
      string error_params[] = { itoa (expected), itoa (seen) };
      unsigned n_error_params = XtNumber (error_params);
      XtErrorMsg ("invalidParameters", name, "BitmapError",
                  "Expected %s argument(s), found %s",
                  error_params, &n_error_params);
  }
}    


/* If V is not one of the elements in VALUE_LIST, abort with an error
   message.  Otherwise, return the (zero-based) index of V in the list.
   VALUE_LIST should end with an element that is NULL.  */

static unsigned
param_value (string v, string value_list[])
{
  unsigned ret = 0;
  
  while (value_list[ret] != NULL && !STREQ (value_list[ret], v))
    ret++;

  /* Did we find it?  */
  if (value_list[ret] == NULL)
    {
      unsigned this_value;
      unsigned n_error_params = 2;
      string error_params[n_error_params];
      
      error_params[0] = *value_list;
      for (this_value = 1; this_value < ret; this_value++)
        {
          string temp = error_params[0];
          error_params[0] = concat3 (error_params[0], ",",
                                     value_list[this_value]);
          free (temp);
        }
      
      error_params[1] = v;

      XtErrorMsg ("invalidParameter", "acceptPaste", "BitmapError",
                  "Expected one of (%s), found `%s'",
                  error_params, &n_error_params);
    }
  
  return ret;
}


/* Given a position in window coordinates, we sometimes need to
   translate it to an expanded pixel boundary.  We want to do this via
   rounding, instead of truncating, so that the user can select the
   pixels in the rightmost column and bottommost row of the bitmap.
   
   On the other hand, sometimes we want to truncate the position,
   instead of rounding, so that the starting positions are intuitive.
   
   We assume that the variable `bw' points to a bitmap widget.  */

#define EXPANSION_ROUND(n)						\
  (bw->bitmap.expansion							\
   * (((n) + bw->bitmap.expansion - 1) / bw->bitmap.expansion))

#define EXPANSION_TRUNC(n)						\
  (bw->bitmap.expansion * ((n) / bw->bitmap.expansion))


/* Return whether the cursor position in E is within the bitmap in BW.
   We assume that the bitmap is non-null.  In any case, return the row
   and column in window coordinates in ROW and COL.  */

static boolean
find_bitmap_position (XEvent *e, BitmapWidget bw, int *row, int *col)
{
  coordinate_type pos = get_pointer_position (e);
  bitmap_type b = *(bitmap_type *) bw->bitmap.bits;
  
  /* Convert from the window coordinates to the pixel in the bitmap.  */
  int bitmap_row = pos.y / bw->bitmap.expansion;
  int bitmap_col = pos.x / bw->bitmap.expansion;
  
  *row = pos.y;
  *col = pos.x;

  /* Return whether the row and column are outside the boundaries of the
     bitmap.  */
  return pos.y >= 0 && bitmap_row < BITMAP_HEIGHT (b)
    && pos.x >= 0 && bitmap_col < BITMAP_WIDTH (b);
}



/* Action procedures.  */

/* N_PARAMS should be exactly one, and PARAMS should point to either the
   string `Continuous' or `Discrete' (case is significant).  If the
   latter, we simply invert the pixel at the current mouse location, as
   specified in EVENT.  If the former, we only do the inversion if we
   are at a different pixel on this call than we were on the previous
   one.  */

static void
invert_pixel (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  static int last_row = -1; /* Expanded pixel position of the last event.  */
  static int last_col = -1;

  /* This is the color of the pixel at the pointer position at the time
     of the last `Discrete' call.  */
  static one_byte source_pixel;

  int row, col;
  
  one_byte target_color;
  long foreground_color;
  
  BitmapWidget bw = (BitmapWidget) w;
  bitmap_type *bitmap = (bitmap_type *) bw->bitmap.bits;
  BitmapWidget shadow_bw = (BitmapWidget) bw->bitmap.shadow;

  check_action_parameters ("invertPixel", bw, *n_params, 1);
  if (!find_bitmap_position (event, bw, &row, &col))
    return;
  
  row /= bw->bitmap.expansion;
  col /= bw->bitmap.expansion;

  if (STREQ (*params, "Continuous"))  
    { /* If we are still within the same (expanded) pixel, do nothing.  */
      if (col == last_col && row == last_row)
        return;

      /* We've moved, so do the inversion.  */
      last_col = col;
      last_row = row;
    }
  else if (STREQ (*params, "Discrete"))
    {
      /* Update `last_row' and `last_col' here, because it is easy to
         jiggle (inadvertently) the mouse a little after clicking.  The
         jiggling will cause a motion event that will invoke us again with
         `Continuous' as the parameter.  If we didn't update the current
         position here, that invocation would invert the pixel again.  */
      last_col = col;
      last_row = row;
      source_pixel = BITMAP_PIXEL (*bitmap, row, col);
    }
  else
    {
      unsigned n_error_params = 1;
      string error_params[1] = { *params };
      XtErrorMsg ("invalidParameter", "invertPixel", "BitmapError",
                  "Expected `Continuous' or `Discrete', found `%s'",
                  error_params, &n_error_params);
    }

  /* See comments in `bitmap_to_pixmap' for why the constants 1 and 0
     are appropriate here.  We depend on getting a `Discrete' event
     before any `Continuous' events; otherwise, `target_color' might be
     garbage.
     
     We are not testing the color of the pixel at the current
     position here.  Why?  Because we want to change pixels on
     `Continuous' events to a single color, not inverting them.  See
     `Bitmap.h' for more discussion of this.  */
  if (source_pixel == WHITE)
    { /* Make it black.  */
      target_color = BLACK;
      foreground_color = 1;
    }
  else
    { /* Make it white.  */
      target_color = WHITE;
      foreground_color = 0;
    }
  
  /* Update ourselves.  */
  redraw_pixel (bw, target_color, foreground_color, row, col);

  /* Update our shadow bitmap, if we have one.  */
  if (shadow_bw != NULL)
    redraw_pixel (shadow_bw, target_color, foreground_color, row, col);
}


/* Fill the selection in the bitmap widget W with the color of the pixel
   the pointer is on, as recorded in EVENT.  N_PARAMS should be zero.  */ 
   
static void
fill_selection (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  int row, col; /* The coordinates in the bitmap.  */
  BitmapWidget bw = (BitmapWidget) w;

  check_action_parameters ("fillSelection", bw, *n_params, 0);
  if (bw->bitmap.selection != NULL
      && bw->bitmap.select_width > 0 && bw->bitmap.select_height > 0
      && find_bitmap_position (event, bw, &row, &col))
    {
      bitmap_type new;
      bitmap_type *bitmap = (bitmap_type *) bw->bitmap.bits;
      one_byte color = BITMAP_PIXEL (*bitmap, row / bw->bitmap.expansion,
                                     col / bw->bitmap.expansion);
      bounding_box_type select_bb = find_selection_bb (bw);

      for (row = MIN_ROW (select_bb); row <= MAX_ROW (select_bb); row++)
        for (col = MIN_COL (select_bb); col < MAX_COL (select_bb); col++)
          BITMAP_PIXEL (*bitmap, row, col) = color;
      
      bw->bitmap.modified = true;

      new = extract_subbitmap (*bitmap, select_bb);
      redraw_selection (bw, new);
      free_bitmap (&new);
    }
}


/* Prepare to select a rectangle whose upper left corner is the pixel
   enclosing the position recorded in EVENT.  N_PARAMS should be zero.  */

static void
start_selection (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  int row, col; /* The coordinates in the bitmap.  */
  BitmapWidget bw = (BitmapWidget) w;

  check_action_parameters ("startSelection", bw, *n_params, 0);
  if (find_bitmap_position (event, bw, &row, &col))
    {
      /* Set the upper left corner and initialize the lower right corner to
         the current position.  */
      bw->bitmap.initial_select.x = bw->bitmap.select_ul.x =
        EXPANSION_TRUNC (col);
      bw->bitmap.initial_select.y = bw->bitmap.select_ul.y = 
        EXPANSION_TRUNC (row);
      bw->bitmap.select_width = bw->bitmap.select_height = 0;
    }
}


/* Take the point in EVENT as the new second point to define the
   selection.  */

static void
adjust_selection (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  int row, col; /* The coordinates in the bitmap.  */
  BitmapWidget bw = (BitmapWidget) w;

  check_action_parameters ("adjustSelection", bw, *n_params, 0);
  unshow_selection (bw);

  if (find_bitmap_position (event, bw, &row, &col))
    { 
      update_selection (bw, row, col);
      show_selection (bw);
      selection_showing = true;
    }
}


/* Take the point in EVENT to be the definitive second point for the
   selection.  We therefore can now set the (sub)bitmap that the selected
   rectangle defines, for pasting.  */

static void
accept_selection (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  int row, col; /* The coordinates in the bitmap.  */
  BitmapWidget bw = (BitmapWidget) w;

  check_action_parameters ("acceptSelection", bw, *n_params, 0);
  unshow_selection (bw);

  /* If the selection is empty, do nothing.  Do not clear out the
     previous selection, even.  */
  if (bw->bitmap.select_width > 0 && bw->bitmap.select_height > 0
      && find_bitmap_position (event, bw, &row, &col))
    {
      bounding_box_type select_bb;

      /* Set the final coordinates.  */
      update_selection (bw, row, col);

      /* Free the old bitmap.  */
      if (bw->bitmap.selection != NULL)
        free_bitmap (bw->bitmap.selection);

      /* Make the new one.  */
      select_bb = find_selection_bb (bw);
      bw->bitmap.selection = xmalloc (sizeof (bitmap_type));
      *(bw->bitmap.selection)
        = extract_subbitmap (*(bitmap_type *) bw->bitmap.bits, select_bb);
    }
}


/* If something has been selected, show its bounding rectangle with the
   upper left corner at the position in EVENT (so the user can see where
   the paste will happen).  */

static void
start_paste (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  int row, col; /* The coordinates in the bitmap.  */
  BitmapWidget bw = (BitmapWidget) w;

  check_action_parameters ("startPaste", bw, *n_params, 0);
  /* We obviously can't paste anything if nothing has been selected.
     Perhaps it would be better to show a warning message in that case,
     but for now we just do nothing.  */
  if (bw->bitmap.selection != NULL
      && find_bitmap_position (event, bw, &row, &col))
    {
      bw->bitmap.select_ul.x = EXPANSION_TRUNC (col);
      bw->bitmap.select_ul.y = EXPANSION_TRUNC (row);
      show_selection (bw);
      selection_showing = true;
    }
}


/* Update where the paste rectangle is according to EVENT.  */

static void
move_paste (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  int row, col; /* The coordinates in the bitmap.  */
  BitmapWidget bw = (BitmapWidget) w;

  check_action_parameters ("movePaste", bw, *n_params, 0);
  unshow_selection (bw);

  if (bw->bitmap.selection != NULL
      && find_bitmap_position (event, bw, &row, &col))
    {
      bw->bitmap.select_ul.x = EXPANSION_TRUNC (col);
      bw->bitmap.select_ul.y = EXPANSION_TRUNC (row);
      show_selection (bw); /* Show the new.  */
      selection_showing = true;
    }
}


/* Take the position in EVENT as the place to paste the selection, and
   update the bitmap.  Either ignore or keep the black in the bitmap
   underneath the paste according to first element of PARAMS, which must
   be either `Opaque' or `Transparent'.  We flip the bitmap vertically,
   horizontally, both, or neither according to the second parameter
   being `FlipVertical', `FlipHorizontal', `FlipBoth', or `FlipNeither'.  */

static void
accept_paste (Widget w, XEvent *event, String *params, Cardinal *n_params) 
{
  coordinate_type paste; /* The coordinates in the bitmap.  */
  int row, col;
  BitmapWidget bw = (BitmapWidget) w;

  check_action_parameters ("acceptPaste", bw, *n_params, 2);
  unshow_selection (bw);

  if (bw->bitmap.selection != NULL
      && find_bitmap_position (event, bw, &row, &col))
    {
      enum { flip_neither, flip_horizontal, flip_vertical, flip_both } flip;
      boolean opaque;
      unsigned source_row, source_col;
      unsigned target_row, target_col;
      unsigned this_row, this_col;
      string param1_values[]
        = { "Transparent", "Opaque", NULL };
      string param2_values[]
        = { "FlipNeither", "FlipHorizontal", "FlipVertical", "FlipBoth",
            NULL };

      bitmap_type source = *(bw->bitmap.selection);
      unsigned source_height = BITMAP_HEIGHT (source),
               source_width = BITMAP_WIDTH (source);
      bitmap_type *target = (bitmap_type *) bw->bitmap.bits;
      unsigned target_height = BITMAP_HEIGHT (*target),
               target_width = BITMAP_WIDTH (*target);
      
      /* Get the parameters we're passed.  */
      opaque = param_value (params[0], param1_values);
      flip = param_value (params[1], param2_values);

      /* Figure out where we are.  */
      paste.x = col / bw->bitmap.expansion;
      paste.y = row / bw->bitmap.expansion;

      /* Update the underlying bitmap.  Copy the black pixels from
         the original if we're pasting transparently.  */
      for (this_row = 0, target_row = paste.y;
           this_row < source_height && target_row < target_height; 
           this_row++, target_row++)
        {
          source_row = (flip == flip_horizontal || flip == flip_neither)
                       ? this_row 
                       : source_height - this_row - 1;

          for (this_col = 0, target_col = paste.x;
               this_col < source_width && target_col < target_width;
               this_col++, target_col++)
            {
              source_col = (flip == flip_vertical || flip == flip_neither)
                           ? this_col
                           : source_width - this_col - 1;
                           
              BITMAP_PIXEL (*target, target_row, target_col)
                = BITMAP_PIXEL (source, source_row, source_col)
                  | (opaque
                     ? WHITE
                     : BITMAP_PIXEL (*target, target_row, target_col));
            }
        }

      bw->bitmap.modified = true;

      /* If necessary, get the combined selection image back.  */
      if (!opaque || flip != flip_neither)
        {
          bounding_box_type select_bb = find_selection_bb (bw);
          source = extract_subbitmap (*target, select_bb);
        }

      redraw_selection (bw, source);

      if (!opaque)
        free_bitmap (&source);
    }
}



/* Convert the selection, which is in window coordinates, to a bounding
   box, in bitmap coordinates.  */

static bounding_box_type
find_selection_bb (BitmapWidget bw)
{
  bounding_box_type select_bb;
  unsigned expansion = bw->bitmap.expansion;
  
  MIN_COL (select_bb) = bw->bitmap.select_ul.x / expansion;
  MIN_ROW (select_bb) = bw->bitmap.select_ul.y / expansion;
  MAX_COL (select_bb)
    = MIN_COL (select_bb) + bw->bitmap.select_width / expansion;
  MAX_ROW (select_bb)
    = MIN_ROW (select_bb) + bw->bitmap.select_height / expansion;

  /* Because of the asymmetry of bounding boxes, we must
     subtract one from the row.  */
  MAX_ROW (select_bb)--;
  
  return select_bb;
}

          
/* Redraw the area in the bitmap widget BW given by the selection.  The
   new appearance of the area is given as a bitmap in SOURCE.  Also
   redraw the shadow bitmap, if it exists.  */

#define COMPUTE_SHADOW_COORD(field)					\
  shadow_bw->bitmap.field = shadow_bw->bitmap.expansion			\
                            * bw->bitmap.field / bw->bitmap.expansion;

static void
redraw_selection (BitmapWidget bw, bitmap_type source)
{
  BitmapWidget shadow_bw = (BitmapWidget) bw->bitmap.shadow;

  redraw_selection_bitmap (bw, source);

  if (shadow_bw != NULL)
    {
      COMPUTE_SHADOW_COORD (select_ul.x);
      COMPUTE_SHADOW_COORD (select_ul.y);
      COMPUTE_SHADOW_COORD (select_width);
      COMPUTE_SHADOW_COORD (select_height);
      redraw_selection_bitmap (shadow_bw, source);
    }
}

static void
redraw_selection_bitmap (BitmapWidget bw, bitmap_type source)
{
  Display *display = XtDisplay (bw);
  Pixmap selection_pixmap = bitmap_to_pixmap (display, source,
                                              bw->bitmap.expansion);

  /* Update the pixmap in our parent label.  */
  XCopyArea (XtDisplay (bw), selection_pixmap, bw->label.pixmap,
             bw->label.normal_GC, 0, 0, 
             bw->bitmap.select_width, bw->bitmap.select_height,
             bw->bitmap.select_ul.x, bw->bitmap.select_ul.y);
  XFreePixmap (display, selection_pixmap);

  /* Update the display.  */
  if (XtIsRealized ((Widget) bw))
    /* Server bug?  On my machine, this doesn't always update large pastes
       correctly; some parts of the window are left.  */
#if 0
    XCopyPlane (display, bw->label.pixmap, XtWindow (bw), 
                bw->label.normal_GC, 
                bw->bitmap.select_ul.x, bw->bitmap.select_ul.y,
                bw->bitmap.select_width, bw->bitmap.select_height,
                bw->bitmap.select_ul.x, bw->bitmap.select_ul.y, 1L);
#endif
    XClearArea (display, XtWindow (bw),
                bw->bitmap.select_ul.x, bw->bitmap.select_ul.y,
                bw->bitmap.select_width, bw->bitmap.select_height,
                True); /* Generate Expose event.  */
}


/* If we are realized, display the currently selected rectangle.  */

static void
show_selection (BitmapWidget bw)
{
  if (XtIsRealized ((Widget) bw) != None)
    XDrawRectangle (XtDisplay (bw), XtWindow (bw), bw->bitmap.select_gc, 
                    bw->bitmap.select_ul.x, bw->bitmap.select_ul.y,
                    bw->bitmap.select_width, bw->bitmap.select_height);
}


/* If the user moves off the bitmap, we need to erase the last
   selection rectangle we drew.  */

static void
unshow_selection (BitmapWidget bw)
{
  if (selection_showing)
    {
      show_selection (bw);
      selection_showing = false;
    }
}


/* Do the appropriate maneuvers for the new row ROW and column COL for
   computing the upper left corner of the selection.  These numbers are
   in unrounded window coordinates, i.e., they may not lie on pixel
   boundaries. If we think of the initial selection point as the origin,
   the new point can be in any of the four quadrants.  */

#define initial_x (bw->bitmap.initial_select.x)
#define initial_y (bw->bitmap.initial_select.y)

static void
update_selection (BitmapWidget bw, unsigned row, unsigned col)
{
  if (col < initial_x)
    {
      bw->bitmap.select_ul.x = EXPANSION_TRUNC (col);
      bw->bitmap.select_width = EXPANSION_ROUND (initial_x - col);
    }
  else
    {
      bw->bitmap.select_ul.x = initial_x;
      bw->bitmap.select_width = EXPANSION_ROUND (col - initial_x);
    }

  if (row < initial_y)
    {
      bw->bitmap.select_ul.y = EXPANSION_TRUNC (row);
      bw->bitmap.select_height = EXPANSION_ROUND (initial_y - row);
    }
  else
    {
      bw->bitmap.select_ul.y = initial_y;
      bw->bitmap.select_height = EXPANSION_ROUND (row - initial_y);
    }
}
