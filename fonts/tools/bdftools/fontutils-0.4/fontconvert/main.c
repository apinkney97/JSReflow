/* fontconvert -- various operations on a bitmap font.

Copyright (C) 1990, 1991, 1992 Free Software Foundation, Inc.

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

#include "cmdline.h"
#include "filename.h"
#include "font.h"
#include "getopt.h"
#include "list.h"
#include "report.h"

#include "filter.h"
#include "output-epsf.h"
#include "output-gf.h"
#include "output-tfm.h"
#include "random.h"


/* How much to adjust the baseline of output characters by.  We rely on
   this being initialized to all zeros by C.  (-baseline-adjust)  */
int baseline_adjust[MAX_CHARCODE + 1];

/* If `column_split[N]' is non-NULL, we break character N into new
   characters at each of the columns specified.  We depend on this being
   initialized to all zeros.  (-column-split)  */
int *column_split[MAX_CHARCODE + 1];

/* The names of fonts to be concatenated onto the main font in the
   output.  (-concat-list)  */
list_type fontname_list;

/* The resolution of the font we will read, in pixels per inch.  We
   don't deal with nonsquare pixels.  (-dpi)  */
string dpi = "300";

/* The design size which will affect both the GF and TFM output, unless
   the tfm-header `designsize' option overrides it.  (-design-size)  
   `design_size_ratio' is the ratio of the input design size to the
   output design size.  */
real design_size = 0.0;
static real design_size_ratio;

/* If `omit[n]' is true for an element N, we throw away that character
   in that output.  (-omit)  */ 
static boolean omit[MAX_CHARCODE + 1];
   
/* An explicitly specified output filename.  (-output-file)  */
string output_name = NULL;

/* Says which characters we should process.  This is independent of the
   ordering in the font file.  (-range)  */
int starting_char = 0;
int ending_char = MAX_CHARCODE;

/* The variable `translate' maps character codes in the input font to
   character codes in the output; by default, maps everything to
   themselves.  (-remap)  */ 
one_byte translate[MAX_CHARCODE + 1];

/* Says whether to output an EPS file for each character.  (-epsf)  */
boolean wants_epsf = false;

/* Says whether to output a GF file.  (-gf)  */
boolean wants_gf = false;

/* Says whether to output the font as plain text.  (-text)  */
boolean wants_text = false;

/* Says whether to output a TFM file.  (-tfm)  */
boolean wants_tfm = false;

/* Says whether to output progress reports as we run.  (-verbose)  */ 
boolean verbose = false;



static void append_concat_list (list_type *, string);
static bitmap_type column_extract (bitmap_type, unsigned, unsigned);
static void do_char (char_info_type);
static void do_split_char (char_info_type, int[]);
static string read_command_line (int, string []);
static void scan_baseline_adjust (string);
static void scan_column_split (string);
static void scan_omit_list (string);
static void scan_remap_list (string);



