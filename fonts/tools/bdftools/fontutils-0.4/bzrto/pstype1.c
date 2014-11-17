/* pstype1.c: translate the BZR font to a Type 1 PostScript font. 
   See the book Adobe Type 1 Font Format for details of the format.

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

#include "bounding-box.h"
#include "bzr.h"
#include "filename.h"
#include "hexify.h"
#include "spline.h"
#include "tfm.h"
#include "varstring.h"

#include "pstype1.h"
#include "psutil.h"


/* Where we'll output.  */
static FILE *ps_file;
static string ps_filename;

/* Factor by which to multiply a dimension in points to get a number in
   Adobe's 1000 units/em coordinate system.  */
static real em_factor;

/* The interword space from the TFM file, in points.  */
static real tfm_space;

/* The UniqueID from the font info, or zero.  */
static unsigned unique_id;


/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)							\
  do { fprintf (ps_file, "%s\n", s); } while (0)			\

/* These output their arguments, preceded by the indentation.  */
#define OUT1(s, e)							\
  do { fprintf (ps_file, s, e); } while (0)

#define OUT2(s, e1, e2)							\
  do { fprintf (ps_file, s, e1, e2); } while (0)

#define OUT4(s, e1, e2, e3, e4)						\
  do { fprintf (ps_file, s, e1, e2, e3, e4); } while (0)




/* Type 1 opcodes.  */
typedef unsigned char type1_opcode;

#define T1_CLOSEPATH	9
#define T1_ENDCHAR	14
#define T1_HLINETO	6
#define T1_HMOVETO	22
#define T1_HSBW		13
#define T1_HVCURVETO	31
#define T1_RLINETO	5
#define T1_RMOVETO	21
#define T1_RRCURVETO	8
#define T1_VHCURVETO	30
#define T1_VLINETO	7
#define T1_VMOVETO	4




static void begin_char (string, real, real);
static void cs_byte (one_byte);
static variable_string cs_encrypt (variable_string);
static void cs_number (real);
static void end_char (void);
static void init_charstring_buffer (void);
static void out_curveto (real_coordinate_type *, spline_type);
static void out_lineto (real_coordinate_type *, real_coordinate_type);
static void out_moveto (real_coordinate_type *, real_coordinate_type);
static void out_point (real_coordinate_type *, real_coordinate_type);
static void out_relative_cmd (real_coordinate_type *, real_coordinate_type,
                              type1_opcode, type1_opcode, type1_opcode);
static void output_charstring_buffer (void);
static void output_private_dict_1 (void);
static void output_private_dict_2 (void);
static int points_to_adobes (real);




/* This should be called before the others in this file.  It opens the
   output file `OUTPUT_NAME.gsf', writes some preliminary boilerplate, the
   font bounding box, and sets up to writes the characters.  */

void
pstype1_start_output (string font_name, string output_name,
		      bzr_preamble_type pre, bzr_postamble_type post, 
                      tfm_global_info_type tfm_info)
{
  /* Find general information about the font.  Pass in the filename
     we were given.  */
  ps_font_info_type ps_info = ps_init (font_name, tfm_info);

  /* Remember the natural interword space, so we can output it later, if
     necessary.  Ditto for the UniqueID.  */
  tfm_space = TFM_FONT_PARAMETER (tfm_info, TFM_SPACE_PARAMETER);
  unique_id = ps_info.unique_id;

  /* If a Type 3 font is eexec-encrypted and in ASCII, its extension
     should be `pfa'.  If they are eexec-encrypted and binary, its
     extension should be `pfb'.  But if they're not encrypted, I don't
     know of any real convention.  We use Ghostscript's semi-conventional
     extension, `gsf'.  */
  ps_filename = make_output_filename (output_name, "gsf");
  ps_file = xfopen (ps_filename, "w");
  
  /* Output some identification.  */
  OUT2 ("%%!FontType1-1.0 %s %s\n", ps_info.font_name, ps_info.version);

  ps_start_font (ps_file, ps_info, BZR_COMMENT (pre));
  OUT_LINE ("/FontType 1 def");

  /* Always use the 1000 units-to-the-em coordinate system which Adobe
     has foisted on the world, and remember the scaling factor given
     this font's size.  */
  OUT_LINE ("/FontMatrix [0.001 0 0 0.001 0 0] readonly def"); 
  em_factor = 1000.0 / TFM_DESIGN_SIZE (tfm_info);
  
  /* Some interpreters refuse to handle a normal array,
     despite the specification.  */
  OUT4 ("/FontBBox {%d %d %d %d} def\n",
        points_to_adobes (MIN_COL (BZR_FONT_BB (post))),
        points_to_adobes (MIN_ROW (BZR_FONT_BB (post))),
        points_to_adobes (MAX_COL (BZR_FONT_BB (post))),
        points_to_adobes (MAX_ROW (BZR_FONT_BB (post))) );

  /* We output the encoding and some of the Private dictionary after
     we've read all the characters, so we can guess good values.  I hope
     this doesn't break any interpreters, but it probably will.  */
  output_private_dict_1 ();

  /* Start the character definitions.  Aside from the characters in the
     input font, we always output characters `.notdef' and `space'.  We
     must make the procedures in the Private dictionary visible.  */
  OUT_LINE ("Private begin");
  OUT1 ("/CharStrings %d dict begin\n", BZR_NCHARS (post) + 2);

  begin_char (".notdef", 0, 0);
  end_char ();
}




