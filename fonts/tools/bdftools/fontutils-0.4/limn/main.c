/* limn -- convert bitmaps to splines.

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

#include "bzr.h"
#include "cmdline.h"
#include "filename.h"
#include "font.h"
#include "getopt.h"
#include "spline.h"

#include "display.h"
#include "fit.h"
#include "logreport.h"
#include "main.h"
#include "output-bzr.h"
#include "pxl-outline.h"


/* The resolution of the font we will read, in pixels per inch.  We
   don't deal with nonsquare pixels.  (-dpi)  */
string dpi = "300";

/* Says which characters we should process.  This is independent of the
   ordering in the font file.  (-range)  */
int starting_char = 0;
int ending_char = MAX_CHARCODE;

static string read_command_line (int, string []);



void
main (int argc, string argv[])
{
  int code;
  string font_name = read_command_line (argc, argv);
  bitmap_font_type f = get_bitmap_font (font_name, atou (dpi));
  string font_basename = basename (font_name);

  /* Initializing the display might involve forking a process.  We
     wouldn't want that process to get copies of open output files,
     since then when it exited, random stuff might get written to the
     end of the file.  */
  init_display (f);

  if (logging)
    log_file = xfopen (concat (font_basename, ".log"), "w");

  if (strlen (BITMAP_FONT_COMMENT (f)) > 0)
    REPORT1 ("{%s}\n", BITMAP_FONT_COMMENT (f));

  if (output_name == NULL)
    output_name = font_basename;

  bzr_start_output (output_name, f);

  /* The main loop: for each character, find the outline of the shape,
     then fit the splines to it.  */
  for (code = starting_char; code <= ending_char; code++)
    {
      pixel_outline_list_type pixels;
      spline_list_array_type splines;
      char_info_type *c = get_char (font_name, code);
    
      if (c == NULL) continue;

      REPORT1 ("[%u ", code);
      if (logging)
	{
	  LOG ("\n\n\f");
	  print_char (log_file, *c);
	}

      x_start_char (*c);
      pixels = find_outline_pixels (*c);
      /* `find_outline_pixels' uses corners as the coordinates, instead
         of the pixel centers.  So we have to increase the bounding box.  */
      CHAR_MIN_COL (*c)--; CHAR_MAX_COL (*c)++;
      CHAR_MIN_ROW (*c)--; CHAR_MAX_ROW (*c)++;

      REPORT ("|");
      splines = fitted_splines (pixels);

      bzr_output_char (*c, splines);
      
      /* Flush output before displaying the character, in case the user
         is interested in looking at it and the online version
         simultaneously.  */
      flush_log_output ();

      x_output_char (*c, splines);
      REPORT ("]\n");

      /* If the character was empty, it won't have a bitmap.  */
      if (BITMAP_BITS (CHAR_BITMAP (*c)) != NULL)
        free_bitmap (&CHAR_BITMAP (*c));

      free_pixel_outline_list (&pixels);
      free_spline_list_array (&splines);
    }

  bzr_finish_output ();
  close_display ();

  close_font (font_name);

  exit (0);
}



/* Reading the options.  */

/* This is defined in version.c.  */
extern string version_string;

#define USAGE "Options:
<font_name> should be a base filename, e.g., `cmr10'."			\
  GETOPT_USAGE								\
"align-threshold <real>: if either coordinate of the endpoints on a
  spline are closer than this, make them the same; default is .5.
corner-always-threshold <angle-in-degrees>: if the angle at a pixel is
  less than this, it is considered a corner, even if it is within
  `corner-surround' pixels of another corner; default is 60.
corner-surround <unsigned>: number of pixels on either side of a
  point to consider when determining if that point is a corner; default
  is 4.
corner-threshold <angle-in-degrees>: if a pixel, its predecessor(s),
  and its successor(s) meet at an angle smaller than this, it's a
  corner; default is 100.
display: show some information in a window, online.
display-grid-size <unsigned>: number of expanded pixels between the grid
  lines; default is 10. 
