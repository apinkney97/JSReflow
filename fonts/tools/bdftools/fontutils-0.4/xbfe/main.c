/* xbfe -- a bitmap font editor for X11.

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
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Viewport.h>

#include "Bitmap.h"
#include "cmdline.h"
#include "filename.h"
#include "font.h"
#include "getopt.h"
#include "Item.h"

#include "char.h"
#include "commands.h"
#include "kern.h"
#include "main.h"

/* The resolution of the font we will read, in pixels per inch.  We
   don't deal with nonsquare pixels.  (-dpi)  */
string dpi = "300";

/* The representation of the font we're working on.  We depend on the
   character pointers all being initialized to NULL.  */
static font_type the_font;



static void create_widgets (Widget, char_type *);
static string read_command_line (int, string[]);
static Boolean update_position (XtPointer);




void
main (int argc, string argv[])
{
  unsigned this_char;
  char_type *c;
  /* We parse more options with getopt in `read_command_line', below.  */
  XrmOptionDescRec x_options[]
    = { { "-expansion", "*expansion", XrmoptionSepArg, NULL },
        { "--expansion", "*expansion", XrmoptionSepArg, NULL },
        { "+expansion", "*expansion", XrmoptionSepArg, NULL },
      };
  Widget top_level = XtInitialize (NULL, CLASS_NAME, XTARG (x_options),
                                   &argc, argv);

  FONT_NAME (the_font) = read_command_line (argc, argv);
  FONT_DPI (the_font) = atof (dpi);

  /* Find the font before creating any widgets, so if the font isn't
     found, we don't waste time.  xx need to only get_bitmap_font if
     there is no tfm file */
  FONT_INFO (the_font) = get_font (FONT_NAME (the_font), atou (dpi));
  
  /* Find the first character in the font to use for the current
     character, if the current character is set to something besides
     zero, or if the current character doesn't exist.  */
  c = read_char (the_font, FONT_CURRENT_CHARCODE (the_font));

  if (c == NULL)
    {
      for (this_char = 0; this_char <= MAX_CHARCODE && c == NULL; this_char++)
        c = read_char (the_font, this_char);

      if (c == NULL)
        FATAL1 ("xbfe: No characters in the font `%s'", FONT_NAME (the_font));

      FONT_CURRENT_CHARCODE (the_font) = this_char;
    }

  FONT_CURRENT_CHAR (the_font) = c;

  /* Make a copy of the first bitmap so the original isn't changed.  */
  BCHAR_BITMAP (*c) = copy_bitmap (BCHAR_BITMAP (*c));

  /* OK, we have something to edit.  Initialize the other fields of the
     font structure.  */
  FONT_MODIFIED (the_font) = false;
  FONT_DPI (the_font) = atof (dpi);
  
  /* I don't remember why this is done.  */
  TFM_CHECKSUM (FONT_TFM_FONT (FONT_INFO (the_font))) = 0;
  
  create_widgets (top_level, FONT_CURRENT_CHAR (the_font));
  
  XtRealizeWidget (top_level);

  XtMainLoop ();
}



/* Create the widget hierarchy underneath TOP.  The bitmap to be
   initially displayed is INITIAL_BITMAP.  We also use the global
   `the_font', and set the widgets in that structure.  */

/* Will we ever need side bearings larger than 999 or -99 pixels?  */
#define MAX_SB_LENGTH 3

