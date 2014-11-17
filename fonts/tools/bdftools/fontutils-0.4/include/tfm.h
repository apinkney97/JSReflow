/* tfm.h: read and write TeX font metric files.  See Metafont: The
   Program, by Don Knuth, (Volume D of Computers & Typesetting), chapter 45,
   among other places, for the precise definition of this font metric
   format.

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

#ifndef TFM_FONT_LIBRARY_H
#define TFM_FONT_LIBRARY_H

#include "fix-num.h"
#include "list.h"
#include "types.h"


/* Only one input file may be open at a time.  You therefore do not give a
   file as a parameter to the routines.  */
extern boolean tfm_open_input_file (string filename);
extern void tfm_close_input_file (void);


/* We output a property list file, since it is so much easier to deal
   with than the binary tfm format.  The program PLtoTF, which is part
   of the standard TeX distribution, converts from the one to the other.  */
extern boolean tfm_open_pl_output_file (string filename);
extern void tfm_close_pl_output_file (void);

/* This uses the PL filename to `tfm_open_pl_output_file' and invokes
   PLtoTF to output a TFM file with the same root and extension `.tfm'.
   You should not call `tfm_close_pl_output_file' before calling this.
   This flushes all the data written before converting.  */
extern void tfm_convert_pl (boolean verbose);


/* The restriction to 256 characters in a TFM file is part of the file
   format, so this number should only be changed in the (very unlikely)
   even that the file format changes.  */
#define TFM_SIZE 256



/* Fontwide information.  All real values are in printer's points:
   72.27 points = 1 inch.  */

/* The maximum number of global font parameters we allow.  */
#define TFM_MAX_FONT_PARAMETERS 30


/* A checksum must be greater or equal to this.  */
#define TFM_MIN_DESIGNSIZE 1.0

/* A checksum must be less than this.  */
#define TFM_MAX_DESIGNSIZE 2048

/* Check that a design size's value is in range.  */
#define TFM_CHECK_DESIGN_SIZE(ds)					\
if ((ds) < TFM_MIN_DESIGNSIZE || TFM_MAX_DESIGNSIZE <= (ds))		\
  FATAL3 ("Design size, %.2f, is not within range from %.1f to less than %d",\
          ds, TFM_MIN_DESIGNSIZE, TFM_MAX_DESIGNSIZE);

/* The maximum length of a coding scheme string.  */
#define TFM_MAX_CODINGSCHEME_LENGTH 39


typedef struct
{
  charcode_type first_charcode, last_charcode;
  four_bytes checksum;
  real design_size;
  string coding_scheme;
  unsigned parameter_count;
  real parameters[TFM_MAX_FONT_PARAMETERS];
} tfm_global_info_type;

/* The checksum.  */
#define TFM_CHECKSUM(info) ((info).checksum)

/* The design size of the font.  */
#define TFM_DESIGN_SIZE(info) ((info).design_size)

/* The coding scheme.  */
#define TFM_CODING_SCHEME(info) ((info).coding_scheme)

/* How many parameters are actually being used.  */
#define TFM_FONT_PARAMETER_COUNT(info) ((info).parameter_count)

/* The NUMBERth parameter of the `tfm_global_info_type' variable INFO,
   in points.  Since font parameters are numbered starting at 1, and the
   C array starts at 0, we subtract 1 from NUMBER.  */
#define TFM_FONT_PARAMETER(info, number) ((info).parameters[(number) - 1])

/* Define symbolic names for the numbers of the parameters we
   recognize.  Some numbers have more than one name.  */
#define TFM_SLANT_PARAMETER 1
#define TFM_SPACE_PARAMETER 2
#define TFM_STRETCH_PARAMETER 3
#define TFM_SHRINK_PARAMETER 4
#define TFM_XHEIGHT_PARAMETER 5
#define TFM_QUAD_PARAMETER 6
#define TFM_EXTRASPACE_PARAMETER 7
#define TFM_NUM1_PARAMETER 8
#define TFM_NUM2_PARAMETER 9
#define TFM_NUM3_PARAMETER 10
#define TFM_DENOM1_PARAMETER 11
#define TFM_DENOM2_PARAMETER 12
#define TFM_SUP1_PARAMETER 13
#define TFM_SUP2_PARAMETER 14
#define TFM_SUP3_PARAMETER 15
#define TFM_SUB1_PARAMETER 16
#define TFM_SUB2_PARAMETER 17
#define TFM_SUPDROP_PARAMETER 18
#define TFM_SUBDROP_PARAMETER 19
#define TFM_DELIM1_PARAMETER 20
#define TFM_DELIM2_PARAMETER 21
#define TFM_AXISHEIGHT_PARAMETER 22
#define TFM_DEFAULTRULETHICKNESS_PARAMETER 8
#define TFM_BIGOPSPACING1_PARAMETER 9
#define TFM_BIGOPSPACING2_PARAMETER 10
#define TFM_BIGOPSPACING3_PARAMETER 11
#define TFM_BIGOPSPACING4_PARAMETER 12
#define TFM_BIGOPSPACING5_PARAMETER 13

/* These are not in any of the standard TeX fonts, but the information
   is useful nevertheless.  */
#define TFM_LEADINGHEIGHT_PARAMETER 23
#define TFM_LEADINGDEPTH_PARAMETER 24
#define TFM_FONTSIZE_PARAMETER 25
#define TFM_VERSION_PARAMETER 26