void
main (int argc, string argv[])
{
  int code;
  bitmap_font_type f;
  string *first_element;
  string font_name;
  string output_basename;
  unsigned this_font;
  unsigned char_count = 0;


  /* Initialize the character code translation array before parsing the
     command line, since we probably will not be told to remap every
     character.  */
  for (code = 0; code <= MAX_CHARCODE; code++)
    translate[code] = code;

  /* Initialize the list of fonts to do to have one empty element at the
     beginning, which we will fill in with the main font name.  */
  fontname_list = new_list ();
  first_element = append_element (&fontname_list, sizeof (string));
  
  /* Read all the arguments.  */
  font_name = read_command_line (argc, argv);
  
  /* Put the main font name in the list.  */
  *first_element = font_name;

  output_basename = output_name == NULL
                    ? basename (font_name) : output_name;

  if (wants_gf && wants_tfm && find_suffix (output_name) != NULL)
    FATAL ("You can't specify the GF and TFM output files' suffices to be \
the same");

  /* Open the main font specially, since we need to know some basic
     information for most of the output formats.  We don't need to close
     it again, as opening a font twice does no harm, and in fact saves
     doing the path searching twice.  */
  f = get_bitmap_font (font_name, atou (dpi));

  
  if (wants_epsf)
    epsf_start_output (output_basename);

  if (wants_gf)
    gf_start_output (BITMAP_FONT_FILENAME (f), output_basename, dpi,
                     BITMAP_FONT_COMMENT (f)); 
  if (wants_tfm)
    tfm_start_output (font_name, output_basename, design_size,
		      BITMAP_FONT_DESIGN_SIZE (f));


  /* Have to do this after tfm_start_output, as it needs to know if
     design_size is set or not.  */
    
  if (design_size == 0.0)  
    design_size = BITMAP_FONT_DESIGN_SIZE (f);
  
  design_size_ratio = BITMAP_FONT_DESIGN_SIZE (f) / design_size;

  for (this_font = 0; this_font < LIST_SIZE (fontname_list); this_font++)
    {
      string font_name = *(string *) LIST_ELT (fontname_list, this_font);
      f = get_bitmap_font (font_name, atou (dpi));

      /* The main loop: convert each character.  */
      for (code = starting_char; code <= ending_char; code++)
        {
          char_info_type *c = get_char (font_name, code);

          if (c == NULL) continue;

          REPORT1 ("[%u", code);

          if (omit[code])
            REPORT (" omit");
          else
            {
              /* If this character isn't supposed to split, just output it.
                 Otherwise, output each of the pieces after splitting.  */
              if (column_split[code] == NULL)
                do_char (*c);
              else
                do_split_char (*c, column_split[code]);
            }

          REPORT1 ("]%c", ++char_count % 8 ? ' ' : '\n');
        }

      close_font (font_name);
    }
  
  if (char_count % 8 != 0)
    REPORT ("\n");
    
  if (wants_epsf)
    epsf_finish_output ();

  if (wants_gf)
    gf_finish_output (design_size, atof (dpi));

  if (wants_tfm)
    tfm_finish_output ();

  exit (0);
}


/* Output the character in whatever forms have been requested.  */

static void
do_char (char_info_type c)
{
  static boolean char_done_p[MAX_CHARCODE + 1];
  
  one_byte code;
  one_byte original_code = CHARCODE (c);
  
  /* Change the character code.  */
  code = CHARCODE (c) = translate[original_code];
  
  if (char_done_p[code])
    {
      WARNING1 ("Character %d already output", CHARCODE (c));
      return;
    }

  char_done_p[code] = true;

  if (filter_passes > 0)
    filter_bitmap (CHAR_BITMAP (c));

  if (random_max > 0.0)
    {
      bounding_box_type adjust_bb;
      randomize_bitmap (&CHAR_BITMAP (c), &adjust_bb);
      CHAR_MIN_ROW (c) -= MIN_ROW (adjust_bb);
      CHAR_MAX_ROW (c) += MAX_ROW (adjust_bb);
      CHAR_MIN_COL (c) -= MIN_COL (adjust_bb);
      CHAR_MAX_COL (c) += MAX_COL (adjust_bb);
    }

  /* If the `baseline_adjust' value is positive, the baseline should
     move up, i.e., we subtract from the height and add to the depth.
     But since the depth is represented as the minimum row, which can be
     negative, we must subtract from it to make it ``larger''.  */
  CHAR_MIN_ROW (c) -= baseline_adjust[code];
  CHAR_MAX_ROW (c) -= baseline_adjust[code];
  
  /* Text output is already implemented in the library.  */
  if (wants_text)
    {
      puts ("\f");
      print_char (stdout, c);
    }

  if (wants_epsf)
    epsf_output_char (c);

  if (wants_gf)
    gf_output_char (c, design_size_ratio);

  if (wants_tfm)
    tfm_output_char (c, atof (dpi));

  if (original_code != code)
    REPORT1 ("->%u", code);
    
    /* Free the space here, instead of in the caller, since we may
       change the bitmap.  */
    free_bitmap (&CHAR_BITMAP (c));
}


/* Output the character C in pieces, the division points being the
   columns in SPLIT.  */

