/* filename.c: routines to manipulate filenames.

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
#include "paths.h"

#include "filename.h"
#include "pathsrch.h"


/* Try to find the PK font FONT_NAME at resolution DPI, using various
   paths (see `filename.h').  The filename must have the resolution as
   part of the extension, e.g., `cmr10.300pk', for it to be found.  */
   
string
find_pk_filename (string font_name, unsigned dpi)
{
  static string *dirs = NULL;
  string pk_var, pk_name, name;
  char suffix[MAX_INT_LENGTH + 3];

  sprintf (suffix, "%dpk", dpi);

  pk_var = getenv ("PKFONTS") ? "PKFONTS"
           : getenv ("TEXPKS") ? "TEXPKS" : "TEXFONTS";

  if (dirs == NULL)
    dirs = initialize_path_list (pk_var, DEFAULT_PK_PATH, true);

  name = make_suffix (font_name, suffix);
  pk_name = find_path_filename (name, dirs);

  if (name != pk_name)
    free (name);
  
  return pk_name;
}


/* Like `find_pk_filename', but search for a font in GF format.  */

string
find_gf_filename (string font_name, unsigned dpi)
{
  static string *dirs = NULL;
  string gf_var, gf_name, name;
  char suffix[MAX_INT_LENGTH + 3];

  sprintf (suffix, "%dgf", dpi);

  gf_var = getenv ("GFFONTS") ? "GFFONTS" : "TEXFONTS";
  
  if (dirs == NULL)
    dirs = initialize_path_list (gf_var, DEFAULT_GF_PATH, true);

  name = make_suffix (font_name, suffix);
  gf_name = find_path_filename (name, dirs);

  if (name != gf_name)
    free (name);
    
  return gf_name;
}


/* Like `find_pk_filename', except search for a TFM file.  */

string
find_tfm_filename (string font_name)
{
  static string *dirs = NULL;
  string tfm_name, name;

  if (dirs == NULL)
    dirs = initialize_path_list ("TEXFONTS", DEFAULT_TFM_PATH, true);

  name = make_suffix (font_name, "tfm");
  tfm_name = find_path_filename (name, dirs);

  if (name != tfm_name)
    free (name);
    
  return tfm_name; 
}



/* See filename.h for the specifications for the following routines.  */

string
find_suffix (string name)
{
  string dot_pos = strrchr (name, '.');
  string slash_pos = strrchr (name, '/');
  
  /* If the name is `foo' or `/foo.bar/baz', we have no extension.  */
  return
    dot_pos == NULL || dot_pos < slash_pos
    ? NULL
    : dot_pos + 1;
}


string
make_suffix (string s, string new_suffix)
{
  string new_s;
  string old_suffix = find_suffix (s);

  if (old_suffix == NULL)
    new_s = concat3 (s, ".", new_suffix);
  else
    {
      unsigned length_through_dot = old_suffix - s;
      
      new_s = xmalloc (length_through_dot + strlen (new_suffix) + 1);
      strncpy (new_s, s, length_through_dot);
      strcpy (new_s + length_through_dot, new_suffix);
    }

  return new_s;
}


string
remove_suffix (string s)
{
  string suffix = find_suffix (s);
  
  return suffix == NULL ? xstrdup (s) : substring (s, 0, suffix - 2 - s);
}


string
make_prefix (string prefix, string pathname)
{
  string slash_pos = strrchr (pathname, '/');
  
  if (slash_pos == NULL)
    return concat (prefix, pathname);
  else
    {
      pathname[slash_pos - pathname] = 0;
      return concat4 (pathname, "/", prefix, slash_pos + 1);
    }
}


string
make_stem_suffix (string name, string stem_suffix)
{
  string suffix = find_suffix (name);
  
  name = remove_suffix (name);
  
  return suffix == NULL
  	 ? concat (name, stem_suffix)
         : concat4 (name, stem_suffix, ".", suffix);
}


string
make_output_filename (string name, string default_suffix)
{
  string new_s;
  string suffix = find_suffix (name);

  new_s = suffix == NULL 
          ? concat3 (name, ".", default_suffix) 
          : xstrdup (name);
  return new_s;
}

