/* tfm_util.c: routines independent of reading or writing a particular
   font.

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

#include "global.h"
#include "list.h"
#include "tfm.h"


static void tfm_set_font_parameter (unsigned parameter, real value, 
			            tfm_global_info_type *tfm_info);
                             
static void tfm_set_fontsize (tfm_global_info_type *tfm_info);


/* A constructor for the TFM character type.  */

tfm_char_type
tfm_new_char ()
{
  tfm_char_type ch;

  ch.code = 0;
  ch.width = ch.height = ch.depth = ch.italic_correction = 0.0;
  ch.fix_width = ch.fix_height = ch.fix_depth = ch.fix_italic_correction = 0;

  ch.ligature = new_list ();
  ch.kern = new_list ();

  ch.exists = false;

  return ch;
}


/* Return an initialized array of `tfm_char_type's.  */

tfm_char_type *
tfm_new_chars ()
{
  unsigned i;
  tfm_char_type *chars = XTALLOC (TFM_SIZE, tfm_char_type);
  
  for (i = 0; i < TFM_SIZE; i++)
    chars[i] = tfm_new_char ();
  
  return chars;
}



/* The character RIGHT may or may not be in the list of kerns already.  */

void
tfm_set_kern (tfm_char_type *left, charcode_type right, real k)
{
  unsigned this_right;
  tfm_kern_type *new_kern;
  list_type kern_table;
  
  assert (left != NULL && TFM_CHAR_EXISTS (*left));
  
  kern_table = left->kern;

  for (this_right = 0; this_right < LIST_SIZE (kern_table); this_right++)
    {
      tfm_kern_type *kern = LIST_ELT (kern_table, this_right);

      if (kern->character == right)
	{			/* Already there.  */
	  kern->kern = k;
	  return;
	}
    }

  /* RIGHT wasn't in the existing list.  Add it to the end.  */
  new_kern = append_element (&(left->kern), sizeof (tfm_kern_type));
  new_kern->character = right;
  new_kern->kern = k;
}


/* Find the kern between the characters LEFT and RIGHT.  (Return zero if
   none such.)  */ 

real
tfm_get_kern (tfm_char_type left, charcode_type right)
{
  list_type kern_table;
  unsigned r;
  
  if (!TFM_CHAR_EXISTS (left))
    return 0.0;
  
  kern_table = left.kern;

  for (r = 0; r < LIST_SIZE (kern_table); r++)
    {
      tfm_kern_type *kern = LIST_ELT (kern_table, r);

      if (kern->character == right)
	return kern->kern;
    }

  return 0.0;
}



/* The character RIGHT may or may not be in the list of ligatures already.  */

void
tfm_set_ligature (tfm_char_type *left, charcode_type right, charcode_type l)
{
  unsigned this_right;
  tfm_ligature_type *new_ligature;
  list_type ligature_table;
  
  assert (left != NULL && TFM_CHAR_EXISTS (*left));
  
  ligature_table = left->ligature;

  for (this_right = 0; this_right < LIST_SIZE (ligature_table); this_right++)
    {
      tfm_ligature_type *ligature = LIST_ELT (ligature_table, this_right);

      if (ligature->character == right)
	{
	  ligature->ligature = l;
          return;
        }
    }

  /* RIGHT wasn't in the existing list.  Add it to the end.  */
  new_ligature 
    = append_element (&(left->ligature), sizeof (tfm_ligature_type));
  new_ligature->character = right;
  new_ligature->ligature = l;
}



/* Set the header in TFM_INFO according to the string S.  */
   