/* Output the character C.  */

void
pstype1_output_char (bzr_char_type c)
{
  unsigned this_list;
  spline_list_array_type shape = BZR_SHAPE (c);
  real_coordinate_type current = { 0.0, 0.0 };
  
  begin_char (ps_encoding_name (CHARCODE (c)), CHAR_LSB (c),
              CHAR_SET_WIDTH (c));
  
  /* Go through the list of splines, outputting the
     path-construction commands.  Since all Type 1 commands are
     relative, we also keep track of the current track.  */
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      spline_type first_spline = SPLINE_LIST_ELT (list, 0);

      /* Start this path.  */
      out_moveto (&current, START_POINT (first_spline));
      
      /* For each spline in the list, output the Type 1 commands.  */
      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == LINEAR)
            out_lineto (&current, END_POINT (s));
          else
            out_curveto (&current, s);
        }
      cs_byte (T1_CLOSEPATH);
    }

  end_char ();
  ps_char_output_p[CHARCODE (c)] = true;
}


/* Output the delta between *CURRENT and P, then update *CURRENT to be
   P.  */

static void
out_point (real_coordinate_type *current, real_coordinate_type p)
{
  cs_number (p.x - current->x);
  cs_number (p.y - current->y);
  *current = p;
}


/* Output some kind of lineto command between *CURRENT and P, and update
   *CURRENT.  If the line is horizontal or vertical, we can use hlineto
   or vlineto; otherwise, we have to use rlineto.  */

static void
out_lineto (real_coordinate_type *current, real_coordinate_type p)
{
  out_relative_cmd (current, p, T1_RLINETO, T1_HLINETO, T1_VLINETO);
}


/* Like out_lineto, except for moves.  */

static void
out_moveto (real_coordinate_type *current, real_coordinate_type p)
{
  out_relative_cmd (current, p, T1_RMOVETO, T1_HMOVETO, T1_VMOVETO);
}


/* Output a command relative to the points *CURRENT and P, then update
   CURRENT.  Use V_OPCODE if the two points are aligned vertically,
   H_OPCODE if horizontally, else R_OPCODE.  */

static void
out_relative_cmd (real_coordinate_type *current, real_coordinate_type p,
                  type1_opcode r_opcode, type1_opcode h_opcode,
                  type1_opcode v_opcode)
{
  boolean x_equal = epsilon_equal (p.x, current->x);
  boolean y_equal = epsilon_equal (p.y, current->y);
  
  if (x_equal)
    { /* If the two points are the same, we can omit this command
         altogether.  (And we should improve our curve-generating
         algorithm, perhaps.)  */
      if (!y_equal)
        {
          cs_number (p.y - current->y);
          cs_byte (v_opcode);
          current->y = p.y;
        }
    }
  else if (y_equal)
    {
      cs_number (p.x - current->x);
      cs_byte (h_opcode);
      current->x = p.x;
    }
  else
    {
      out_point (current, p);
      cs_byte (r_opcode);
    }
}