static void
create_widgets (Widget top, char_type *initial_char)
{
  Widget form_widget;

  /* The basic commands for the user.  These are strung along the top of
     the window, in the order given.  */
  Widget widget_to_the_left, widget_above;
  DECLARE_BUTTON (prev, "Prev", &the_font);
  DECLARE_BUTTON (next, "Next", &the_font);
  DECLARE_BUTTON (revert, "Revert", &the_font);
  DECLARE_BUTTON (expand, "Expand", &the_font);
  DECLARE_BUTTON (save, "Save", &the_font);
  DECLARE_BUTTON (exit, "Exit", FONT_NAME (the_font));

#if 0
  /* Tell them the resolution, in pixels per point.  It might be nice to
     be able to show dimensions in either points or pixels, but this
     suffices for us.  */
  string ppp_string = xmalloc (16); /* See the sprintf below.  */
  Arg ppp_args[]
    = { { XtNfromHoriz,	NULL }, /* We assign to this below.  */
        { XtNlabel,	(XtArgVal) ppp_string },
      };
  Widget ppp_widget;
#endif  /* 0 */

  /* An Item widget for changing the character randomly.  */
  static XtCallbackRec char_change_callbacks[]
    = SINGLE_CALLBACK (char_change_callback, &the_font);
  
  Arg item_args[]
    = { { XtNfromHoriz,	NULL }, /* We assign to this below.  */
        { XtNvalue,	NULL }, /* And this.  */
        { XtNcallback,	(XtArgVal) char_change_callbacks },
        { XtNlabel,	(XtArgVal) "Char: " },
        { XtNlength,	4 },    /* Longest valid code, e.g., `0xff'.  */
      };
  Widget item_widget;

  Arg position_args[]
    = { { XtNfromHoriz,	NULL }, /* We assign to this below.  */
      };
  Widget position_widget;
  Widget* update_position_data;
    
  /* An item widget for changing the left side bearing.  */
  static XtCallbackRec lsb_callbacks[]
    = SINGLE_CALLBACK (lsb_change_callback, &the_font);
  Arg lsb_args[]
    = { { XtNfromVert,	NULL }, /* We assign to this below.  */
        { XtNcallback,	(XtArgVal) lsb_callbacks },
        { XtNlabel,	(XtArgVal) "lsb: " },
        { XtNlength,	MAX_SB_LENGTH },
      };
  Widget lsb_widget;

  /* And one for changing the rsb.  */
  static XtCallbackRec rsb_callbacks[]
    = SINGLE_CALLBACK (rsb_change_callback, &the_font);
  Arg rsb_args[]
    = { { XtNfromHoriz,	NULL }, /* We assign to this below.  */
        { XtNfromVert,	NULL }, /* And this.  */
        { XtNcallback,	(XtArgVal) rsb_callbacks },
        { XtNlabel,	(XtArgVal) "rsb: " },
        { XtNlength,	MAX_SB_LENGTH },
      };
  Widget rsb_widget;

  /* Widgets to manipulate the kerning table.  */
  static XtCallbackRec add_kern_callbacks[]
    = SINGLE_CALLBACK (add_kern_callback, &the_font);
  Arg add_kern_args[]
    = { { XtNfromHoriz, NULL }, /* Assigned below.  */
        { XtNfromVert,	NULL },	/* Assigned below.  */
        { XtNlength,	4 },    /* Longest valid character code.  */
        { XtNlabel,	(XtArgVal) "Add kern: "},
        { XtNcallback,	(XtArgVal) add_kern_callbacks },
      };
  Widget add_kern_widget;

  static XtCallbackRec remove_kern_callbacks[]
    = SINGLE_CALLBACK (remove_kern_callback, &the_font);
  Arg remove_kern_args[]
    = { { XtNfromHoriz, NULL }, /* Assigned below.  */
        { XtNfromVert, 	NULL },	/* Assigned below.  */
        { XtNlength,	4 },    /* Longest valid character code.  */
        { XtNlabel,	(XtArgVal) "Del kern: "},
        { XtNcallback,	(XtArgVal) remove_kern_callbacks },
      };
  Widget remove_kern_widget;

  /* One Bitmap widget, with no expansion, displays the edited bitmap at
     its true size.  */
  Arg true_bitmap_args[]
    = { { XtNfromVert,	NULL }, /* We assign to this below.  */
        { XtNbits, (XtArgVal) &BCHAR_BITMAP (*initial_char) },
        { XtNexpansion,	1 },
      };
  Widget true_bitmap_widget;

  Arg kern_box_args[]
    = { { XtNfromVert,	NULL },	/* Assigned below.  */
      };
  Widget kern_box_widget;

  /* The viewport widget will contain the editable bitmap.  It does the
     scrolling of the (potentially big) bitmap for us.  */
  Arg viewport_args[]
    = { { XtNfromVert,	NULL },  /* Assigned below.  */
        { XtNhorizDistance, NULL },
      };
  Widget viewport_widget;
  
  /* The bitmap widget handles the actual pixel pushing.  */
  Arg bitmap_args[]
    = { { XtNshadow,	NULL }, /* We assign to this below.  */
        { XtNbits,	(XtArgVal) &BCHAR_BITMAP (*initial_char) },
      };
  Widget bitmap_widget;


  /* We specify the arguments for the Form in the defaults file.  */
  form_widget = XtCreateManagedWidget ("form", formWidgetClass, top, 0, NULL);

  prev_widget = XtCreateManagedWidget ("prev", commandWidgetClass, form_widget,
                                       prev_args, XtNumber (prev_args));

  widget_to_the_left = prev_widget;
  DEFINE_BUTTON (next, form_widget);
  DEFINE_BUTTON (revert, form_widget);
  DEFINE_BUTTON (expand, form_widget);
  DEFINE_BUTTON (save, form_widget);
  DEFINE_BUTTON (exit, form_widget);

#if 0
  /* Don't bother with this anymore.  */
  XTASSIGN_ARG (ppp_args[0], exit_widget);
  sprintf (ppp_string, "px/pt=%.2f", FONT_DPI (the_font) / POINTS_PER_INCH);
  ppp_widget = XtCreateManagedWidget ("ppp", labelWidgetClass,
                                      form_widget, XTARG (ppp_args));
  widget_to_the_left = ppp_widget;
#endif

  XTASSIGN_ARG (item_args[0], widget_to_the_left);
  XTASSIGN_ARG (item_args[1], utoa (FONT_CURRENT_CHARCODE (the_font)));
  item_widget
    = XtCreateManagedWidget ("current character item", itemWidgetClass,
                             form_widget, XTARG (item_args)); 
  widget_to_the_left = item_widget;

  XTASSIGN_ARG (position_args[0], item_widget);
  position_widget = XtCreateManagedWidget ("position", labelWidgetClass,
                                           form_widget, XTARG (position_args));

  /* So much for the top row.  The next row contains the left side
     bearing and right side bearing items, and stuff to add and remove
     kerns.  */
  widget_above = prev_widget; /* Any in the top row would do.  */

  XTASSIGN_ARG (lsb_args[0], widget_above);
  lsb_widget = XtCreateManagedWidget ("lsb item", itemWidgetClass,
                                      form_widget, XTARG (lsb_args));

  XTASSIGN_ARG (rsb_args[0], lsb_widget);
  XTASSIGN_ARG (rsb_args[1], widget_above);
  rsb_widget = XtCreateManagedWidget ("rsb item", itemWidgetClass,
                                      form_widget, XTARG (rsb_args));
  
  XTASSIGN_ARG (add_kern_args[0], rsb_widget);
  XTASSIGN_ARG (add_kern_args[1], widget_above);
  add_kern_widget = XtCreateManagedWidget ("+kern", itemWidgetClass,
                                          form_widget, XTARG (add_kern_args)); 

  XTASSIGN_ARG (remove_kern_args[0], add_kern_widget);
  XTASSIGN_ARG (remove_kern_args[1], widget_above);
  remove_kern_widget = XtCreateManagedWidget ("-kern", itemWidgetClass,
                                        form_widget, XTARG (remove_kern_args));

  /* The next row is the true-size bitmap.  */
  widget_above = lsb_widget;
  
  XTASSIGN_ARG (true_bitmap_args[0], widget_above);
  true_bitmap_widget = XtCreateManagedWidget ("true bitmap", bitmapWidgetClass,
                                        form_widget, XTARG (true_bitmap_args));

  /* The next row is the kern box.  */
  XTASSIGN_ARG (kern_box_args[0], true_bitmap_widget);
  kern_box_widget = XtCreateManagedWidget ("kern box", boxWidgetClass,
                                           form_widget, XTARG (kern_box_args));

  /* The Viewport holds the editable bitmap. We don't want it to overlap
     with any of the other windows, and yet we want to have as much
     screen space as possible.  So we put it below the previous row with
     the lsb_widget, even though we've also put in the kern box and true
     bitmap since then.  To ensure that it doesn't overlap with those
     two horizontally, we compute the width of a kern item, and the
     width of 1em, and make that the distance from the left edge.  This
     fails if a character's width > 1em > width (kern item), but that is
     an unlikely cirumstance.  We don't find the actual width of the
     widest character in the font, since then we would (perhaps) have to
     read all its characters.  */
  XTASSIGN_ARG (viewport_args[0], lsb_widget);
  {
    unsigned box_border;
    /* Compute an em in pixels.  */
    tfm_global_info_type tfm_info = FONT_TFM_FONT (FONT_INFO (the_font));
    real em_points = TFM_FONT_PARAMETER (tfm_info, TFM_QUAD_PARAMETER);
    unsigned em_pixels = POINTS_TO_PIXELS (em_points, FONT_DPI (the_font));
    
    /* Find the maximum width of a kern item.  */
    unsigned kern_item_width = find_kern_item_width (top);
    
    /* Use the max, plus the border width of the kern box.  We assume
       the border width of the form and other widgets is zero.  */
    unsigned offset = MAX (em_pixels, kern_item_width); 
    
    XtVaGetValues (kern_box_widget, XtNborderWidth, &box_border, NULL, NULL);
    offset += box_border * 2;
    
    XTASSIGN_ARG (viewport_args[1], offset);
  }
  viewport_widget = XtCreateManagedWidget ("viewport", viewportWidgetClass,
                                           form_widget, XTARG (viewport_args));

  /* The Viewport will manage the editable Bitmap.  We want the
     `true_bitmap' to be updated when the Bitmap is edited, so assign to
     the shadow resource.  */
  XTASSIGN_ARG (bitmap_args[0], true_bitmap_widget);
  bitmap_widget = XtCreateManagedWidget ("bitmap", bitmapWidgetClass,
                                         viewport_widget, XTARG (bitmap_args));

  /* We want to show the coordinates of the pointer in the bitmap
     widget whenever we have some spare time.  We may as well pass the
     widgets to the procedure.  */
  update_position_data = xmalloc (sizeof (Widget) * 2);
  update_position_data[0] = position_widget;
  update_position_data[1] = bitmap_widget;
  (void) XtAddWorkProc (update_position, update_position_data);
  
  show_char (form_widget, &the_font, initial_char);
}



