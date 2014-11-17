/* display.c: show output online using the X window system.

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
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xmu/SysUtil.h>

#include "spline.h"
#include "vector.h"

#include "display.h"
#include "main.h"
#include "xserver.h"

/* From main.c.  */
extern boolean verbose;

/* Says whether to show stuff online or not.  (-display)  */
boolean wants_display = false;

/* How many expanded pixels we put between lines on the grid.  Even
   though all this size and the others must be nonnegative, we must
   declare them as integers, since we do subtractions on them that might
   result in values less then zero.  (-display-grid-size) */
int display_grid_size = 10;

/* Size of square we print each pixel in.  (-display-pixel-size)  */ 
int display_pixel_size = 9;

/* Size of what we display inside the square for each pixel (must be
   less than `display_pixel_size').  (-display-rectangle-size)  */
int display_rectangle_size = 6;

/* Says whether to stop and wait for a <return> after each character. 
   (-display-stop)  */
boolean display_stop = false;

/* These are used for identifying the server. */
Atom foserver_identity_atom;

/* The messages our server recognizes.  */
Atom foserver_exit_atom, foserver_update_pixmap_atom;




/* The X state.  */

/* Where we should send our requests.  (This is information about
   our own ``limn server'', not the X server.)   We need the
   window only for when we send messages to our server.  */
static Display *display;
static Window server_window;

/* What we actually draw into.  */
static Pixmap pixmap = None;

/* The GC we use for drawing.  */
static GC gc;

/* Expands into the four arguments that define a rectangle for Xlib.  */
#define X_RECTANGLE(c) \
  (c).x, (c).y, display_rectangle_size, display_rectangle_size

/* Ditto, for lines.  */
#define X_LINE(c1, c2) (c1).x, (c1).y, (c2).x, (c2).y

/* Offsets for converting Cartesian coordinates to X ones.  */
static bounding_box_type offset_bb;



static coordinate_type cartesian_to_x (coordinate_type);
static void digitize_spline (spline_type);
static string get_identity (void);
static void get_server_info (Window *, Display **, string);
static coordinate_type real_cartesian_to_x (real_coordinate_type);
static coordinate_type real_cartesian_to_offset_x (real_coordinate_type);
static XSegment segment_to_x (XSegment);
static void send_event (Window, Atom, long);
static Bool window_change_p (Display *, XEvent *, char *);



/* Start things off.  */

void
init_display (bitmap_font_type f)
{
  string server_identity;
  
  /* Do nothing unless the user wants us to.  */
  if (!wants_display) return;
  
  server_identity = get_identity ();

#ifndef STANDALONE_SERVER
  { /* Fork our server.  This is the production case.  */
    unsigned design_size;

    int pid = fork ();
    switch (pid)
      {
      case -1:
        FATAL_PERROR ("fork");

      case 0:
        { /* We are the child, i.e., the server.  Convert the design size,
             which we have in points, to pixels.  `start_server' never
             returns, because it waits forever for events.  */
          design_size = (BITMAP_FONT_DESIGN_SIZE (f) * atof (dpi)
                         / POINTS_PER_INCH);

          start_server (design_size, server_identity);
          FATAL ("init_display: start_server returned");
        }

      default:
        /* We are the parent, i.e., the main process.
           Continue outside the switch.  */
        ;
      }
  }
#endif /* not STANDALONE_SERVER */

  /* Assign to the globals so that our other routines can send messages.  */
  get_server_info (&server_window, &display, server_identity);

  /* Register our message atoms with the X server.  */
  foserver_exit_atom = XInternAtom (display, FOSERVER_EXIT_ATOM, False);
  foserver_update_pixmap_atom
    = XInternAtom (display, FOSERVER_UPDATE_PIXMAP_ATOM, False);

}


/* Return the first child of W that has the property
   FOSERVER_IDENTITY_ATOM with the attribute string S.  This is not
   very efficient, but it is the only sure way.  We could have a window
   manager that put a lot of windows around the display server.
  
   If the search does not succed we return None.  */

