/* charspace -- find intercharacter spacing based on user information.

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

#include "char.h"
#include "cmdline.h"
#include "encoding.h"
#include "filename.h"
#include "font.h"
#include "getopt.h"
#include "logreport.h"
#include "perctab.h"
#include "symtab.h"

#include "main.h"
#include "sbi.h"
#include "output.h"


#ifdef DEBUG
int debug = 0;
#endif


/* The file with the default sidebearing apportionments.  */
string alphabet_sbi_name = "alphabet";

/* Font-specific sidebearing and other information.  */
string *sbi_names = NULL;


/* The resolution of the font we will read, in pixels per inch.  We
   don't deal with nonsquare pixels.  (-dpi)  */
string dpi = "300";


/* The font name we are given by the user.  */
string font_name;


/* Says which characters we should process.  This is independent of the
   ordering in the font file.  (-range)  */

/* Contains info for two below; have to parse after reading command line.  */
string unparsed_range;			

int starting_char = 0;
int ending_char = MAX_CHARCODE;


/* The name of the encoding file specified by the user.  (-encoding-file)  */
string encoding_name = NULL;

/* The encoding information.  */
encoding_info_type encoding_info;

/* Whether or not to output a gf file.  */
boolean no_gf;



static string read_command_line (int, string []);
static string * scan_string_list (string l);
static void read_sbi_file_list (string *sbi_names, string font_name, 
				string dpi, char_type chars[MAX_CHARCODE + 1]);
static boolean do_char (charcode_type code, char_type *c);



void
main (int argc, string argv[])
{
  bitmap_font_type font;
  string font_basename;
  char_type chars[MAX_CHARCODE + 1];
  unsigned charcode;
  unsigned char_count = 0;

  font_name = read_command_line (argc, argv);
  font = get_bitmap_font (font_name, atou (dpi));
  font_basename = basename (font_name);
  
  if (input_name == NULL)
    input_name = font_name;

  if (output_name == NULL)
    output_name = font_name;
  else if (find_suffix (output_name) != NULL)
    FATAL ("You can't specify all the output files' suffices to be the same");

  if (encoding_name == NULL)
    encoding_name = font_name;

  encoding_info = read_encoding_file (encoding_name);

  if (logging)
    log_file = xfopen (concat (font_basename, ".log"), "w");
  
  for (charcode = 0; charcode < MAX_CHARCODE; charcode++)
    init_char (&chars[charcode]);

  read_sbi_file (false, alphabet_sbi_name, "", chars);
  read_sbi_file_list (sbi_names, font_name, dpi, chars);
  
  /* Do percentage calculations.  */
  add_perc_defs_to_symbol_table ();

  /* The main loop: compute each existing character's information.  */
  for (charcode = starting_char; charcode <= ending_char; charcode++)
    {
      if (do_char (charcode, &(chars[charcode])))
	{
          char_count++;
          REPORT2 ("[%d]%c", charcode, char_count % 13 == 0 ? '\n' : ' ');
        }
    }

  if (char_count % 13 != 0)
    REPORT ("\n");


  output_font (font, chars);
  close_font (font_name);

  exit (0);
}



static void
read_sbi_file_list (string *sbi_names, string font_name, string dpi, 
	        char_type chars[MAX_CHARCODE + 1])
{
  unsigned this_sbi_name = 0;
  
  while (sbi_names[this_sbi_name] != NULL)
    {
      boolean give_warnings 
        = STREQ (basename (sbi_names[this_sbi_name]), basename (font_name));
    
      read_sbi_file (give_warnings, sbi_names[this_sbi_name], dpi, chars);
      this_sbi_name++;
    }
}



/* If the character is in the font, set its char_info and fill in its
   sidebearings using its sidebearing apportionment values.  */