/* Output a curveto command.  The relative versions here occur if the
   first tangent is horizontal and the second vertical, or vice versa.  */

static void
out_curveto (real_coordinate_type *current, spline_type s)
{
  /* First tangent vertical, second horizontal?  */
  if (epsilon_equal (current->x, CONTROL1 (s).x)
      && epsilon_equal (CONTROL2 (s).y, END_POINT (s).y))
    {
      cs_number (CONTROL1 (s).y - current->y);
      current->y = CONTROL1 (s).y;
      out_point (current, CONTROL2 (s));
      cs_number (END_POINT (s).x - current->x);
      current->x = END_POINT (s).x;
      cs_byte (T1_VHCURVETO);
    }
  
  /* First tangent horizontal, second vertical?  */
  else if (epsilon_equal (current->y, CONTROL1 (s).y)
      && epsilon_equal (CONTROL2 (s).x, END_POINT (s).x))
    {
      cs_number (CONTROL1 (s).x - current->x);
      current->x = CONTROL1 (s).x;
      out_point (current, CONTROL2 (s));
      cs_number (END_POINT (s).y - current->y);
      current->y = END_POINT (s).y;
      cs_byte (T1_HVCURVETO);
    }
  
  /* Fall back.  */
  else
    {
      out_point (current, CONTROL1 (s));
      out_point (current, CONTROL2 (s));
      out_point (current, END_POINT (s));
      cs_byte (T1_RRCURVETO);
    }
}

/* Begin a character definition, i.e., a definition in the CharStrings
   dictionary.  We are given the character name, the left sidebearing,
   and the set width.  The latter two are in printer's points.  We
   always output an `hsbw' command -- our characters do not have y
   sidebearings.  */

static void
begin_char (string name, real lsb, real set_width)
{
  OUT1 ("/%s ", name);

  /* We cannot just output the bytes of the charstring as we go, because
     we need to output the length of the charstring before the
     charstring itself, so we can use readhexstring in the PostScript.  */
  init_charstring_buffer ();

  cs_number (lsb);
  cs_number (set_width);
  cs_byte (T1_HSBW);
}


/* End a character definition with the `endchar' command.  */

static void
end_char ()
{
  cs_byte (T1_ENDCHAR);
  output_charstring_buffer ();
}




/* Convert N, a number in printer's points, to the 1000-unit/em coordinate
   system the Type 1 format requires.  (I call these units ``adobes''.)
   We've already computed the scaling factor for this, since it depends
   only on the design size of the font.  */

static int
points_to_adobes (real n)
{
  int n_in_adobes = ROUND (n * em_factor);
  
  return n_in_adobes;
}




/* Charstring operations.  */

/* As we read a character, we encode each command in the Type 1 scheme
   and stick the result into a buffer.  (Because we need to output the
   length of the entire charstring before we output the charstring
   itself, we can't output as we go.)  */
static variable_string charstring_buffer;

static void
init_charstring_buffer ()
{
  charstring_buffer = vs_init ();
}


/* This appends the single byte B to the buffer, unchanged.  */

static void
cs_byte (one_byte b)
{
  vs_append_char (&charstring_buffer, b);
}


/* This appends the charstring encoding of the number N, assumed to be
   in printer's points, to the buffer.  */

static void
cs_number (real n)
{
  int n_in_adobes = points_to_adobes (n);
  
  if (-107 <= n_in_adobes && n_in_adobes <= 107)
    cs_byte (n_in_adobes + 139);
  else if (108 <= n_in_adobes && n_in_adobes <= 1131)
    {
      n_in_adobes -= 108;
      cs_byte (n_in_adobes / 256 + 247);
      cs_byte (n_in_adobes % 256);
    }
  else if (-1131 <= n_in_adobes && n_in_adobes <= -108)
    {
      n_in_adobes = -n_in_adobes - 108;
      cs_byte (n_in_adobes / 256 + 251);
      cs_byte (n_in_adobes % 256);
    }
  else
    {
      cs_byte (255);
      cs_byte ((n_in_adobes & 0xff000000) >> 24);
      cs_byte ((n_in_adobes & 0x00ff0000) >> 16);
      cs_byte ((n_in_adobes & 0x0000ff00) >> 8);
      cs_byte  (n_in_adobes & 0x000000ff);
    }
}