/* Return the global info from the current input font.  */
extern tfm_global_info_type tfm_get_global_info (void);

/* We could have more routines here, one for each global quantity. 
   These call `tfm_get_global_info' for you, so that if all you are
   interested in is, say, the interword space, you can get only that.  */
extern unsigned tfm_get_checksum (void);
extern double tfm_get_design_size (void);
extern string tfm_get_coding_scheme (void);
extern double tfm_get_interword_space (void);
extern double tfm_get_x_height (void);


/* Return an initialized structure.  */
extern tfm_global_info_type tfm_init_global_info (void);


/* The `first_charcode', `last_charcode', and `parameter_count'
   members of this structure are computed automatically.  The rest of
   the structure is output to the PL file.  */
extern void tfm_put_global_info (tfm_global_info_type);


/* Set the header in TFM_INFO according to the string S, which
   should look like: <header-item>:<value>,<header-item>:<value>,...,
   where each <header-item> is <header-item> can be one of the strings
   `checksum', `designsize' or `codingscheme', with casefolding.  `checksum'
   requires <four-bytes>, `designsize' a <real>, with TFM_MIN_DESIGNSIZE
   <= <real> <= TFM_MAX_DESIGNSIZE, and `codingscheme' a <string> of
   length not greater than TFM_MAX_CODINGSCHEME_LENGTH and containing no
   parentheses or commas.  */

extern void tfm_set_header (string s, tfm_global_info_type *tfm_info);


/* Set values in INFO according to the specification in S, which should
   look like `<fontdimen>:<real>,<fontdimen>:<real>,...', where each
   <fontdimen> is either a number between 1 and TFM_MAX_FONT_PARAMETERS
   or one of the standard names.  */
extern void tfm_set_fontdimens (string s, tfm_global_info_type *info);



/* We store the character dimensions we read as both approximate
   floating point values, in printer's points, and as (unscaled by the
   design_size) `fix_word' values.  On output, we look only at the
   former.  */

typedef struct
{
  boolean exists;
  charcode_type code;
  double width, height, depth, italic_correction;
  fix_word fix_width, fix_height, fix_depth, fix_italic_correction;
  list_type kern;
  list_type ligature;
} tfm_char_type;

/* Says whether or not this character was in the TFM file.  */
#define TFM_CHAR_EXISTS(tc) ((tc).exists)

/* The character code.  */
#define TFM_CHARCODE(tc) ((tc).code)

/* The (possibly negative) dimensions, in points and fixes.  */
#define TFM_WIDTH(tc)  ((tc).width)
#define TFM_FIX_WIDTH(tc)  ((tc).fix_width)
#define TFM_HEIGHT(tc)  ((tc).height)
#define TFM_FIX_HEIGHT(tc)  ((tc).fix_height)
#define TFM_DEPTH(tc)  ((tc).depth)
#define TFM_FIX_DEPTH(tc)  ((tc).fix_depth)
#define TFM_ITALIC_CORRECTION(tc)  ((tc).italic_correction)
#define TFM_FIX_ITALIC_CORRECTION(tc)  ((tc).fix_italic_correction)

/* The kern list.  */
#define TFM_KERN(tc)  ((tc).kern)

/* The ligature list.  */
#define TFM_LIGATURE(tc)  ((tc).ligature)


/* This allocates and returns an array of `TFM_SIZE' elements, filled
   with the information in the input file about each character.  */
extern tfm_char_type *tfm_get_chars (void);

/* Return a pointer to the TFM information about the single character
   CODE, or NULL if the character CODE wasn't in the TFM file.  */
extern tfm_char_type *tfm_get_char (charcode_type code);

/* Return a single initialized `tfm_char_type' structure, and an
   initialized array of `TFM_SIZE' elements, respectively.  These are
   useful for output.  */
extern tfm_char_type tfm_new_char (void);
extern tfm_char_type *tfm_new_chars (void);

/* Take an array of `TFM_SIZE' elements and outputs them to
   the PL file.  The fix_word dimensions aren't looked at.  */
extern void tfm_put_chars (tfm_char_type *);

/* Output the single TFM character C.  */
extern void tfm_put_char (tfm_char_type c);

/* When typesetting, the current character + `character' leads to
   `ligature'.  The TFM format was extended in 1990 to allow for more
   complicated ligatures than this, but we do not make those
   distinctions.  */
typedef struct
{
  charcode_type character;
  charcode_type ligature;
} tfm_ligature_type;

/* Similarly for kerns.  */
typedef struct
{
  charcode_type character;
  real kern;
} tfm_kern_type;


/* Return the kern between the characters LEFT and RIGHT; if no such
   kern exists, return zero.  */
extern real tfm_get_kern (tfm_char_type left, charcode_type right);

/* Make the kern between the characters LEFT and RIGHT be K printer's
   points, replacing any kern already present.  */
extern void tfm_set_kern (tfm_char_type *left, charcode_type right, real k);

/* Add ligature for the characters LEFT and RIGHT to be L, replacing any
   ligature already present.  */

extern void tfm_set_ligature (tfm_char_type *left, charcode_type right,
			      charcode_type ligature);

/* Set the design (and font size) of TFM_INFO to DESIGN_SIZE, if they're
   not set already.  */
void
tfm_set_design_size (real design_size, tfm_global_info_type *tfm_info);

#endif /* not TFM_FONT_LIBRARY_H */