static boolean
do_char (charcode_type code, char_type *c)
{
  if (get_char_info (code, c))
    {
      if (CHAR_LSB_PERCENT (*c) != CHAR_UNSET_VALUE 
          && CHAR_DISPLACEMENT (*c) != NULL)
	do_displacement (c);
      else if (CHAR_LSB_APP (*c) != NULL
	       && CHAR_RSB_APP (*c) != NULL)
        do_sidebearings (c);
      else
        return false;
        
      return true;
    }
  return false;
}


/* Reading the options.  */

/* This is defined in version.c.  */
extern string version_string;

#define USAGE "Options:
<font_name> should be a base filename, e.g., `cmr10'."			\
  GETOPT_USAGE								\
"dpi <unsigned>: use this resolution; default is 300.
encoding-file <filename>: get ligature and other encoding information
  from `<filename>.enc'; default is to get it from a TFM file
  `<font_name>.tfm'.
fontdimens <fontdimen>:<real>,<fontdimen>:<real>,...: assign <value>
  to each <fontdimen> given, when outputting a TFM file.  A <fontdimen>
  can be either one of the standard names (in either upper or
  lowercase), or a number between 1 and 30.  Each <real> is taken to be
  in points (except in the case of the <fontdimen> `SLANT' (parameter 1), 
  which is a pure number).
sbi-files <filename>: define variables or change letter relationships
  via `<filename>.<dpi>sbi'; default is `<font-name>.<dpi>sbi'.
log: write detailed progress reports to <font>.log.
no-gf: don't output a GF file.
output-file <filename>: write the TFM file to `<filename>.tfm' and the
  GF file to `<filename>.<dpi>gf'; <filename> shouldn't have a suffix;
  default is <font-name>.tfm and <font-name>.<dpi>gf, or, if those would
  overwrite the input, those preceded by `o'.
range <char1>-<char2>: only process characters between <char1> and
  <char2>, inclusive. 
verbose: print brief progress reports on stdout.
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
    = { { "dpi",			1, 0, 0 },
        { "encoding-file",		1, 0, 0 },
        { "fontdimens",			1, 0, 0 },
        { "help",                       0, 0, 0 },
        { "sbi-files",			1, 0, 0 },
        { "log",			0, (int *) &logging, 1 },
        { "no-gf",			0, (int *) &no_gf, 1 },
        { "output-file",		1, 0, 0 },
        { "range",			1, 0, 0 },
        { "verbose",			0, (int *) &verbose, 1 },
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
      
      else if (ARGUMENT_IS ("encoding-file"))
        encoding_name = optarg;

      else if (ARGUMENT_IS ("fontdimens"))
        fontdimens = optarg;

      else if (ARGUMENT_IS ("help"))
        {
          fprintf (stderr, "Usage: %s [options] <font_name>.\n", argv[0]);
          fprintf (stderr, USAGE);
          exit (0);
        }

      else if (ARGUMENT_IS ("sbi-files"))
        sbi_names = scan_string_list (optarg);

      else if (ARGUMENT_IS ("output-file"))
        output_name = optarg;

      else if (ARGUMENT_IS ("range"))
        GET_RANGE (optarg, starting_char, ending_char);
/*  	unparsed_range = optarg;*/
      
      else if (ARGUMENT_IS ("version"))
        printf ("%s.\n", version_string);
      
      else
        ; /* It was just a flag; getopt has already done the assignment.  */
    }
  
  FINISH_COMMAND_LINE ();
}


/* Take a string L consisting of unsigned strings separated by commas
   and return a vector of the strings, as pointers. 
   Append an element to the vector.  */

static string *
scan_string_list (string l)
{
  string map;
  unsigned length = 1;
  string *vector = xmalloc (sizeof (string));
  
  for (map = strtok (l, ","); map != NULL; map = strtok (NULL, ","))
    {
      length++;
      vector = xrealloc (vector, length * sizeof (string));
      vector[length - 2] = map;
    }
  
  vector[length - 1] = NULL;
  return vector;
}
