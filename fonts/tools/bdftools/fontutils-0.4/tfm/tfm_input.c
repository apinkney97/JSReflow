/* tfm_input.c: read a TFM file.

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

#include "file-input.h"
#include "tfm.h"


/* If true, print out what we read.  */
static boolean tracing_tfm_input = false;

/* These identify the file we're reading.  */
static FILE *tfm_input_file;
static string tfm_input_filename;


/* A copy of the character information.  We rely on the `exists' member
   of all these characters being initialized to zero (i.e., `false') by
   virtue of the variable being static.  */
static tfm_char_type tfm_char_table[TFM_SIZE];

/* A copy of the global information.  */
static tfm_global_info_type *global_info;

/* Global information which is only useful to us, not the user.  The
   `..._pos' members record the location of the various components of
   the file.  The member `param_word_count' says how many words of
   header information are present in this TFM file.  */
typedef struct
{
  byte_count_type char_info_pos;
  byte_count_type width_pos;
  byte_count_type height_pos;
  byte_count_type depth_pos;
  byte_count_type italic_correction_pos;
  byte_count_type lig_kern_pos;
  byte_count_type kern_pos;
  unsigned param_word_count;
} tfm_header_type;

static tfm_header_type tfm_header;


/* Low-level input.  These macros call the corresponding routines in
   kbase, using the static variables for the input file and filename.  */
#define TFM_FTELL() xftell (tfm_input_file, tfm_input_filename)
#define TFM_FSEEK(offset, from_where) \
  xfseek (tfm_input_file, offset, from_where, tfm_input_filename)
#define TFM_GET_BYTE() get_byte (tfm_input_file, tfm_input_filename)
#define TFM_GET_TWO() get_two (tfm_input_file, tfm_input_filename)
#define TFM_GET_FOUR() get_four (tfm_input_file, tfm_input_filename)


static tfm_char_type get_char ();
static string tfm_get_bcpl_string (void);
static real tfm_get_fix_word (void);
static void get_lig_kern_program (list_type *, list_type *);
static real tfm_get_scaled_fix (void);
static void get_tfm_header (void);
static void get_tfm_params (void);



/* Routines to start and finish reading a file.  (For the user to call.)  */