static void
do_split_char (char_info_type c, int split[])
{
  unsigned last_col = 0;
  unsigned n_split = 0;

  REPORT (" (");

  /* We exit the loop after splitting the last character.  */
  while (true)
    {
      int diff;
      char_info_type split_char = c;
      int this_col = *split++;
      unsigned split_col = this_col != -1
                           ? this_col : BITMAP_WIDTH (CHAR_BITMAP (c));

      /* Change the character code of this piece.  */
      CHARCODE (split_char) += n_split;

      REPORT1 ("%u", CHARCODE (split_char));

      /* Take the chunk from the big bitmap.  */
      CHAR_BITMAP (split_char)
        = column_extract (CHAR_BITMAP (c), last_col, split_col);
      
      /* Move the right column over.  */
      diff = BITMAP_WIDTH (CHAR_BITMAP (c))
             - BITMAP_WIDTH (CHAR_BITMAP (split_char));
      CHAR_MAX_COL (split_char) -= diff;
      CHAR_SET_WIDTH (split_char) -= diff;
      
      /* Output it.  */
      do_char (split_char);

      /* Exit the loop if we just output the last piece.  */
      if (this_col == -1)
        break;

      n_split++;
      last_col = this_col;
      REPORT (",");
    }

  REPORT (")");
  free_bitmap (&CHAR_BITMAP (c));
}



/* Return the part of the bitmap SOURCE that lies between the columns
   START and FINISH - 1, inclusive.  */

static bitmap_type
column_extract (bitmap_type source, unsigned start, unsigned finish)
{
  unsigned this_row;
  dimensions_type d = { BITMAP_HEIGHT (source), finish - start };
  bitmap_type answer = new_bitmap (d);
  
  /* Move to the given starting column.  */
  BITMAP_BITS (source) += start;
  
  for (this_row = 0; this_row < BITMAP_HEIGHT (source); this_row++)
    {
      one_byte *answer_row = BITMAP_ROW (answer, this_row);
      
      memcpy (answer_row, BITMAP_BITS (source), BITMAP_WIDTH (answer));
      BITMAP_BITS (source) += BITMAP_WIDTH (source);
    }
  
  return answer;
}



/* Reading the options.  */

/* This is defined in version.c.  */
extern string version_string;

#define USAGE "Options:
<font_name> should be a filename, possibly with a resolution, e.g.,
  `cmr10' or `cmr10.300'."						\
  GETOPT_USAGE								\
"baseline-adjust <char>:<integer>,<char:integer>,...: move the baseline
  of each <char> by <integer>.  A positive number moves the baseline up.
column-split <char>@<column1>,...,<columnN>: split the character with
  code <char> (before translation) before each of the <column>s, producing n
  new characters, with codes <char>, <char> + 1, ..., <char> + n, whose
  bitmaps go from 0 to <column1> - 1 (inclusive), then <column1> to
  <column2> - 1, ..., from <columnN> to the bitmap width.
  Give the <column>s in bitmap coordinates, i.e., starting at zero.
  To split more than one character, give this option for each.
concat <font_name>,<font_name>,...: concatenate the main input font with
  the given <font_name>s; if a character code exists in more than one
  font, it's the first occurrence that counts.
design-size <real>: use this as the design size for both the GF and TFM
  output files, if any, unless overridden by `design-size' in the
  `tfm-header' option.
dpi <unsigned>: use a resolution of <unsigned>; default is 300.
epsf: output each character in the font as an Encapsulated PostScript
  file, named <font_name>-<code>.eps, where <code> is the character code
  in decimal.
filter-passes <unsigned>: do the filtering this many times on each
  character; default is 0.
filter-size <unsigned>: half the size of the filter cell, i.e., a side
  is this number * 2 + 1; default is 1.
filter-threshold <real>: if the average of the pixels in the filter cell
  is greater than this, change the pixel; default is .5.