static Window
search_children (Display *display, Window w, string s)
{
  Window root, parent, win;
  Window *children;
  unsigned nchildren;
  XTextProperty textP;

  /* Test this window. */
  if (XGetTextProperty (display, w, &textP, foserver_identity_atom)
      && textP.encoding == XA_STRING && textP.encoding == XA_STRING
      && textP.format == 8 && textP.nitems != 0
      && STREQ ((string) textP.value, s))
    {
      XFree ((char *) textP.value);
      return w; /* This is the correct window.  */
    }

  /* OK, we found nothing. Recursively try the children. */
  if (XQueryTree (display, w, &root, &parent, &children, &nchildren) == 0)
    FATAL ("limn: XQueryTree failed on parent");

  for (; nchildren > 0; nchildren--)
    if (win = search_children (display, children[nchildren-1], s))
      {
        XFree ((char *) children);
        return win;
      }
  XFree ((char *) children);

  return None; /* We didn't find anything. */
}


/* Return `hostname:pid' as a string.  */

static string
get_identity ()
{
  char s[1024];
  int i;
  
  i = XmuGetHostname (s, sizeof (s));
  if (i == 0)
    FATAL ("limn: Could not get hostname");

  sprintf (&s[i], ":%d", getpid ());
  return xstrdup (s);
}


/* Return the X window id for the window that the forked process should
   have created.  */

static void
get_server_info (Window *server_window, Display **display, string identity)
{
  int scr;
  
  /* Open our own connection to the X server, since sending messages
     through the server's connection hangs.  */
  *display = XOpenDisplay (NULL);
  if (*display == NULL)
    FATAL1 ("limn: Could not open display `%s'", getenv ("DISPLAY"));

  /* Get the identity atom. Create it if it doesn't exist.  */
  foserver_identity_atom
    = XInternAtom (*display, FOSERVER_IDENTITY_ATOM, False);

  /* Ask to be told about most window events.  */
  XSelectInput (*display, DefaultRootWindow (*display),
                SubstructureNotifyMask);

  /* Remove potential race condition below -- if our server has already
     had its window realized, we wouldn't find it otherwise. Search all
     windows first.  We might just as well search all screens too. */
  *server_window = None;
  for (scr = ScreenCount (*display); scr > 0 && *server_window == None; scr--)
    {
      Window root = RootWindow (*display, scr - 1);
      *server_window = search_children (*display, root, identity);
    }

  while (*server_window == None)
    {
      XEvent event;
      Window w;
      
      /* Wait for an event that might mean our server is ready.  */
      XIfEvent (*display, &event, window_change_p, NULL);

      /* We look for some property on the window, and rely on our server
         setting that property.  */
      switch (event.type)
        {
        case MapNotify:
          w = event.xmap.window;
	  *server_window = search_children (*display, w, identity);
          break;
          
        default:
          FATAL1 ("limn: Unexpected event (type %d)", event.type);
        }
    }

  /* We don't want to see any events any more.  */
  XSelectInput (*display, DefaultRootWindow (*display),
                NoEventMask);
}


/* The only event we're interested in is when a window gets mapped,
   because that's the last thing our server does before waiting for
   events.  */

static Bool
window_change_p (Display *d, XEvent *e, char *client_data)
{
  return e->type == MapNotify;
}


/* Tell our server to shut down, and close our own connection.  */

void
close_display ()
{
  if (!wants_display) return;

  send_event (server_window, foserver_exit_atom, 0);

  XCloseDisplay (display);
}



/* To start off a new character, we print a grid so we can locate points.  */

