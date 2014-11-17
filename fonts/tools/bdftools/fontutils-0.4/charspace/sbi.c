/* input.c: input a .sbi file.

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
#include "encoding.h"
#include "filename.h"
#include "libfile.h"
#include "logreport.h"
#include "perctab.h"
#include "report.h"
#include "symtab.h"

#include "main.h"
#include "sbi.h"


#define WORD_MAX 80

/* The name of the output file specified by the user.  (-input-file)  */
string input_name = NULL;




/* Read an SBI file; set global variables and CHARS accordingly. 

   `alphabet.sbi' has defaults for: A--G, I--N, P--R, T--Z, b--e, h--m,
   p--r, u--w, and y.

   An SBI file can contain things of the form:
   
   1) char <id_1> <id_2> <id_3>
        or
      char <id_1> <num_2> <num_3>
        where
      <id_1> is a character, 
        and
      <id_2> is a left sidebearing apportionment,
      <id_3> is a right sidebearing apportionment,
        or
      <num_2> is a left sidebearing (real) value,
      <num_3> is a right sidebearing (real) value,

   2) define <id> <number>.

   3) kern <id_1> <id_2> <number>
        where 
      <id_1> is a character to the left of character <id_2>.
      <number> is the kern (integer) value, in pixels.
      
   4) define-percent <id_1> <number> <id_2>
        where
      <id_1> is the percentage of <number> multiplied with the value of <id_2>.
      <id_2> must be defined using `define', but not necessarily previously.

   5) char-percent <id_1> <number> <id_2>
        where
      <id_1> is a character.
      <number> is the percentage of <id_2> that <id_1>'s left
         sidebearing will have.
*/

void 
read_sbi_file (boolean give_warnings, 
	       string input_name, string suffix_dpi,
	       char_type chars [MAX_CHARCODE + 1])
{
  string line;
  
  libfile_start (input_name, concat (suffix_dpi, "sbi"));

  while ((line = libfile_line ()) != NULL)
    {
      string word;
      string save_line = line; /* So we can free it.  */

      word = strtok (line, " \t");
      
      if (STREQ (word, "char"))
        {
	  string lsb, rsb;
          
          word = strtok (NULL, " \t");
          
	  /* If the character isn't in the encoding.  */
          if (encoding_number (encoding_info, word) == -1)
	    {
              if (give_warnings)
                WARNING2 (
                  "Character `%s' from file `%s' not found in encoding file",
                   word, concat (input_name, ".sbi"));
            }
	  else
            {
              lsb = xstrdup (strtok (NULL, " \t"));
              CHAR_LSB_APP (chars[encoding_number (encoding_info,word)]) = lsb;

              rsb = xstrdup (strtok (NULL, " \t"));
              CHAR_RSB_APP (chars[encoding_number (encoding_info,word)])= rsb;

              DEBUG_PRINT4 ("\nCHAR: `%s'; LSB: `%s'; RSB: `%s'.\n",
                            word, lsb, rsb);
            }
        }
      else if (STREQ (word, "kern"))
	{
	  string left, right;
          
          left = strtok (NULL, " \t");
          right = strtok (NULL, " \t");
          word = strtok (NULL, " \t");
          
	  set_kern (&chars[encoding_number (encoding_info, left)],
          	    encoding_number (encoding_info, right),
                    atoi (word));
        }
      else if (STREQ (word, "define"))
	{
          string value_string;
          real value;
          
          word = xstrdup (strtok (NULL, " \t"));
	  value_string = strtok (NULL, " \t");

          if (value_string != NULL)
	    value =  atof (value_string);
          else
            FATAL2 ("Bad define value `%s' in file `%s'",
            	    value_string, concat (input_name, ".sbi"));

          DEBUG_PRINT3 ("\nDEFINE: `%s' as `%s'.\n", word, value_string);

          add_to_symbol_table (word, value);
        }
      else if (STREQ (word, "define-percent"))
	{
          string percent_string;
          string percent_of_string;
          real percent;
          
          word = xstrdup (strtok (NULL, " \t"));
	  percent_string = strtok (NULL, " \t");

          if (percent_string != NULL)
	    percent =  atof (percent_string);
          else
            FATAL2 ("Bad define percent percentage `%s' in file `%s'",
            	    percent_string, concat (input_name, ".sbi"));

	  percent_of_string = strtok (NULL, " \t");

          if (percent_of_string == NULL)
            FATAL2 ("Bad define percent `percent of' `%s' in file `%s'",
            	    percent_of_string, concat (input_name, ".sbi"));

          DEBUG_PRINT4 ("\nDEFINE-PERCENT: `%s' as `%s' times `%s'.\n", 
		        word, percent_string, percent_of_string);

          add_to_perc_def_table (word, percent, percent_of_string);
        }
      else if (STREQ (word, "char-percent"))
	{
          char_type *c;
          string encoding_name = strtok (NULL, " \t");
	  string displacement;
                    
          c = &chars[encoding_number (encoding_info, encoding_name)];
          word = strtok (NULL, " \t");

          if (word != NULL)
	    CHAR_LSB_PERCENT (*c) = atof (word);
          else
            FATAL2 ("Bad lsb sidebearing value `%s' in file `%s'",
            	    word, concat (input_name, ".sbi"));

          displacement = xstrdup (strtok (NULL, " \t"));
          CHAR_DISPLACEMENT (*c) = displacement;
          DEBUG_PRINT3 ("\nCHAR-PERCENT: char displacement of character `%c'\
 is: `%s'.\n", encoding_number (encoding_info, encoding_name),		\
CHAR_DISPLACEMENT (*c));
        }
      else
        {
	  FATAL2 ("Found bad command `%s' in file `%s'",
	          word, concat (input_name, ".sbi"));
        }
      free (save_line);
    }
}

