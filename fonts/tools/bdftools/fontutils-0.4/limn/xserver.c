/* xserver.c: show pictures of the fitted characters on an X display.

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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>

#include "xserver.h"



/* For the resource database.  */
#define CLASS_NAME "Limn"

/* The root of the widget hierarchy.  */
static Widget top_level;

/* The connection to the X server.  */
static Display *display;

/* Where we draw the pictures.  */
static Widget canvas_widget;

/* The messages we recognize.  */
Atom foserver_exit_atom, foserver_update_pixmap_atom;

/* This property contains an identification string for the server.
   The string name is in xserver.h.  */
Atom foserver_identity_atom;

/* The type of the `data' member in a ClientMessage event.  */
typedef union
{
  char b[20];
  short s[10];
  long l[5];
} message_data_type;



static void dispatch_message (Atom, message_data_type);
static void foserver_exit (void);
static void foserver_update_pixmap (Pixmap);
static void message_handler (Widget, XEvent *, String *, Cardinal *);



void
#ifdef STANDALONE_SERVER
main (int argc, char *argv[])
{
  unsigned window_size;
  string identity; /* This will be the contents of the identity_atom */
#else /* not STANDALONE_SERVER */
start_server (unsigned window_size, string identity)
{
#endif /* not STANDALONE_SERVER */

  XTextProperty textP;
  Widget viewport_widget;
  int zero = 0;
  
  /* This routine will intercept messages we get.  */
  XtActionsRec actions[] = { { "MessageHandler", message_handler } };
  
  /* Here we assume that all the characters will fit in an em square. 
     (We make further assumptions below.)  The TFM format guarantees
     that the design size is not negative.  Values in the resource
     database override this.  These initial values are only useful if we
     are not standalone, but we reassign just below if we are
     standalone.  */
  Arg geometry_args[]
    = { { XtNheight, window_size },
        { XtNwidth,  window_size },
      };

#ifdef STANDALONE_SERVER
  if (argc == 3)
    {
      window_size = atou (argv[1]);
      geometry_args[0].value = geometry_args[1].value = window_size;
      identity = argv[2];
    }
  else
    {
      fprintf (stderr, "Usage: %s window-size identity.\n", argv[0]);
      exit(0);
    }
#endif /* STANDALONE_SERVER */

  /* We have no fallback resources, and we've already parsed the args.  */
  top_level = XtInitialize (NULL, CLASS_NAME, NULL, 0, &zero, NULL);

  viewport_widget
    = XtCreateManagedWidget ("viewport", viewportWidgetClass, top_level,
	                     NULL, 0); 
  canvas_widget
    = XtCreateManagedWidget ("canvas", labelWidgetClass, viewport_widget,
                             geometry_args, XtNumber (geometry_args));

  XtAddActions (actions, XtNumber (actions));

  XtRealizeWidget (top_level);
  
  display = XtDisplay (top_level);

  foserver_exit_atom = XInternAtom (display, FOSERVER_EXIT_ATOM, False);
  foserver_update_pixmap_atom
    = XInternAtom (display, FOSERVER_UPDATE_PIXMAP_ATOM, False);

  /* Get the identity atom number. Create it if it doesn't exist.  */
  foserver_identity_atom
    = XInternAtom (display, FOSERVER_IDENTITY_ATOM, False);

  textP.value = (unsigned char *) identity; /* Set value.  */
  textP.encoding = XA_STRING;
  textP.format = 8;
  textP.nitems = strlen (identity);
  XSetTextProperty (display, XtWindow (canvas_widget), &textP,
                    foserver_identity_atom);

  /* Process events forever.  */
  XtMainLoop ();
}


static void
message_handler (Widget w, XEvent *event, String *params, Cardinal *n_params)
{
  switch (event->type)
    {
    case ClientMessage:
      {
        XClientMessageEvent m = event->xclient;
        message_data_type d;
        
        /* Because two union types are never equivalent, and the X
           library does not define a type for the data part of the
           event, we must copy the data.  */
        memcpy (&d, &(m.data), sizeof (d.b));
        
        dispatch_message (m.message_type, d);
        break;
      }

    default:
      FATAL1 ("foserver: Unexpected event (type %d)", event->type);
    }
}


/* Call the appropriate routine for the atom A, or give an error.  */

static void
dispatch_message (Atom a, message_data_type data)
{
  if (a == foserver_exit_atom)
    foserver_exit ();
  
  else if (a == foserver_update_pixmap_atom)
    foserver_update_pixmap (data.l[0]);

  else
    WARNING1 ("foserver: Unknown message `%s'", XGetAtomName (display, a));
}


/* Shut down.  */

static void
foserver_exit ()
{
  XCloseDisplay (display);
  exit (0);
}


/* Show a new pixmap.  */

static void
foserver_update_pixmap (Pixmap p)
{
  Arg label_args[]
     = { { XtNbitmap, p },
       };
  
  /* Call XtSetValues only after drawing everything, since it
     doesn't particularly matter if what we draw is visible as we
     go.  (In general, because of the asynchronicity of the server,
     it wouldn't be, anyway.)  */
  XtSetValues (canvas_widget, label_args, XtNumber (label_args));
}
