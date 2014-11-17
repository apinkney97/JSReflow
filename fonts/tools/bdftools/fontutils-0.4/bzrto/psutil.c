/* psutil.c: utility routines for PostScript output.

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

#include "encoding.h"
#include "libfile.h"

#include "psutil.h"
extern string version_string;


/* Whether or not a particular character has been written.  We don't set
   this, but we use it to figure out the encoding to output.  */
boolean ps_char_output_p[ENCODING_VECTOR_SIZE];

/* The information from the encoding file.  */
static encoding_info_type encoding;


/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)							\
  do { fprintf (f, "%s\n", s); } while (0)

/* These output their arguments, preceded by the indentation.  */
#define OUT1(s, e)							\
  do { fprintf (f, s, e); } while (0)

#define OUT2(s, e1, e2)							\
  do { fprintf (f, s, e1, e2); } while (0)


static void out_notdef_loop (FILE *, charcode_type, charcode_type);




/* Return the information that goes in the FontInfo dictionary for the
   font FONT_NAME, as well as a few other miscellaneous things.  This
   reads an auxiliary file to get some information, uses TFM_INFO
   for other things, and guesses for the rest.  */

ps_font_info_type
ps_init (string font_name, tfm_global_info_type tfm_info)
{
  string base_encoding, mapping;
  ps_font_info_type ret;

  libfile_start ("postscript", "map");

  while ((mapping = libfile_line ()) != NULL
         && !STREQ (font_name, strtok (mapping, " \t")))
    ;

  libfile_close ();

  if (mapping == NULL)
    {
      WARNING2 ("%s: No information for font `%s'; using defaults",
                "postscript.map", font_name);
      ret.font_name = xstrdup (font_name);
      base_encoding = "adobestd";
    }
  else
    { /* We found it; dissect the rest of the line for what we need.
         The second word is the full PostScript font name, e.g.,
         `Times-BoldItalic'.  The third word is the base filename for
         the encoding vector.  */
      ret.font_name = strtok (NULL, " \t");
      base_encoding = strtok (NULL, " \t");
    }

  /* Read the encoding file.  We don't store this in the structure we
     return, since we've never needed it.  Perhaps we should anyway.  */
  encoding = read_encoding_file (base_encoding);

  /* The family name would be `Times' for the font `Times-BoldItalic', but it's
     `Helvetica' for `Helvetica'.  (It should be `Lucida' for
     `LucidaBright-Italic', but we don't handle that case -- maybe
     optional arg in mapping file?)  */
  ret.family_name = strchr (ret.font_name, '-');
  if (ret.family_name == NULL)
    ret.family_name = ret.font_name;  /* E.g., `Helvetica'.  */
  else
    ret.family_name
      = substring (ret.font_name, 0, ret.family_name - 1- ret.font_name);

  /* If the font name contains `Bold', that's the weight.  Otherwise,
     guess `Medium'.  (I don't know of any programs that actually care
     about this.  Again, perhaps it should be an optional arg in the
     mapping file.)  */
  ret.weight = strstr (ret.font_name, "Bold") ? "Bold" : "Medium";

  /* We should be able to compute the italic angle by somehow looking at
     the characters.  bdftops.ps rotates the `I' and takes the angle
     that minimizes the width, for example.  xx */
  ret.italic_angle = 0;
  
  /* Monospaced fonts have no stretch or shrink in their interword
     space (or shouldn't), but they do have a nonzero interword space
     (math fonts sometimes have all their interword space parameters set
     to zero).  */
  ret.monospace_p
    = TFM_FONT_PARAMETER (tfm_info, TFM_STRETCH_PARAMETER) == 0.0
      && TFM_FONT_PARAMETER (tfm_info, TFM_SPACE_PARAMETER) != 0.0;

  /* What might be a good way to compute this one?  xx */
  ret.underline_position = -100;
  
  /* Here we can use the rule thickness from the TFM file, if it's set.
     Otherwise, just guess.  (A better guess would be the dominant stem
     width in the font.)  */
  ret.underline_thickness
    = TFM_FONT_PARAMETER (tfm_info, TFM_DEFAULTRULETHICKNESS_PARAMETER)
      ? : 50;

  /* What to do about the UniqueID's?  Actually, I'm not sure they
     should really be necessary.  Adobe wants people to register their
     fonts to get a UniqueID from them, which is semi-reasonable, but a
     lot of trouble.  We just omit them.  */
  ret.unique_id = 0;
  
  /* If there is no version number in the TFM file, then just say it's
     version 0.  */
  ret.version = TFM_FONT_PARAMETER (tfm_info, TFM_VERSION_PARAMETER)
                ? dtoa (TFM_FONT_PARAMETER (tfm_info, TFM_VERSION_PARAMETER))
                : "0.0";

  return ret;
}