void
tfm_set_header (string s, tfm_global_info_type *tfm_info)
{
  string spec;

  /* If S is empty, we have nothing more to do.  */
  if (s == NULL || *s == 0)
    return;
  
  /* Parse the specification string.  */
  for (spec = strtok (s, ","); spec != NULL; spec = strtok (NULL, ","))
    {
      string header_item;
      string value_string = strchr (spec, ':');
      
      if (value_string == NULL)
        FATAL1 ("TFM headers look like `<header-item>:<value>', not `%s'",
	         spec);

      header_item = substring (spec, 0, value_string - spec - 1);
            
      if (STREQ (header_item, "checksum"))
	TFM_CHECKSUM (*tfm_info) = atoi (value_string + 1);

      else if (STREQ (header_item, "designsize"))
	{
          real design_size = atof (value_string + 1);
	  
          tfm_set_design_size (design_size, tfm_info);
        }
      else if (STREQ (header_item, "codingscheme"))
	{
          unsigned length = strlen (value_string + 1);
	  
          if (strchr (value_string + 1, '(') != NULL
	      || strchr (value_string + 1, '(') != NULL)
            FATAL ("Your coding scheme cannot have any parentheses in it");
        
          if (length > TFM_MAX_CODINGSCHEME_LENGTH)
            WARNING2 ("Your coding scheme string of length %d will be \
truncated to %d", length, TFM_MAX_CODINGSCHEME_LENGTH);
	  
	  TFM_CODING_SCHEME (*tfm_info) = xstrdup (value_string + 1);
        }
    }
}



/* Set the font parameter entries in TFM_INFO according to the string S.
   If S is non-null and non-empty, it should look like
   <fontdimen>:<real>,<fontdimen>:<real>,..., where each <fontdimen> is
   either a standard name or a number between 1 and 30 (the 30 comes
   from the default value in PLtoTF, it's not critical).  

   Also, if the `design_size' member of TFM_INFO is nonzero, we set the
   `FONTSIZE' parameter to 1.0 (meaning it's equal to the design size),
   unless the `FONTSIZE' parameter is already set.  */
   

/* This structure maps font parameter names to numbers; most of the
   information comes from the PLtoTF utility program.  */

typedef struct
{
  string name;
  unsigned number;
} font_param_type;