void 
x_start_char (char_info_type c)
{
  XGCValues gc_values;
  int x, y;
  unsigned n_vlines, n_hlines;
  unsigned char_width = (CHAR_BITMAP_WIDTH (c) + 2) * display_pixel_size;
  unsigned max_vlines = 1 + char_width / display_grid_size;
  unsigned char_height = (CHAR_BITMAP_HEIGHT (c) + 2) * display_pixel_size;
  unsigned max_hlines = 1 + char_height / display_grid_size;
  XSegment lines[max_vlines + max_hlines];
  
  if (!wants_display) return;

  /* Remember this bounding box, since we have to change coordinates
     from its coordinates into X coordinates.  */
  offset_bb = CHAR_BB (c);
  
  /* Allow for rounding errors.  */
  MAX_ROW (offset_bb) += 1;
  MIN_ROW (offset_bb) -= 1;
  MAX_COL (offset_bb) += 1;
  MIN_COL (offset_bb) -= 1;

  /* If this isn't the very first time we are called, we must free the
     storage for the previous character.  */
  if (pixmap != None)
    {
      XFreePixmap (display, pixmap);
      /* The GC and the pixmap are created together, so we can free them
         together.  */ 
      XFreeGC (display, gc);
    }

  /* We always want a Pixmap of depth one.  */
  pixmap = XCreatePixmap (display, DefaultRootWindow (display),
                          char_width, char_height, 1); 
  
  /* When drawing onto `pixmap', we want to draw in 1's, and have the
     background be 0's.  The Label widget does an XCopyPlane on
     monochrome Pixmaps, so this will result in the right colors on the
     screen.  But we want to start with the foreground 0 (which is the
     default), since the first thing we want to do is initialize it to
     all white.  */
  gc_values.background = 0;
  gc = XCreateGC (display, pixmap, GCBackground, &gc_values);

  /* Initialize `pixmap' to white.  */
  XFillRectangle (display, pixmap, gc, 0, 0, char_width, char_height);
  XSetForeground (display, gc, 1);

  /* Draw the grid.  */
  for (x = (MIN_COL (offset_bb) / display_grid_size) * display_grid_size,
         n_vlines = 0;
       x <= MAX_COL (offset_bb) && n_vlines < max_vlines;
       x += display_grid_size, n_vlines++)
    {
      lines[n_vlines].x1 = lines[n_vlines].x2 = x;
      lines[n_vlines].y1 = MAX_ROW (offset_bb);
      lines[n_vlines].y2 = MIN_ROW (offset_bb);
      lines[n_vlines] = segment_to_x (lines[n_vlines]);
    }

  /* X considers the origin to be at the upper left.  But it is easier
     to understand the display if the pixels in the row y=0 are directly 
     above a grid line, rather than hanging below it.  So we offset the
     y value by one.  */
  for (y = ((MIN_ROW (offset_bb)) / display_grid_size) * display_grid_size,
         n_hlines = 0;
       y <= MAX_ROW (offset_bb) && n_hlines < max_hlines;
       y += display_grid_size, n_hlines++)
    {
      lines[n_vlines + n_hlines].x1 = MIN_COL (offset_bb);
      lines[n_vlines + n_hlines].x2 = MAX_COL (offset_bb);
      lines[n_vlines + n_hlines].y1 = lines[n_vlines + n_hlines].y2 = y - 1;
      lines[n_vlines + n_hlines] = segment_to_x (lines[n_vlines + n_hlines]);
    }

  /* The grid is less obtrusive if we use dashed lines.  */
  XSetLineAttributes (display, gc, 0, LineOnOffDash, CapButt, JoinMiter);
  XDrawSegments (display, pixmap, gc, lines, n_vlines + n_hlines);
  XSetLineAttributes (display, gc, 0, LineSolid, CapButt, JoinMiter);
}


/* This routine is called for every pixel in the outline of the filtered
   bitmap.  */ 

void
display_pixel (real_coordinate_type real_c)
{
  coordinate_type c;

  if (!wants_display) return;
  
  c = real_cartesian_to_x (real_c);
  XDrawRectangle (display, pixmap, gc, X_RECTANGLE (c));
}


/* This marks the corners we find before we start the fitting.  Since
   these corners are on the original pixel outline, they are integers.  */

void
display_corner (coordinate_type c)
{
  if (!wants_display) return;

  c = cartesian_to_x (c);
  XFillRectangle (display, pixmap, gc, X_RECTANGLE (c));
}


/* This marks the points at which we subdivide during the fitting. 
   Since the filtering has happened now, the coordinates are not
   necessarily integers.  */

void
display_subdivision (real_coordinate_type real_c)
{
  coordinate_type c;
  if (!wants_display) return;
  
  c = real_cartesian_to_x (real_c);
  
  /* So the pixel we already drew at this location doesn't still appear
     behind the circle we are about to draw, thus turning the circle
     into a square.  */
  XSetForeground (display, gc, 0);
  XFillRectangle (display, pixmap, gc, X_RECTANGLE (c));
  XSetForeground (display, gc, 1);
  
  /* Oddly enough, the ``circle'' that's drawn is a rectangle, for small
     enough sizes.  */
  XFillArc (display, pixmap, gc, c.x, c.y,
            display_pixel_size, display_pixel_size, 0, 360 * 64);
}


/* Here we display the character C after fitting.  The list SPLINE_LIST
   is the fitted segments.  */

