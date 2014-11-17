/* bzrto -- translate a font in the binary BZR outline format to various
   other formats.

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
#define CMDLINE_NO_DPI /* Outline fonts are resolution-independent.  */
#include "cmdline.h"
#include "filename.h"
#include "getopt.h"
#include "report.h"
#include "tfm.h"

#include "metafont.h"
#include "oblique.h"
#include "pstype1.h"
#include "pstype3.h"
#include "text.h"


/* Possible targets for the translation.  */
typedef enum
{ metafont, pstype1, pstype3, text, dummy_for_end
} translation_targets;
#define NTARGETS dummy_for_end

/* This array tells us which forms to translate the BZR file to.  */
static boolean output[NTARGETS];

/* Says whether to output simple progress reports as we run.  (-verbose)  */ 
boolean verbose = false;

/* The name of the output file specified by the user.  (-output-file)  */
string output_name = NULL;

/* This is defined in version.c.  */
extern string version_string;

static string read_command_line (int, string []);



void
main (unsigned argc, string argv[])
{
  bzr_preamble_type preamble;
  bzr_postamble_type postamble;
  tfm_char_type *tfm_chars;
  tfm_global_info_type tfm_info;
  string tfm_name;
  unsigned this_char;
  unsigned char_count = 0;
  string font_name = read_command_line (argc, argv);
  string font_basename = basename (font_name);
  string bzr_name = concat (font_name, ".bzr");

  if (!bzr_open_input_file (bzr_name))
    FATAL1 ("%s: Could not find BZR file", font_name);

  tfm_name = find_tfm_filename (font_name);
  if (tfm_name == NULL || !tfm_open_input_file (tfm_name))
    FATAL1 ("%s: Could not find TFM file", font_name);
  
  preamble = bzr_get_preamble ();
  postamble = bzr_get_postamble ();
  
  tfm_info = tfm_get_global_info ();
  tfm_chars = tfm_get_chars ();

  if (output_name == NULL)
    output_name = strtok (xstrdup (font_basename), "0123456789");
  else 
    if (find_suffix (output_name) != NULL
	&& ((output[metafont] && output[pstype1])
	    || (output[metafont] && output[pstype3])
	    || (output[pstype1] && output[pstype3])))
     FATAL ("You can't specify all the output files' suffices to be the same");

  
  if (output[metafont])
    metafont_start_output (output_name, preamble, tfm_info);
    
  if (output[pstype1])
    pstype1_start_output (font_basename, output_name, preamble, postamble,
    			  tfm_info); 
  if (output[pstype3])
    pstype3_start_output (font_basename, output_name, preamble, postamble, 
			  tfm_info); 
  if (output[text])
    text_start_output (font_basename, preamble);
  
  for (this_char = 0; this_char <= MAX_CHARCODE; this_char++)
    {
      bzr_char_type *c = bzr_get_char (this_char);
      
      if (c != NULL)
        {
          REPORT1 ("[%u", this_char);
          
          BZR_SHAPE (*c) = oblique_splines (BZR_SHAPE (*c));
          
          if (output[metafont])
            metafont_output_char (*c);
  
          if (output[pstype1])
            pstype1_output_char (*c);

          if (output[pstype3])
            pstype3_output_char (*c);
  
          if (output[text])
            text_output_char (*c);
  
          REPORT1 ("]%c", ++char_count % 8 ? ' ' : '\n');
        }
    }
  
  if (output[metafont]) metafont_finish_output (tfm_chars);
  if (output[pstype1]) pstype1_finish_output ();
  if (output[pstype3]) pstype3_finish_output ();
  if (output[text]) text_finish_output (postamble);

  if (char_count % 8 != 0)
    REPORT ("\n");
    
  bzr_close_input_file ();
  tfm_close_input_file ();
  exit (0);
}



/* Reading the options.  */

/* This is defined in version.c.  */
extern string version_string;

#define USAGE "Options:
<font_name> should be a filename, e.g., `cmr10'.  Any extension is ignored." \
  GETOPT_USAGE								     \
"help: print this message.
metafont: translate the font to a Metafont program.
oblique-angle <angle-in-degrees>: angle from the vertical by which to
  slant the shapes; default is 0.
output-file <filename>: use <filename> as the basename for output if it
  has no suffix, and <filename> if it does.  <filename> cannot have a
  suffix if more than one of `metafont', `pstype1' and `pstype3' options
  are chosen.  Default is <font_name> as the basename.
pstype1: translate the font to (unencrypted) PostScript type 1 font format.
pstype3: translate the font to PostScript type 3 font format.
text: translate the font to human-readable text; write to stdout.
verbose: print brief progress reports on stderr.
version: print the version number of this program.
"


/* We return the name of the font to process.  */

static string
read_command_line (int argc, string argv[])
{
  int g;   /* `getopt' return code.  */
  int option_index;
  boolean printed_version = false;
  struct option long_options []
    = { { "help",                       0, 0, 0 },
        { "metafont",			0, (int *) &output[metafont], 1 },
        { "oblique-angle",              1, 0, 0 },
        { "output-file",		1, 0, 0 },
        { "pstype1",			0, (int *) &output[pstype1], 1 },
        { "pstype3",			0, (int *) &output[pstype3], 1 },
        { "text",    			0, (int *) &output[text], 1 },
        { "verbose",                    0, (int *) &verbose, 1 },
        { "version",                    0, (int *) &printed_version, 1 },
        { 0, 0, 0, 0 } };

  while (true)
    {
      g = getopt_long_only (argc, argv, "", long_options, &option_index);
      
  if (g == EOF) break;
  if (g == '?') exit (1);  /* Unknown option.  */
  
      assert (g == 0); /* We have no short option names.  */
      
      if (ARGUMENT_IS ("help"))
        {
          fprintf (stderr, "Usage: %s [options] <font_name>.\n", argv[0]);
          fprintf (stderr, USAGE);
          exit (0);
        }

      else if (ARGUMENT_IS ("oblique-angle"))
        oblique_angle = M_PI * atof (optarg) / 180.0;
        
      else if (ARGUMENT_IS ("output-file"))
        output_name = optarg;

      else if (ARGUMENT_IS ("version"))
        printf ("%s.\n", version_string);
      
      else
        ; /* It was just a flag; getopt has already done the assignment.  */
    }
  
  FINISH_COMMAND_LINE ();
}