void
tfm_set_fontdimens (string s, tfm_global_info_type *tfm_info)
{
  static font_param_type font_param[] = {
    { "slant",			TFM_SLANT_PARAMETER },
    { "space",			TFM_SPACE_PARAMETER },
    { "stretch",		TFM_STRETCH_PARAMETER },
    { "shrink",			TFM_SHRINK_PARAMETER },
    { "xheight",		TFM_XHEIGHT_PARAMETER },
    { "quad",			TFM_QUAD_PARAMETER },
    { "extraspace",		TFM_EXTRASPACE_PARAMETER },
    { "num1",			TFM_NUM1_PARAMETER },
    { "num2",			TFM_NUM2_PARAMETER },
    { "num3",			TFM_NUM3_PARAMETER },
    { "denom1",			TFM_DENOM1_PARAMETER },
    { "denom2",			TFM_DENOM2_PARAMETER },
    { "sup1",			TFM_SUP1_PARAMETER },
    { "sup2",			TFM_SUP2_PARAMETER },
    { "sup3",			TFM_SUP3_PARAMETER },
    { "sub1",			TFM_SUB1_PARAMETER },
    { "sub2",			TFM_SUB2_PARAMETER },
    { "supdrop",		TFM_SUPDROP_PARAMETER },
    { "subdrop",		TFM_SUBDROP_PARAMETER },
    { "delim1",			TFM_DELIM1_PARAMETER },
    { "delim2",			TFM_DELIM2_PARAMETER },
    { "axisheight",		TFM_AXISHEIGHT_PARAMETER },
    { "defaultrulethickness",	TFM_DEFAULTRULETHICKNESS_PARAMETER },
    { "bigopspacing1",		TFM_BIGOPSPACING1_PARAMETER },
    { "bigopspacing2",		TFM_BIGOPSPACING2_PARAMETER },
    { "bigopspacing3",		TFM_BIGOPSPACING3_PARAMETER },
    { "bigopspacing4",		TFM_BIGOPSPACING4_PARAMETER },
    { "bigopspacing5",		TFM_BIGOPSPACING5_PARAMETER },
    { "leadingheight",		TFM_LEADINGHEIGHT_PARAMETER },
    { "leadingdepth",		TFM_LEADINGDEPTH_PARAMETER },
    { "fontsize",		TFM_FONTSIZE_PARAMETER },
    { "version",		TFM_VERSION_PARAMETER },
  };

  string spec;
  
  /* If S is empty, we have nothing more to do.  */
  if (s == NULL || *s == 0)
    return;
  
  /* Parse the specification string.  */
  for (spec = strtok (s, ","); spec != NULL; spec = strtok (NULL, ","))
    {
      string fontdimen;
      real value;
      unsigned param_number = 0;
      string value_string = strchr (spec, ':');
      
      if (value_string == NULL)
        FATAL1 ("Fontdimens look like `<fontdimen>:<real>', not `%s'",
                spec);

      value = atof (value_string + 1);
      fontdimen = substring (spec, 0, value_string - spec - 1);
      
      /* If `fontdimen' is all numerals, we'll take it to be an integer.  */
      if (strspn (fontdimen, "0123456789") == strlen (fontdimen))
        {
          param_number = atou (fontdimen);

          if (param_number == 0 || param_number > TFM_MAX_FONT_PARAMETERS)
            FATAL2 ("Font parameter %u is not between 1 and the maximum (%u)",
                    param_number, TFM_MAX_FONT_PARAMETERS);
        }
      else
        { /* It wasn't a number; see if it's a valid name.  */
          unsigned this_param;
          
          for (this_param = 0;
               this_param < TFM_MAX_FONT_PARAMETERS && param_number == 0;
               this_param++)
            if (STREQ (fontdimen, font_param[this_param].name))
              param_number = font_param[this_param].number;
          
          if (param_number == 0)
            FATAL1 ("I don't know the font parameter name `%s'", fontdimen);
        }
      
      /* If we got here, `param_number' is the number of the font
         parameter we are supposed to assign to.  */

      tfm_set_font_parameter (param_number, value, tfm_info);
      
      /* If `param_number' is past the last fontdimen previously
         assigned, we have to assign zero to all the intervening ones.
         But this can never happen, because we always assign the
         `fontsize' parameter, which happens to be the last one.  */
      assert (param_number <= TFM_FONT_PARAMETER_COUNT (*tfm_info));
    }
}


/* Set the design and font size of TFM_INFO to DESIGN_SIZE.  */

void
tfm_set_design_size (real design_size, tfm_global_info_type *tfm_info)
{
  TFM_CHECK_DESIGN_SIZE (design_size);
  TFM_DESIGN_SIZE (*tfm_info) = design_size;
  tfm_set_fontsize (tfm_info);
}



/* Set the PARAMETER-th font parameter of TFM_INFO to VALUE.  */

static void
tfm_set_font_parameter (unsigned parameter, real value, 
		    tfm_global_info_type *tfm_info)
{
  /* Set all the intervening parameters to zero.  */
  
  if (TFM_FONT_PARAMETER_COUNT (*tfm_info) < parameter)
    {
      unsigned p;
      
      for (p = TFM_FONT_PARAMETER_COUNT (*tfm_info) + 1; p < parameter; p++)
        TFM_FONT_PARAMETER (*tfm_info, p) = 0.0;
      
      /* Now we have more parameters.  */
      TFM_FONT_PARAMETER_COUNT (*tfm_info) = parameter;
    }

  TFM_FONT_PARAMETER (*tfm_info, parameter) = value;
}


/* Set the `FONTSIZE' parameter of TFM_INFO, if the design size is set.  */

static void
tfm_set_fontsize (tfm_global_info_type *tfm_info)
{
  if (TFM_DESIGN_SIZE (*tfm_info) != 0.0)
    tfm_set_font_parameter (TFM_FONTSIZE_PARAMETER, 
			    TFM_DESIGN_SIZE (*tfm_info), tfm_info);
}