display-pixel-size <unsigned>: length of one side of the square that
  each pixel expands into; default is 9.
display-rectangle-size <unsigned>: length of one side of the square
  we draw to represent pixels; default is 6.  Must be less than
  display-pixel-size. 
display-stop: wait for you to hit return after displaying each character.
dpi <unsigned>: use a resolution of <unsigned>; default is 300.
error-threshold <real>: reject fitted curves that are off by
  more pixels than this; default is 2.0.
filter-alternative-surround <unsigned>: another choice for
  filter-surround; default is 1.
filter-epsilon <real>: if the angles using filter-surround and
   filter-alternative-surround points differ by more than this, use the
   latter; default is 10.0.
filter-iterations <unsigned>: smooth the curve this many times
  before trying to fit it; default is 4.
filter-percent <percent>: when filtering, use the old point plus this
  much of neighbors to determine the new point; default is 33.
filter-surround <unsigned>: number of pixels on either side of a point
  to consider when filtering that point; default is 2.
help: print this message.
keep-knees: do not remove ``knees''---points on the inside of the
  outline that are between two others.
line-reversion-threshold <real>: if a spline is closer to a straight
   line than this, weighted by the square of the curve length, keep it a
   straight line even if it is a list with curves; default is .01.
line-threshold <real>: if the spline is not more than this far away
  from the straight line defined by its endpoints, 
  then output a straight line; default is 1.
log: write detailed progress reports to <font_name>.log.
output-file <filename>: write to <filename> if it has a suffix and to
  `<filename>.bzr' if it doesn't.  Default is `<font_name>.bzr'.
range <char1>-<char2>: only process characters between <char1> and
  <char2>, inclusive. 
reparameterize-improve <percent>: if reparameterization
  doesn't improve the fit by this much, as a percentage, stop; default
  is 10.
reparameterize-threshold <real>: if an initial fit is off by more
  pixels than this, don't bother to reparameterize; default is 30.
subdivide-search <percent>: percentage of the curve from the initial
  guess for a subdivision point to look for a better one; default is 10.
subdivide-surround <unsigned>: number of points on either side of a
  point to consider when looking for a subdivision point; default is 4.
subdivide-threshold <real>: if a point is this close or closer to a
  straight line, subdivide there; default is .03.
tangent-surround <unsigned>: number of points on either side of a
  point to consider when trying to find the tangent at that point;
  default is 3.
verbose: print brief progress reports on stderr.
version: print the version number of this program.
"

/* We return the name of the font to process.  */