void
x_output_char (char_info_type c, spline_list_array_type splines)
{
  unsigned this_list;
  
  if (!wants_display) return;
  
  /* Draw the fitted lines a little thicker, so they'll be easier to see.  */
  XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (splines);
       this_list++) 
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (splines, this_list);

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == LINEAR)
            {
              coordinate_type start
                = real_cartesian_to_offset_x (START_POINT (s));
              coordinate_type end
                = real_cartesian_to_offset_x (END_POINT (s));

              XDrawLine (display, pixmap, gc, X_LINE (start, end));
            }

          else if (SPLINE_DEGREE (s) == CUBIC)
            digitize_spline (s);

          else
            FATAL1 ("x_output_char: Spline with degree %d", SPLINE_DEGREE (s));
        }
    }

  /* If desired, make sure everything is visible, then wait for the user
     to hit return, so s/he can look at the results.  */
  if (display_stop)
    {
      char dummy;
      
      send_event (server_window, foserver_update_pixmap_atom, pixmap);
      XSync (display, False);
      
      if (verbose) putchar ('\n');
      printf ("RET to continue: " );
      scanf ("%c", &dummy);
    }
}



/* Rasterize the spline S using forward differences, in floating point,
   non-adaptively.  This is slow compared to the integer, adaptive,
   version, but it's easier to program, and the speed is good enough for
   our purposes (i.e., non-interactive).  A brief but readable
   description of the algorithm is in ``Fast generation of outline
   characters'', by J. Gonczarowski, in the proceedings of the 1989
   conference on raster imaging and digital typography (page 100).  More
   of the math is spelled out in @cite{Fundamentals of interactive
   computer graphics}, by J.D. Foley and A. Van Dam (pp.531-534).  */
   
static void
digitize_spline (spline_type s)
{
  coordinate_type start = real_cartesian_to_offset_x (START_POINT (s)),
                  control1 = real_cartesian_to_offset_x (CONTROL1 (s)),
                  control2 = real_cartesian_to_offset_x (CONTROL2 (s)),
                  end = real_cartesian_to_offset_x (END_POINT (s));

  /* These a_i are the coefficients of the polynomial
     p(t) = a_3 t^3 + a_2 t^2 + a_1 t + a_0,
     computed by expanding the Bernshte\u in polynomial 
     z(t) = (1 - t)^3z_1 + 3(1 - t)^2tz_2 + 3(1 - t)t^2z_3 + t^3z_4,
     where z_1 is the starting point of the spline, z_2 and z_3 the
     control points, and z_4 the ending point.  We have two such
     polynomials p(t), one for the x-coordinate, one for the
     y-coordinate.  So everything here is done with points and vectors,
     instead of just numbers.
     
     a_0 = x_0
     a_1 = 3(x_1 - x_0)
     a_2 = 3(x_0 + x_2 - 2x_1)
     a_3 = x_3 - x_0 + 3(x_1 - x_2)  */
     
  coordinate_type /* a0 = start, */
    a1 = IPmult_scalar (IPsubtractP (control1, start), 3),
    a2 = IPmult_scalar (IPsubtractP (IPadd (start, control2),
                                     IPmult_scalar (control1, 2)),
                        3),
    a3 = IPadd (IPsubtractP (end, start),
                IPmult_scalar (IPsubtractP (control1, control2), 3));

  /* The step size.  We want to use the length of the bounding rectangle
     to compute this, instead of the distance between the starting point
     and the ending point, since the latter will be zero if the spline
     is cyclic.  */
  real factor = int_distance (control1, start)
                + int_distance (control2, control1)
                + int_distance (end, control2)
                + int_distance (start, end),
       /* Avoid division by zero, in pathological cases.  (We will just
          produce one point.  */
       delta = 1.0 / MAX (factor, 1),
       delta_squared = delta * delta,
       delta_cubed = delta_squared * delta;

  /* The current position.  */
  coordinate_type p = start,
                  previous_p = { start.x - 1, 0 };
  /* The real current position.  */
  real_coordinate_type real_p = int_to_real_coord (p);

  /* The first three forward differences evaluated at t = 0.  */
  vector_type d = make_vector (Padd (IPmult_real (a3, delta_cubed), 
                                     Padd (IPmult_real (a2, delta_squared),
                                           IPmult_real (a1, delta)))),
              d2 = make_vector (Padd (IPmult_real (a3, 6 * delta_cubed),
                                      IPmult_real (a2, 2 * delta_squared))),
              d3 = make_vector (IPmult_real (a3, 6 * delta_cubed));

  /* Where we will collect the points to output.  */
  unsigned point_count = 0;
  XPoint point_list[(unsigned) (1.0 / delta) + 1];

  real t;

  for (t = 0.0; t <= 1.0; t += delta)
    {
      if (!IPequal (p, previous_p))
        {
          point_list[point_count].x = p.x;
          point_list[point_count].y = p.y;
          point_count++;
          previous_p = p;
        }

      /* We must keep track of the current position in unrounded 
         coordinates, so the calculations do not lose precision.  */
      real_p = Padd_vector (real_p, d);
      p.x = ROUND (real_p.x);
      p.y = ROUND (real_p.y);
      d = Vadd (d, d2);
      d2 = Vadd (d2, d3);
      /* d3 is constant with respect to t.  */
    }

#if 0
  /* GDB under system V doesn't grok XPoint, so here's a way to print
     out the points we find.  */
  {
    unsigned i;
    for (i = 0; i < point_count; i++)
      printf ("(%d,%d)", point_list[i].x, point_list[i].y);
    puts ("");
  }
#endif

  XDrawPoints (display, pixmap, gc, point_list, point_count,
               CoordModeOrigin);
}