/* Perform charstring encryption on S, returning the (dynamically
   allocated) result as a string of binary characters.  See page 62 of
   Adobe Type 1 Font Format.  */

#define CS1 52845
#define CS2 22719

static variable_string
cs_encrypt (variable_string s)
{
  unsigned i;
  unsigned short cs_key = 4330;
  variable_string e = vs_init ();

  for (i = 0; i < VS_USED (s); i++)
    {
      unsigned char cipher = ((unsigned char) VS_CHARS (s)[i]) ^ (cs_key >> 8);
      cs_key = (cipher + cs_key) * CS1 + CS2;
      vs_append_char (&e, cipher);
    }
  
  return e;
}


/* Output what we've collected in `charstring_buffer'.  Because Type 1
   BuildChar is only required to handle encrypted charstrings, we must
   encrypt our encoded charstring.  */

static void
output_charstring_buffer ()
{
  variable_string encrypted_string = cs_encrypt (charstring_buffer);
  string hex_string = hexify (encrypted_string);

  /* The `-|' and `|-' procedures are defined in the Private dictionary.
     They read the hex string and define the character.  */
  OUT2 ("%d -| %s |-\n", VS_USED (encrypted_string), hex_string);

  free (hex_string);
  vs_free (&encrypted_string);
  vs_free (&charstring_buffer);
}




/* This is called last, after all the characters are output. We close
   the output file after outputting what's left.  */

void
pstype1_finish_output ()
{
  int space_encoding = ps_encoding_number ("space");
  
  /* If the space character hasn't been output, do so (even if it won't
     be encoded).  */
  if (space_encoding == -1 || !ps_char_output_p[space_encoding])
    {
      begin_char ("space", 0, tfm_space);
      end_char ();
      if (space_encoding != -1)
        ps_char_output_p[ps_encoding_number ("space")] = true;
    }

  /* If we were to synthesize additional characters, e.g., accented
     ones, we could output them here.  */

  /* Pop the Private and CharStrings dictionaries from the dictionary
     stack, then define the latter in the main font dict.  */
  OUT_LINE ("currentdict end readonly");
  OUT_LINE ("end % Private");
  OUT_LINE ("def % CharStrings");
  

  /* Two more elements go in the font dictionary.  */
  ps_output_encoding (ps_file);
  output_private_dict_2 ();

  /* Define the font (and discard the result from the operand stack).  */
  OUT_LINE ("currentdict end definefont pop");
  
  xfclose (ps_file, ps_filename);
}


/* The first part of the Private dictionary defines things that are
   independent of the character definitions.  */

static void
output_private_dict_1 ()
{
  OUT_LINE ("/Private 7 dict begin");
  OUT_LINE ("  /MinFeature {16 16} def");
  OUT_LINE ("  /password 5839 def");

  /* According to the spec, the only reason to set lenIV to 4 is to be
     compatible with the interpreter in the original LaserWriter.  We
     don't care about that, so we may as well reduce the number of bytes
     wasted on encryption.  */
  OUT_LINE ("  /lenIV 0 def");

  if (unique_id)
    OUT1   ("  /UniqueID %d def\n", unique_id);

  /* Here is how a character will get defined.  We expect two arguments
     on the stack -- a character name C, and an integer N.  We read an
     N-byte hex string) from the file, and define C to be that string.
     This is a simplified version of what Ghostscript's bdftops.ps does
     (the main simplification being no provision for lazy
     evaluation of characters).  */
  OUT_LINE ("  /-| {string currentfile exch readhexstring pop} readonly def");

  /* Once we have the string, we have to define the character.  */
  OUT_LINE ("  /|- {readonly def} readonly def");
  
  /* End defining the private dictionary.  */
  OUT_LINE ("currentdict end def");
}


/* The second part of the Private dictionary defines the hinting
   information, which we know only after we've seen the characters.  Or
   at least we will know it after we understand how to output hints.  */

static void
output_private_dict_2 ()
{
  OUT_LINE ("Private begin");
  OUT_LINE ("  /BlueValues [] def");
  OUT_LINE ("currentdict end /Private exch readonly def");
}