boolean
tfm_open_input_file (string filename)
{
  if (tfm_input_file != NULL)
    FATAL2 ("tfm_open_input_file: Attempt to open `%s', but `%s' is
already open", filename, tfm_input_filename);

  tfm_input_filename = filename;
  tfm_input_file = fopen (filename, "r");

  return tfm_input_file != NULL;
}


void
tfm_close_input_file ()
{
  assert (tfm_input_file != NULL);

  xfclose (tfm_input_file, tfm_input_filename);

  tfm_input_file = NULL;
  tfm_input_filename = NULL;
  global_info = NULL;
}



/* Some of the global information comes from the beginning of the file,
   and some from the end.  If we've already read the information from
   the file, we don't do it again.  */

tfm_global_info_type
tfm_get_global_info ()
{
  assert (tfm_input_file != NULL);
  
  /* Only read the file once.  */
  if (global_info != NULL) return *global_info; 
  
  global_info = XTALLOC (1, tfm_global_info_type);
  get_tfm_header ();
  get_tfm_params ();

  return *global_info;
}


/* Now comes convenience routines to get particular parts of the global
   info.  */

unsigned
tfm_get_checksum ()
{
  (void) tfm_get_global_info ();
  return TFM_CHECKSUM (*global_info);
}


double
tfm_get_design_size ()
{
  (void) tfm_get_global_info ();
  return TFM_DESIGN_SIZE (*global_info);
}


string
tfm_get_coding_scheme ()
{
  (void) tfm_get_global_info ();
  return TFM_CODING_SCHEME (*global_info);
}


double
tfm_get_interword_space ()
{
  (void) tfm_get_global_info ();
  return TFM_FONT_PARAMETER (*global_info, TFM_SPACE_PARAMETER);
}


double
tfm_get_x_height ()
{
  (void) tfm_get_global_info ();
  return TFM_FONT_PARAMETER (*global_info, TFM_XHEIGHT_PARAMETER);
}


/* Here we read the information at the beginning of the file.  We store
   the result into the static variables `global_info' and
   `tfm_header'.  */

static void
get_tfm_header ()
{
  two_bytes file_length, header_length;
  two_bytes width_word_count, height_word_count, depth_word_count;
  two_bytes italic_correction_word_count, lig_kern_word_count;
  two_bytes kern_word_count, extensible_word_count;

  /* The header is at the beginning of the file, naturally.  */
  TFM_FSEEK (0, SEEK_SET);

  file_length = TFM_GET_TWO ();
  header_length = TFM_GET_TWO ();

  global_info->first_charcode = TFM_GET_TWO ();
  global_info->last_charcode = TFM_GET_TWO ();
  width_word_count = TFM_GET_TWO ();
  height_word_count = TFM_GET_TWO ();
  depth_word_count = TFM_GET_TWO ();
  italic_correction_word_count = TFM_GET_TWO ();
  lig_kern_word_count = TFM_GET_TWO ();
  kern_word_count = TFM_GET_TWO ();
  extensible_word_count = TFM_GET_TWO ();
  tfm_header.param_word_count = TFM_GET_TWO ();

  tfm_header.char_info_pos = (6 + header_length) * 4;
  tfm_header.width_pos = tfm_header.char_info_pos
                         + (global_info->last_charcode
                            - global_info->first_charcode + 1) * 4;
  tfm_header.height_pos = tfm_header.width_pos + width_word_count * 4;
  tfm_header.depth_pos = tfm_header.height_pos + height_word_count * 4;
  tfm_header.italic_correction_pos = tfm_header.depth_pos
                                     + depth_word_count * 4;
  tfm_header.lig_kern_pos = tfm_header.italic_correction_pos
    + italic_correction_word_count * 4;
  tfm_header.kern_pos = tfm_header.lig_kern_pos + lig_kern_word_count * 4;
  /* We don't care about the extensible table.  */

  if (header_length < 2)
    FATAL2 ("TFM header of `%s' has only %u word(s)", tfm_input_filename,
            header_length);

  TFM_CHECKSUM (*global_info) = TFM_GET_FOUR ();
  TFM_DESIGN_SIZE (*global_info) = tfm_get_fix_word ();

  /* Although the coding scheme might be interesting to the caller, the
     font family and face byte probably aren't.  So we don't read them.  */
  TFM_CODING_SCHEME (*global_info)
    = header_length > 2 ? tfm_get_bcpl_string () : "unspecified";

  if (tracing_tfm_input)
    printf ("TFM checksum = %u, design_size = %fpt, coding scheme = `%s'.\n",
	      TFM_CHECKSUM (*global_info),
              TFM_DESIGN_SIZE (*global_info),
	      TFM_CODING_SCHEME (*global_info));
}


/* Although TFM files are only usable by TeX if they have at least seven
   parameters, that is not a requirement of the file format itself, so
   we don't impose it.  And they can have many more than seven, of
   course.  We do impose a limit of TFM_MAX_FONT_PARAMETERS.  We assume
   that `tfm_header' has already been filled in.  */

static void
get_tfm_params ()
{
  unsigned this_param;

  /* If we have no font parameters at all, we're done.  */
  if (tfm_header.param_word_count == 0)
    return;

  /* Move to the beginning of the parameter table in the file.  */
  TFM_FSEEK (-4 * tfm_header.param_word_count, SEEK_END);

  /* It's unlikely but possible that this TFM file has more fontdimens
     than we can deal with.  */
  if (tfm_header.param_word_count > TFM_MAX_FONT_PARAMETERS)
    {
      WARNING3 ("%s: TFM file has %u parameters, which is more than the
%u I can handle",
                tfm_input_filename, tfm_header.param_word_count,
                TFM_MAX_FONT_PARAMETERS);
      tfm_header.param_word_count = TFM_MAX_FONT_PARAMETERS;
    }

  /* The first parameter is different than all the rest, because it
     isn't scaled by the design size.  */
  TFM_FONT_PARAMETER (*global_info, TFM_SLANT_PARAMETER)
    = tfm_get_fix_word ();

  for (this_param = 2; this_param <= tfm_header.param_word_count;
       this_param++)
    TFM_FONT_PARAMETER (*global_info, this_param) = tfm_get_scaled_fix ();

  if (tracing_tfm_input)
    {
      for (this_param = 1; this_param <= tfm_header.param_word_count;
           this_param++)
         printf ("TFM parameter %d: %.3f",
                 TFM_FONT_PARAMETER (*global_info, this_param));
    }
  
  /* Save the number of fontdimens, for the convenience of our caller.  */
  TFM_FONT_PARAMETER_COUNT (*global_info) = tfm_header.param_word_count;
}



/* Read every character in the TFM file, storing the result in the
   static `tfm_char_table'.  We return a copy of that variable.  */

tfm_char_type *
tfm_get_chars ()
{
  tfm_char_type *tfm_chars;
  unsigned this_char;

  assert (tfm_input_file != NULL);

  tfm_get_global_info ();

  for (this_char = global_info->first_charcode;
       this_char <= global_info->last_charcode;
       this_char++)
    /* This fills in the `tfm_chars' global.  */
    (void) tfm_get_char (this_char);

  /* Return a copy, so our information can't get corrupted.  */
  tfm_chars = XTALLOC (TFM_SIZE, tfm_char_type);
  memcpy (tfm_chars, tfm_char_table, sizeof (tfm_char_table));
  return tfm_char_table;
}


/* Read the character CODE.  If the character doesn't exist, return NULL.  */

tfm_char_type *
tfm_get_char (charcode_type code)
{
  assert (tfm_input_file != NULL);

  tfm_get_global_info ();

  /* If the character is outside the declared bounds in the file, don't
     try to read it.  Just return NULL.  */
  if (code < global_info->first_charcode
      || code > global_info->last_charcode)
    return NULL;
  
  /* Move to the appropriate place in the `char_info' array.  */
  TFM_FSEEK (tfm_header.char_info_pos
             + (code - global_info->first_charcode) * 4,
	     SEEK_SET);

  /* Read the character.  */
  tfm_char_table[code] = get_char ();
  
  /* If it exists, return a pointer to it.  We return a copy, so our
     information can't get corrupted.  */
  if (!TFM_CHAR_EXISTS (tfm_char_table[code]))
    return NULL;
  else
    {
      tfm_char_type *c = XTALLOC (1, tfm_char_type);
      
      TFM_CHARCODE (tfm_char_table[code]) = code;
      *c = tfm_char_table[code];
      return c;
    }
}


/* We assume we are positioned at the beginning of a `char_info' word.
   We read that word to get the indexes into the dimension tables; then
   we go read the tables to get the values (if the character exists).  */

static tfm_char_type
get_char ()
{
  one_byte width_index, height_index, depth_index, italic_correction_index;
  one_byte packed;
  one_byte tag, remainder;
  tfm_char_type tfm_char;

  /* Read the char_info word.  */
  width_index = TFM_GET_BYTE ();

  packed = TFM_GET_BYTE ();
  height_index = (packed & 0xf0) >> 4;
  depth_index = packed & 0x0f;

  packed = TFM_GET_BYTE ();
  italic_correction_index = (packed & 0xfc) >> 6;
  tag = packed & 0x3;

  remainder = TFM_GET_BYTE ();

  tfm_char = tfm_new_char ();

#define GET_CHAR_DIMEN(d)						\
   if (d##_index != 0)							\
     {									\
       TFM_FSEEK (tfm_header.##d##_pos + d##_index*4, SEEK_SET);	\
       tfm_char.fix_##d = TFM_GET_FOUR ();				\
       tfm_char.##d = fix_to_real (tfm_char.fix_##d)			\
                      * global_info->design_size;			\
     }

  GET_CHAR_DIMEN (width);
  GET_CHAR_DIMEN (height);
  GET_CHAR_DIMEN (depth);
  GET_CHAR_DIMEN (italic_correction);

  /* The other condition for a character existing is that it be between
     the first and last character codes given in the header.  We've
     already assumed that's true (or we couldn't be positioned at a
     `char_info_word').  */
  TFM_CHAR_EXISTS (tfm_char) = width_index != 0;

  if (tracing_tfm_input)
    {
      printf ("   width = %f, height = %f, ", tfm_char.width, tfm_char.height);
      printf ("depth = %f, ic = %f.\n", tfm_char.depth,
              tfm_char.italic_correction); 
    }

  if (tag == 1)
    {
      TFM_FSEEK (tfm_header.lig_kern_pos + remainder * 4, SEEK_SET);
      get_lig_kern_program (&(tfm_char.ligature), &(tfm_char.kern));
    }

  /* We don't handle the other tags.  */
  return tfm_char;
}



/* Read a ligature/kern program at the current position, storing the
   result into *LIGATURE and *KERN.  We don't distinguish all the kinds
   of ligatures that Metafont can output.  */

#define STOP_FLAG 128
#define KERN_FLAG 128

static void
get_lig_kern_program (list_type *ligature, list_type *kern)
{
  boolean end_of_program;

  assert (ligature != NULL && kern != NULL);

  do
    {
      one_byte next_char;
      boolean kern_step;
      one_byte remainder;

      end_of_program = TFM_GET_BYTE () >= STOP_FLAG;

      next_char = TFM_GET_BYTE ();
      kern_step = TFM_GET_BYTE () >= KERN_FLAG;
      remainder = TFM_GET_BYTE ();

      if (tracing_tfm_input)
	  printf ("   if next = %u (%c), ", next_char, next_char);

      if (kern_step)
	{
	  byte_count_type old_pos = TFM_FTELL ();
	  tfm_kern_type *kern_element
            = append_element (kern, sizeof (tfm_kern_type));

	  kern_element->character = next_char;

	  TFM_FSEEK (tfm_header.kern_pos + remainder * 4, SEEK_SET);
	  kern_element->kern = tfm_get_scaled_fix ();
	  TFM_FSEEK (old_pos, SEEK_SET);

	  if (tracing_tfm_input)
	      printf ("kern %f.\n", kern_element->kern);
	}
      else
	{
	  tfm_ligature_type *ligature_element
            = append_element (ligature, sizeof (tfm_ligature_type));

	  ligature_element->character = next_char;
	  ligature_element->ligature = remainder;

	  if (tracing_tfm_input)
	      printf ("ligature %d (hex %x).\n",
  		      ligature_element->ligature, ligature_element->ligature);
	}
  } while (!end_of_program);
}



/* Most quantities are fixed-point fractions.  */

static real
tfm_get_fix_word ()
{
  return fix_to_real (TFM_GET_FOUR ());
}


/* Dimensions are a `fix_word' scaled by the design size.  */

static real
tfm_get_scaled_fix ()
{
  return tfm_get_fix_word () * global_info->design_size;
}


static string
tfm_get_bcpl_string ()
{
  unsigned string_length = TFM_GET_BYTE ();
  string s = get_n_bytes (string_length, tfm_input_file, tfm_input_filename);
  s = xrealloc (s, string_length + 1);
  s[string_length] = 0;

  return s;
}