/* This routine is called at idle times.  We want to display the
   coordinates of the pointer, if it's in the bitmap widget.  We are
   passed in CLIENT_DATA an array of two Widget pointers: the first is
   the label we are supposed to display the information; the second is
   the bitmap widget we are supposed to find the coordinates relative
   to.  Since it's more important to not consume cycles during periods
   of work than to provide an accurate display at all times, we do
   nothing at all on some percentage of calls.  */

static Boolean
update_position (XtPointer client_data)
{
  static char last_str[MAX_INT_LENGTH * 2 + 2] = "";
  char str[sizeof (last_str)];
  unsigned mask;
  Window root_window, child_window;
  int root_x, root_y, win_x, win_y;
  Widget *widgets = (Widget *) client_data;
  Widget bw = widgets[1];
  real test_value = (real) rand ();
  
  if (test_value / RAND_MAX < .75)
    return False;
    
  if (XQueryPointer (XtDisplay (bw), XtWindow (bw), &root_window,
                     &child_window, &root_x, &root_y, &win_x, &win_y, &mask))
    { /* They're on the same screen.  Now see if the coordinate is
          actually within the bitmap.  */
      unsigned expansion = BitmapExpansion (bw);
      int row = win_y / expansion;
      int col = win_x / expansion;
      bitmap_type *bitmap = BitmapBits (bw);
      
      if (row < 0 || row >= BITMAP_HEIGHT (*bitmap)
          || col < 0 || col >= BITMAP_WIDTH (*bitmap))
        str[0] = 0; 
      else
        sprintf (str, "%u,%u", col, row);
    }
  else
    str[0] = 0;
  
  if (!STREQ (str, last_str))
    {
      Widget lw = widgets[0];
      XtVaSetValues (lw, XtNlabel, str, NULL, NULL);
      strcpy (last_str, str);
    }
  
  /* We never want to be deleted from the work queue.  */
  return False;
}