/* Output the common beginning (after the %! line) of a PostScript font
   to F, using the other args for information.  */

void
ps_start_font (FILE *f, ps_font_info_type ps_info, string comment)
{
  OUT1 ("%%%%Creator: %s\n", version_string);
  OUT1 ("%%%%CreationDate: %s\n", now ());
  OUT_LINE ("%%DocumentData: Clean7Bit");
  OUT_LINE ("%%EndComments");
  OUT1 ("%% {%s}\n", comment);
  OUT_LINE ("% This font is in the public domain.");

  OUT1 ("/%s 11 dict begin\n", ps_info.font_name);
  
  /* The FontInfo dictionary, which has additional
     (supposedly optional) information.  */
  OUT_LINE ("/FontInfo 8 dict begin");
  OUT1     ("  /version (%s) readonly def\n", ps_info.version);
  OUT1     ("  /FullName (%s) readonly def\n", ps_info.font_name);
  OUT1     ("  /FamilyName (%s) readonly def\n", ps_info.family_name);
  OUT1     ("  /Weight (%s) readonly def\n", ps_info.weight);
  OUT1     ("  /ItalicAngle %d def\n", ps_info.italic_angle);
  OUT1     ("  /isFixedPitch %s def\n",
                 ps_info.monospace_p ? "true" : "false");
  OUT1     ("  /underlinePosition %d def\n", ps_info.underline_position);
  OUT1     ("  /underlineThickness %d def\n", ps_info.underline_thickness);
  OUT_LINE ("currentdict end readonly def");
  
  /* Other constant top-level elements of the font dictionary.  Assume
     that the font name is the only thing on the stack at this point
     (which is true).  This saves us from having to write the name
     twice.  */
  OUT_LINE ("/FontName 1 index def");
  OUT_LINE ("/PaintType 0 def");  /* All our fonts are filled.  */

  if (ps_info.unique_id)
    OUT1 ("/UniqueID %d\n", ps_info.unique_id);
}


   

/* Return the name corresponding to the number N in the current
   encoding.  If the number is not defined, give a warning and return
   ".notdef".  */

string
ps_encoding_name (charcode_type n)
{
  string ret = ENCODING_CHAR_NAME (encoding, n);
  if (ret == NULL)
    {
      WARNING2 ("%d: No such character number in encoding scheme `%s'",
                n, ENCODING_SCHEME_NAME (encoding));
      ret = ".notdef";
    }
  return ret;
}


/* Return the number corresponding to NAME in the current encoding.  If
   NAME is not defined, return -1.  */

int
ps_encoding_number (string name)
{
  int ret = encoding_number (encoding, name);
  return ret;
}


/* Output the encoding vector to the file F, based on the variables
   `ps_char_output_p' and `encoding'.  */

#define INIT_CODE  ENCODING_VECTOR_SIZE

void
ps_output_encoding (FILE *f)
{
  unsigned c;
  int range_start = -1;

  fprintf (f, "/Encoding %d array\n", ENCODING_VECTOR_SIZE);

  for (c = 0; c < ENCODING_VECTOR_SIZE; c++)
    { /* Write sequences of .notdef's and characters we didn't output
         using a loop.  */
      if (!ps_char_output_p[c]
	  || STREQ (ENCODING_CHAR_NAME (encoding, c), ".notdef"))
	{ /* Start or continue a loop.  */
	  if (range_start == -1)
            range_start = c;
	}
      else
	{ /* Finish a loop if one is started, then output the current
             character.  */
          if (range_start != -1)
            { /* Loop started, finish it.  */
              out_notdef_loop (f, range_start, c - 1);
              range_start = -1;
            }
          
          /* Output the current character.  */
          OUT2 ("  dup %d /%s put\n", c, ENCODING_CHAR_NAME (encoding, c));
	}
    }
  
  /* If the encoding ends with .notdef's, we'll be in the midst of a
     loop. */
  if (range_start != -1)
    out_notdef_loop (f, range_start, c - 1);
  
  /* Define it.  */
  OUT_LINE ("  readonly def");
}


/* Output a loop to write .notdef's into the Encoding vector between
   START and END.  */

static void
out_notdef_loop (FILE *f, charcode_type start, charcode_type end)
{
  /* Assume the Encoding array is the top of the operand stack.  */
  OUT2 ("  %d 1 %d { 1 index exch /.notdef put } bind for\n", start, end);
}