static string
read_command_line (int argc, string argv[])
{
  int g;   /* `getopt' return code.  */
  int option_index;
  boolean explicit_dpi = false;
  boolean printed_version = false;
  struct option long_options[]
    = { { "align-threshold",		1, 0, 0 },
        { "corner-always-threshold",    1, 0, 0 },
        { "corner-surround",            1, 0, 0 },
        { "corner-threshold",           1, 0, 0 },
        { "display",                    0, (int *) &wants_display, 1 },
        { "display-grid-size",		1, 0, 0 },
        { "display-pixel-size",		1, 0, 0 },
        { "display-rectangle-size",	1, 0, 0 },
        { "display-stop",		0, (int *) &display_stop, 1 }, 
        { "dpi",                        1, (int *) &explicit_dpi, 1 },
        { "error-threshold",            1, 0, 0 },
        { "filter-alternative-surround",1, 0, 0 },
        { "filter-epsilon",		1, 0, 0 },
        { "filter-iterations",          1, 0, 0 },
        { "filter-percent",		1, 0, 0 },
        { "filter-secondary-surround",  1, 0, 0 },
        { "filter-surround",            1, 0, 0 },
        { "help",                       0, 0, 0 },
        { "keep-knees",			0, (int *) &keep_knees, 1 },
        { "line-reversion-threshold",	1, 0, 0 },
        { "line-threshold",             1, 0, 0 },
        { "log",                        0, (int *) &logging, 1 },
        { "output-file",		1, 0, 0 },
        { "range",                      1, 0, 0 },
        { "reparameterize-improve",     1, 0, 0 },
        { "reparameterize-threshold",   1, 0, 0 },
        { "subdivide-search",		1, 0, 0 },
        { "subdivide-surround",		1, 0, 0 },
        { "subdivide-threshold",	1, 0, 0 },
        { "tangent-surround",           1, 0, 0 },
        { "verbose",                    0, (int *) &verbose, 1 },
        { "version",                    0, (int *) &printed_version, 1 },
        { 0, 0, 0, 0 } };

  while (true)
    {
      g = getopt_long_only (argc, argv, "", long_options, &option_index);
      
      if (g == EOF)
        break;

      if (g == '?')
        exit (1);  /* Unknown option.  */
  
      assert (g == 0); /* We have no short option names.  */
      
      if (ARGUMENT_IS ("align-threshold"))
        align_threshold = atof (optarg);
        
      else if (ARGUMENT_IS ("corner-always-threshold"))
        corner_always_threshold = atof (optarg);
        
      else if (ARGUMENT_IS ("corner-surround"))
        corner_surround = atou (optarg);
      
      else if (ARGUMENT_IS ("corner-threshold"))
        corner_threshold = atou (optarg);
      
      else if (ARGUMENT_IS ("display-grid-size"))
        display_grid_size = atou (optarg);
      
      else if (ARGUMENT_IS ("display-pixel-size"))
        display_pixel_size = atou (optarg);
      
      else if (ARGUMENT_IS ("display-rectangle-size"))
        display_rectangle_size = atou (optarg);

      else if (ARGUMENT_IS ("dpi"))
        dpi = optarg;
      
      else if (ARGUMENT_IS ("error-threshold"))
        error_threshold = atof (optarg);
      
      else if (ARGUMENT_IS ("filter-alternative-surround"))
        filter_alternative_surround = atou (optarg);
      
      else if (ARGUMENT_IS ("filter-epsilon"))
        filter_epsilon = atof (optarg);
        
      else if (ARGUMENT_IS ("filter-iterations"))
        filter_iteration_count = atou (optarg);

      else if (ARGUMENT_IS ("filter-percent"))
        filter_percent = GET_PERCENT (optarg);

      else if (ARGUMENT_IS ("filter-surround"))
        filter_surround = atou (optarg);

      else if (ARGUMENT_IS ("help"))
        {
          fprintf (stderr, "Usage: %s [options] <font_name>.\n", argv[0]);
          fprintf (stderr, USAGE);
          exit (0);
        }

      else if (ARGUMENT_IS ("line-reversion-threshold"))
        line_reversion_threshold = atof (optarg);
        
      else if (ARGUMENT_IS ("line-threshold"))
        line_threshold = atof (optarg);
      
      else if (ARGUMENT_IS ("output-file"))
        output_name = optarg;
      
      else if (ARGUMENT_IS ("range"))
        GET_RANGE (optarg, starting_char, ending_char);
      
      else if (ARGUMENT_IS ("reparameterize-improve"))
        reparameterize_improvement = GET_PERCENT (optarg);

      else if (ARGUMENT_IS ("reparameterize-threshold"))
        reparameterize_threshold = atof (optarg);

      else if (ARGUMENT_IS ("subdivide-search"))
        subdivide_search = GET_PERCENT (optarg);

      else if (ARGUMENT_IS ("subdivide-surround"))
        subdivide_surround = atou (optarg);

      else if (ARGUMENT_IS ("subdivide-threshold"))
        subdivide_threshold = atof (optarg); 

      else if (ARGUMENT_IS ("tangent-surround"))
        tangent_surround = atou (optarg);

      else if (ARGUMENT_IS ("version"))
        printf ("%s.\n", version_string);
      
      else
        ; /* It was just a flag; getopt has already done the assignment.  */
    }
  
  FINISH_COMMAND_LINE ();
}
