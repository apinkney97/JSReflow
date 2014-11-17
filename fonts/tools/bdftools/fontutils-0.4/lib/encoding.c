/* encoding.c: read a font encoding (.enc) file.

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
#include "tfm.h"


static encoding_char_type parse_encoding_line (string);
static tfm_ligature_type parse_ligature (void);



/* Returns the number for the character named NAME in E_I, or -1 if NAME
   is not present.  */

int 
encoding_number (encoding_info_type info, string name)
{
  int code;
  
  for (code = 0; code <= MAX_CHARCODE; code++)
    if (ENCODING_CHAR_NAME (info, code) != NULL
        && STREQ (ENCODING_CHAR_NAME (info, code), name))   
      return code;

  return -1;  
}



/* Parse the encoding file `FILENAME.enc' and return a structure
   describing what we read.  If the file doesn't exist, give a fatal
   error.  */

encoding_info_type
read_encoding_file (string filename)
{
  encoding_info_type info;
  string line;
  unsigned code;
  
  /* Start with an empty encoding.  */
  for (code = 0; code <= MAX_CHARCODE; code++)
    ENCODING_CHAR_NAME (info, code) = NULL;

  /* Prepare to read from FILENAME.  */
  libfile_start (filename, "enc");
  
  /* The entire first line is the TFM `CODINGSCHEME'.  */
  ENCODING_SCHEME_NAME (info) = libfile_line ();
  
  /* Each remaining line defines one character.  */
  code = 0;
  while ((line = libfile_line ()) != NULL)
    {
      ENCODING_CHAR_ELT (info, code) = parse_encoding_line (line);
      code++;
      free (line);
    }
  
  return info;
}



/* Parse one line of the encoding file; this specifies one character.
   The BNF is:
   
   <line>      ::= <word>  <ligatures>

   <ligatures> ::= <ligature> [<ligature>]?
                 | <empty>

   <ligature>  ::= lig <charcode> =: <charcode>
   
   Whitespace must be between all elements.
   
   If the line is malformed, we give a fatal error.  */

#define WORD_SEPARATOR " \t"
#define GET_WORD() strtok (NULL, WORD_SEPARATOR)

static encoding_char_type
parse_encoding_line (string line)
{
  encoding_char_type c;
  
  c.name = xstrdup (strtok (line, WORD_SEPARATOR));
  c.ligature = new_list ();
  
  do
    {
      tfm_ligature_type *lig;
      string t = GET_WORD ();
      
      if (t == NULL) /* Do we have more ligature specs?  */
        break;
        
      if (!STREQ (t, "lig"))
        LIBFILE_ERROR1 ("Expected `lig', found `%s'", t);

      lig = append_element (&c.ligature, sizeof (tfm_ligature_type));
      *lig = parse_ligature ();
    }
  while (true);

  return c;
}


/* Parse a single ligature: <charcode> =: <charcode>.  */

static tfm_ligature_type
parse_ligature ()
{
  tfm_ligature_type lig;
  boolean valid;
  string t = GET_WORD ();
  
  lig.character = parse_charcode (t, &valid);
  
  if (!valid)
    LIBFILE_ERROR1 ("Invalid right character code `%s'", t);

  t = GET_WORD ();
  if (!STREQ (t, "=:"))
    LIBFILE_ERROR1 ("Expected `=:', found `%s'", t);
  
  t = GET_WORD ();
  lig.ligature = parse_charcode (t, &valid);
  if (!valid)
    LIBFILE_ERROR1 ("Invalid ligature character code `%s'", t);
  
  return lig;
}
