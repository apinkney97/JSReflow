/* bbcount -- count bounding boxes in each character in a PBM file.

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

#include "bb-outline.h"
#define CMDLINE_NO_DPI /* It's not in the input filename.  */
#include "cmdline.h"
#include "filename.h"
#include "getopt.h"
#include "report.h"

#include "input-pbm.h"


/* Show every scanline on the terminal as we read it? (-trace-scanlines) */
boolean trace_scanlines = false;

/* Says whether to output progress reports.  (-verbose) */
boolean verbose = false;

/* The name of the file we're going to write.  (-output-file) */
static string output_name;


static string read_command_line (int, string[]);



/* The ``bitmap information'' file.  */

typedef struct
{
  /* The total number of characters in the image.  */
  unsigned nchars;
  
  /* The height of each character, in pixels.  */
  unsigned char_height;
} bfi_info_type;

static bfi_info_type read_bfi_file (string);




/* It is sad that this program is needed at all.  But I couldn't see any
   good approach to counting the bounding boxes for each character in
   Ghostscript.  So this program reads an image, counts the bounding
   boxes in each character, and writes another file with the bounding
   box counts.  */

#define PROGRAM_NAME "bbcount"

void
main (int argc, string argv[])
{
  FILE *bbs_file;
  bfi_info_type bfi;
  bitmap_type *b;
  unsigned charcount = 0;
  string font_name = read_command_line (argc, argv);
  
  /* Since we have to read two input files, we just throw away any
     extension they give us.  This doesn't support filenames like
     `a.b.pbm'.  */
  string font_basename = basename (font_name);
  if (font_basename != font_name)
    WARNING2 ("%s: Ignoring extension on `%s'", PROGRAM_NAME, font_name);

  /* Open the image input file.  */
  pbm_open_input_file (concat (font_basename, ".pbm"));
  
  /* Read the information about that image file.  */
  bfi = read_bfi_file (concat (font_basename, ".bfi"));

  if (output_name == NULL)
    output_name = font_basename;

  output_name =  make_output_filename (output_name, "bbs");
                    
  bbs_file = xfopen (output_name, "w");

  while ((b = pbm_get_block (bfi.char_height)) != NULL)
    {
      bounding_box_list_type boxes = find_outline_bbs (*b, false, 0, 0);
      fprintf (bbs_file, "%d\n", BB_LIST_LENGTH (boxes));
      
      charcount++;
      REPORT1 (".%s", charcount % 79 == 0 ? "\n" : "");
      free_bitmap (b);
    }
  REPORT ("\n");

  if (charcount != bfi.nchars)
    WARNING3 ("%s: Found %d characters, expected %d", PROGRAM_NAME,
              charcount, bfi.nchars);

  pbm_close_input_file ();
  
  exit (0);
}



/* Read the BFI file FILENAME, returning what we find.  */

static bfi_info_type
read_bfi_file (string filename)
{
  bfi_info_type i;
  FILE *bfi_file = xfopen (filename, "r");
  
  if (fscanf (bfi_file, "%d", &i.nchars) != 1)
    FATAL1 ("%s: Could not read character count", filename);

  if (fscanf (bfi_file, "%d", &i.char_height) != 1)
    FATAL1 ("%s: Could not read character height", filename);

  xfclose (bfi_file, filename);
  
  return i;
}



/* Reading the options.  */

/* This is defined in version.c.  */
extern string version_string;

#define USAGE "Options:
  <font_name> should be a base filename, e.g., `cmr10'."		\
    GETOPT_USAGE							\
"help: print this message.
output-file <filename>: write to <filename>.bbs if <filename> has no
  suffix, and to <filename> if it has.  Default is the concatenation of
  <font_name> with the size removed (e.g., for `cmr10' it would be `cmr')
  and `.bbs'.
trace-scanlines: show every scanline as we read it.
verbose: output progress reports to stderr.
version: print the version number of this program.
"

static string
read_command_line (int argc, string argv[])
{
  int g;  /* `getopt' return code.  */
  int option_index;
  boolean printed_version = false;
  struct option long_options[]
    = { { "dpi",		1, 0, 0 },
        { "output-file",	1, 0, 0 },
        { "trace-scanlines",	0, (int *) &trace_scanlines, 1 },
        { "verbose",		0, (int *) &verbose, 1 },
        { "version",            0, (int *) &printed_version, 1 },
        { 0, 0, 0, 0 } };

  while (true)
    {
      g = getopt_long_only (argc, argv, "", long_options, &option_index);
      
      if (g == EOF)
        break;

      if (g == '?')
        exit (1);  /* Unknown option.  */
  
      assert (g == 0); /* We have no short option names.  */
      
      if (ARGUMENT_IS ("help"))
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
        ; /* It was a flag; getopt has already done the assignment.  */
    }

  FINISH_COMMAND_LINE ();
}