/* Send a ClientMessage event to the X window W, on the display
   `display'.  The message type is ATOM and the data DATA.  We could
   simply use the semi-global `server_window' (as we do `display'), but
   it proves useful to be able to send messages to other windows for
   testing.  */

static void
send_event (Window w, Atom atom, long data)
{
  XEvent e;
  
  e.xclient.type = ClientMessage;
  e.xclient.display = display;
  e.xclient.window = w;
  e.xclient.message_type = atom;
  e.xclient.format = 32;
  e.xclient.data.l[0] = data;
  
  if (XSendEvent (display, w, False, NoEventMask, &e) == 0)
    FATAL1 ("limn: XSendEvent (of %s) failed", XGetAtomName (display, atom));
}



/* Convert a single point from Cartesian coordinates, where the origin
   is at the reference point of the character, to X coordinates, where
   the origin is at the upper left corner.  */

static coordinate_type 
cartesian_to_x (coordinate_type in_c)
{
  coordinate_type c = in_c;

  c.x *= display_pixel_size;
  c.y *= display_pixel_size;
  c.x -= MIN_COL (offset_bb) * display_pixel_size;
  c.y = MAX_ROW (offset_bb) * display_pixel_size - c.y;

  return c;
}


/* Convert a single point, with real-valued coordinates, to X
   coordinates.  By rounding after the multiplications, we can display
   the real coordinates more accurately.  */

static coordinate_type
real_cartesian_to_x (real_coordinate_type real_c)
{
  coordinate_type c;
  
  real_c.x *= display_pixel_size;
  real_c.y *= display_pixel_size;
  real_c.x -= MIN_COL (offset_bb) * display_pixel_size;
  real_c.y = MAX_ROW (offset_bb) * display_pixel_size - real_c.y;

  c = real_to_int_coord (real_c);
  
  return c;
}


/* Like `real_cartesian_to_x', but translate the result to the center of
   the display_pixel_size by display_pixel_size rectangle that we turn each
   point into.  This is used for displaying the fitted character, since
   visually it makes more sense to have it go through the centers of the
   rectangles that represent the pixels, instead of the upper left
   corner.  */

static coordinate_type
real_cartesian_to_offset_x (real_coordinate_type real_c)
{
  coordinate_type c = real_cartesian_to_x (real_c);
  
  c.x += display_pixel_size / 2;
  c.y += display_pixel_size / 2;
  
  return c;
}


/* Convert two points, i.e., a line segment, from Cartesian to X
   coordinates.  Because an XSegment is not two XPoints, I didn't see
   any gain in defining another routine to deal with XPoints.  */

static XSegment
segment_to_x (XSegment in_s)
{
  XSegment out_s;
  coordinate_type c;
  
  c.x = in_s.x1;
  c.y = in_s.y1;
  c = cartesian_to_x (c);
  out_s.x1 = c.x;
  out_s.y1 = c.y;
  
  c.x = in_s.x2;
  c.y = in_s.y2;
  c = cartesian_to_x (c);
  out_s.x2 = c.x;
  out_s.y2 = c.y;
  
  return out_s;
}