fontdimens <fontdimen>:<real>,<fontdimen>:<real>,...: assign each <real>
  to the corresponding <fontdimen> when outputting a TFM file.  A
  <fontdimen> can be either one of the standard names (in either upper
  or lowercase), or a number between 1 and 30.  Each <real> is taken to
  be in points (except in the case of the <fontdimen> `slant' (parameter
  1), which is a pure number).
gf: write a GF file to `<font_name>.<dpi>gf'.  If this would overwrite the
  input file, write to `x<font_name>.<dpi>gf' instead.
help: print this message.
omit <char1>,<char2>,...: omit the character with the given codes
  (before translation) from the output.
output-file <filename>: use <filename> as the output filename if it has
   a suffix, and as the base of the output files if it doesn't.  It
   cannot have a suffix if using the `epsf' option, or both the `gf' and the
   `tfm' option.  Default is the base part of the input font name.
random <real>: move each pixel a (uniformly) random distance between
   -<real> and <real> in both x and y; default is 0.
random-threshold <real>: if randomizing, do not move pixels with
  probability <real>; default is 0.2.
range <char1>-<char2>: only process characters between <char1> and
  <char2>, inclusive.
remap <char1>:<char2>,<char1>:<char2>,...: for each pair, make the input
  character with code <char1> have code <char2> in the output.
text: output the font to stdout as plain text, using `*'s and ` 's.
tfm: write a TFM file to `<font_name>.tfm'.
tfm-header: <header-item>:<value>,<header-item>:<value>,...: assign each
  <value> to the corresponding <header-item> given, when outputting a
  TFM file.  A <header-item> can be one of the strings `checksum',
  `designsize' or `codingscheme', with casefolding. `checksum' requires
  an unsigned integer, `designsize' a real, with 1.0 <= designsize <
  2048, and `codingscheme' a string of length less than 40 and
  containing no parentheses or commas.
verbose: print brief progress reports on stdout.
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
    = { { "baseline-adjust",	1, 0, 0 },
        { "column-split",	1, 0, 0 },
        { "concat",		1, 0, 0 },
        { "design-size",	1, 0, 0 },
        { "dpi",		1, (int *) &explicit_dpi, 1 },
        { "epsf",		0, (int *) &wants_epsf, 1 },
        { "filter-passes",	1, 0, 0 },
        { "filter-size",	1, 0, 0 },
        { "filter-threshold",	1, 0, 0 },
        { "fontdimens",		1, 0, 0 },
        { "gf",			0, (int *) &wants_gf, 1 },
        { "help",		0, 0, 0 },
        { "omit",		1, 0, 0 },
        { "output-file",	1, 0, 0 },
        { "random",		1, 0, 0 },
        { "random-threshold",	1, 0, 0 },
        { "range",		1, 0, 0 },
        { "remap",		1, 0, 0 },
        { "text",		0, (int *) &wants_text, 1 },
        { "tfm",		0, (int *) &wants_tfm, 1 },
        { "tfm-header",		1, 0, 0 },
        { "verbose",		0, (int *) &verbose, 1 },
        { "version",		0, (int *) &printed_version, 1 },
        { 0, 0, 0, 0 } };

  while (true)
    {
      g = getopt_long_only (argc, argv, "", long_options, &option_index);
      
      if (g == EOF)
        break;

      if (g == '?')
        exit (1);  /* Unknown option.  */
  
      assert (g == 0); /* We have no short option names.  */
      
      if (ARGUMENT_IS ("baseline-adjust"))
        scan_baseline_adjust (optarg);
        
      else if (ARGUMENT_IS ("column-split"))
        scan_column_split (optarg);
        
      else if (ARGUMENT_IS ("concat"))
        append_concat_list (&fontname_list, optarg);
        
      else if (ARGUMENT_IS ("design-size"))
        {
          design_size = atof (optarg);
          TFM_CHECK_DESIGN_SIZE (design_size);
        }

      else if (ARGUMENT_IS ("dpi"))
        dpi = optarg;

      else if (ARGUMENT_IS ("filter-passes"))
        filter_passes = atou (optarg);

      else if (ARGUMENT_IS ("filter-size"))
        filter_size = atou (optarg);
      
      else if (ARGUMENT_IS ("filter-threshold"))
        {
          filter_threshold = atof (optarg);
          if (filter_threshold <= 0.0)
            FATAL1 ("The filter threshold should be positive, not %f",
                    filter_threshold);
        }
      
      else if (ARGUMENT_IS ("fontdimens"))
        fontdimens = optarg;

      else if (ARGUMENT_IS ("help"))
        {
          fprintf (stderr, "Usage: %s [options] <font_name>.\n", argv[0]);
          fprintf (stderr, USAGE);
          exit (0);
        }
      
      else if (ARGUMENT_IS ("omit"))
        scan_omit_list (optarg);
        
      else if (ARGUMENT_IS ("output-file"))
        output_name = optarg;
        
      else if (ARGUMENT_IS ("range"))
        GET_RANGE (optarg, starting_char, ending_char);
      
      else if (ARGUMENT_IS ("random"))
        random_max = atof (optarg);

      else if (ARGUMENT_IS ("random-threshold"))
        random_threshold = atof (optarg);

      else if (ARGUMENT_IS ("remap"))
        scan_remap_list (optarg);

      else if (ARGUMENT_IS ("tfm-header"))
        tfm_header = optarg;

      else if (ARGUMENT_IS ("version"))
        printf ("%s.\n", version_string);
      
      else
        ; /* It was just a flag; getopt has already done the assignment.  */
    }
  
  FINISH_COMMAND_LINE ();
}