/* Reading the options (besides the standard Xt options and our own X
   options, which have already been removed).  The arguments below do
   not have equivalents in X's resource database, nor should they,
   particularly.  So we don't want to give them to X to parse.  */

/* This is defined in version.c.  */
extern string version_string;

#define USAGE "Options:
<font_name> is a filename, e.g., `cmr10'.  Any extension is ignored.
  You can use `+' or `-' to start an option.
  You can use any unambiguous abbreviation for an option name.
  You can separate option names and values with `=' or ` '.
  The standard X toolkit options are also accepted.  This application's
  class name is `XBfe'.
dpi <unsigned>: use this resolution; default is 300.
expansion <unsigned>: each pixel in the bitmap will be this many pixels
  square on the display; default is 12.  You can't use `=' here to
  separate the option name and value.  You can set this value by setting
  a resource `expansion' in your .Xdefaults file.
initial-char <char>: start up displaying the character <char>; default is
  the character in the font with the smallest code.
help: print this message.
output-file <filename>: use <filename> as the output file; default is
  `<font_name>.<dpi>gf', i.e., we always output a GF file.
version: print the version number of this program.
"

static string
read_command_line (int argc, string argv[])
{
  int g;   /* `getopt' return code.  */
  int option_index;
  boolean explicit_dpi = false;
  boolean printed_version = false;
  struct option long_options[]
    = { { "dpi",			1, (int *) &explicit_dpi, 1 },
        { "initial-char",		1, 0, 0 },
        { "help",                       0, 0, 0 },
        { "output-file",		1, 0, 0 },
        { "version",			0, (int *) &printed_version, 1 },
        { 0, 0, 0, 0 } };
  
  while (true)
    {
      g = getopt_long_only (argc, argv, "", long_options, &option_index);
      
      if (g == EOF)
        break;

      if (g == '?')
        continue;  /* Unknown option.  */
  
      assert (g == 0); /* We have no short option names.  */
  
      if (ARGUMENT_IS ("dpi"))
        dpi = optarg;
      
      else if (ARGUMENT_IS ("initial-char"))
        FONT_CURRENT_CHARCODE (the_font) = xparse_charcode (optarg);
        
      else if (ARGUMENT_IS ("help"))
        {
          fprintf (stderr, "Usage: %s [options] <font_name>.\n", argv[0]);
          fprintf (stderr, USAGE);
          exit (0);
        }

      else if (ARGUMENT_IS ("output-file"))
        output_name = optarg;
        
      else if (ARGUMENT_IS ("version"))
        printf ("%s.\n", version_string);
      
      else
        ; /* It was just a flag; getopt has already done the assignment.  */
    }

  FINISH_COMMAND_LINE ();
}