/* The string S specifies baseline adjustments for individual
   characters: `<charcode>:<adjustment>,...'.  We set the element
   <charcode> of the global array `baseline_adjust' to the <adjustment>.  */
   
static void
scan_baseline_adjust (string s)
{
  string spec;
  
  for (spec = strtok (s, ","); spec != NULL; spec = strtok (NULL, ","))
    {
      string code;
      string adjust = strchr (spec, ':');
      
      if (adjust == NULL)
        FATAL1 ("Baseline adjustments look like `<code>:<integer>', not `%s'",
                spec);
      
      code = substring (spec, 0, adjust - spec - 1);
      baseline_adjust[xparse_charcode (code)] = atoi (adjust + 1);
    }
}


/* The string S says how to split a single character into multiple
   characters: `<charcode>@<column1>,...<columnN>'.  We set the element
   <charcode> of the global array `column_split' to the list of the
   integers.  */

static void
scan_column_split (string s)
{
  string code;
  string column_list = strchr (s, '@');
  
  if (column_list == NULL)
    FATAL1 ("Column splits look like `<code>@<column>,<column>,...', not `%s'",
      s);

  code = substring (s, 0, column_list - s - 1);
  column_split[xparse_charcode (code)] = scan_unsigned_list (column_list + 1);
}


/* The string S is a list of font names, separated by commas.  We append
   each onto CONCAT_LIST.  */

static void
append_concat_list (list_type *concat_list, string s)
{
  string name;
  
  /* The main routine must initialize CONCAT_LIST before we are called.  */
  assert (concat_list != NULL && LIST_SIZE (*concat_list) > 0);
  
  for (name = strtok (s, ","); name != NULL; name = strtok (NULL, ","))
    {
      string *new = append_element (concat_list, sizeof (string));
      *new = name;
    }
}


/* The string L is a list of character codes separated by commas; we
   omit those characters in the output.  Here, we parse the list and set
   elements of the global array `omit' according to what we find.  */

static void
scan_omit_list (string l)
{
  string map;
  
  for (map = strtok (l, ","); map != NULL; map = strtok (NULL, ","))
    {
      one_byte code = xparse_charcode (map);
      omit[code] = true;
    }
}


/* The string L is a list of remappings to apply, in the form
   <code1>:<code2>,<code1>:<code2>,...  where <code1> is a character
   code in the original font, and <code2> is the character code to write
   it as.  No checking for remappings to or from the same character is
   done here.
   
   We set elements of the global array `translate' according to what we
   find.  */

static void
scan_remap_list (string l)
{
  string map; /* A single remapping.  */
  
  for (map = strtok (l, ","); map != NULL; map = strtok (NULL, ","))
    {
      one_byte original, target;
      string original_str, target_str;
      unsigned length = strlen (map);
      
      /* Ignore empty mappings, as in `a:b,,c:d'.  */
      if (length == 0)
        continue;
      
      else if (length < 3)
        {
          WARNING1 ("Mapping `%s' too short to be valid", map);
          continue;
        }

      else if (*map == ':')
        { /* Must have form `::<code>'.  */
          original_str = ":";
          if (*(map + 1) != ':')
            {
              WARNING1 ("Mapping `%s' doesn't have form `<code>:<code>'", map);
              continue;
            }
          target_str = map + 2;
        }

      else if (*(map + length - 1) == ':')
        { /* Must have form `<code>::'.  */
          target_str = ":";
          if (*(map + length - 2) != ':')
            {
              WARNING1 ("Mapping `%s' doesn't have form `<code>:<code>'", map);
              continue;
            }
          original_str = substring (map, 0, length - 3);
        }
        
      else
        { /* Must have form `<code>:<code>'.  */
          target_str = strchr (map, ':');
          if (target_str == NULL)
            {
              WARNING1 ("Mapping `%s' doesn't have form `<code>:<code>'", map);
              continue;
            }
          target_str++;
          original_str = substring (map, 0, target_str - 2 - map);
        }

      original = xparse_charcode (original_str);
      target = xparse_charcode (target_str);
      translate[original] = target;
    }
}
