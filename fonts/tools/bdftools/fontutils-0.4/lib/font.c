/* font.c: define (more or less) format-independent font operations.

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

#include <ctype.h>

#include "filename.h"
#include "font.h"
#include "gf.h"
#include "list.h"
#include "pk.h"
#include "tfm.h"


/* We want to allow reading of multiple fonts, so we keep some
   information around for each font we read.  */

typedef struct
{
  bitmap_format_type bitmap_format;
  boolean bitmap_only;
  font_info_type info;
} internal_font_type;

/* The format of the bitmap file for this font.  */
#define INTERNAL_BITMAP_FORMAT(i)  ((i).bitmap_format)

/* If the font was opened with `get_bitmap_font', this field is true;
   if with `get_font', false.  */
#define INTERNAL_BITMAP_ONLY(i)  ((i).bitmap_only)

/* The filename for the bitmap font.  */
#define INTERNAL_BITMAP_NAME(i)  ((i).bitmap_name)

/* The fontwide information.  The parts of this that are found in the
   TFM file are garbage if INTERNAL_BITMAP_ONLY (I) is true.  */
#define INTERNAL_FONT_INFO(i)  ((i).info)


static void delete_internal_font (string filename);
static internal_font_type *find_internal_font (string font_name);
static void save_internal_font (string font_name, internal_font_type);




/* Starting with FONT_NAME as the base filename, e.g., `cmr10', we try
   various extensions to find a PK or GF file at resolution DPI.  This
   bitmap font defines the shapes of the characters.  */
   
bitmap_font_type
get_bitmap_font (string font_name, unsigned dpi)
{
  bitmap_font_type font;
  internal_font_type internal_f;
  internal_font_type *internal_font_ptr;
  string pk_name, gf_name;
  boolean found_pk = false, found_gf = false;

  /* If we are passed a null pointer or the empty string, it's got to be
     a mistake.  */
  assert (font_name && *font_name);

  /* If we already have the font saved, we just return it, thus (1) saving
     going out to the file system, and (2) causing multiple calls with
     the same FONT_NAME not to open more and more bitmap files.  */
  internal_font_ptr = find_internal_font (font_name); 
  if (internal_font_ptr != NULL)
    return FONT_BITMAP_FONT (INTERNAL_FONT_INFO (*internal_font_ptr));

  pk_name = find_pk_filename (font_name, dpi);
  if (pk_name != NULL)
    { /* Found the PK file, so unless something strange is happening
         (like the file being removed between the `find_path_filename'
         call and the `pk_open_input_file' call), we will be able to
         open it.  */
      found_pk = pk_open_input_file (pk_name);
      if (!found_pk)
        FATAL_PERROR (pk_name);
    }
  else if ((gf_name = find_gf_filename (font_name, dpi)) != NULL)
    {
      found_gf = gf_open_input_file (gf_name);
      if (!found_gf)
        {
          perror (gf_name);
          FATAL1 ("(I couldn't find the PK file `%s', either.)", pk_name);
        }
    }
  else
    FATAL2 ("%s.%d{gf,pk}: Nowhere in path", font_name, dpi);

    
  /* We've opened the files; get the information we need to return.  */
  if (found_pk)
    {
      pk_preamble_type preamble = pk_get_preamble (pk_name);

      BITMAP_FONT_DESIGN_SIZE (font)
        = fix_to_real (PK_DESIGN_SIZE (preamble));
      BITMAP_FONT_COMMENT (font) = PK_COMMENT (preamble);
      BITMAP_FONT_CHECKSUM (font) = PK_CHECKSUM (preamble);
      BITMAP_FONT_FILENAME (font) = pk_name;

      INTERNAL_BITMAP_FORMAT (internal_f) = pk_format;
    }
  else if (found_gf)
    {
      gf_postamble_type postamble = gf_get_postamble ();

      BITMAP_FONT_DESIGN_SIZE (font)
        = fix_to_real (GF_DESIGN_SIZE (postamble));
      BITMAP_FONT_COMMENT (font) = gf_get_preamble ();
      BITMAP_FONT_CHECKSUM (font) = GF_CHECKSUM (postamble);
      BITMAP_FONT_FILENAME (font) = gf_name;

      INTERNAL_BITMAP_FORMAT (internal_f) = gf_format;
    }
  else
    FATAL ("No bitmap file found, but I must have found one");

  FONT_BITMAP_FONT (INTERNAL_FONT_INFO (internal_f)) = font;
  INTERNAL_BITMAP_ONLY (internal_f) = true;

  save_internal_font (font_name, internal_f);

  return font;
}


/* Look for both a bitmap font named FONT_NAME, and for the TFM file
   FONT_NAME.tfm.  We call `get_bitmap_font' to find the bitmap font. 
   For the TFM file, we use the path in the environment variable
   TEXFONTS; if TEXFONTS isn't set, we use the default defined above.
   
   If FONT_NAME was previously opened with `get_bitmap_font', we give a
   fatal error.  */

font_info_type
get_font (string font_name, unsigned dpi)
{
  unsigned bitmap_checksum;
  unsigned c;
  font_info_type font;
  internal_font_type internal_f;
  internal_font_type *internal_font_ptr;
  tfm_char_type *tfm_char;
  unsigned tfm_checksum;
  string tfm_name;

  /* If we are passed a null pointer or the empty string, it's got to be
     a mistake.  */
  assert (font_name != NULL && *font_name);

  /* If we already have the font saved, we just return it, thus (1) saving
     going out to the file system, and (2) causing multiple calls with
     the same FONT_NAME not to open more and more bitmap files.  */
  internal_font_ptr = find_internal_font (font_name); 
  if (internal_font_ptr != NULL)
    if (INTERNAL_BITMAP_ONLY (*internal_font_ptr))
      FATAL1 ("get_font: `%s' was opened with get_bitmap_font", font_name);
    else
      return INTERNAL_FONT_INFO (*internal_font_ptr);

  /* If we don't have the font saved, we have to look for the TFM file.  */
  tfm_name = find_tfm_filename (font_name);

  if (tfm_name == NULL)
    FATAL1 ("%s.tfm: Nowhere in path", font_name);

  if (!tfm_open_input_file (tfm_name))
    FATAL_PERROR (tfm_name);


  /* The TFM file is opened.  Save some global information.  */
  FONT_TFM_FONT (font) = tfm_get_global_info ();
  FONT_TFM_FILENAME (font) = tfm_name;
  
  /* Read the (we hope) corresponding bitmap file.  */
  FONT_BITMAP_FONT (font) = get_bitmap_font (font_name, dpi);
  
  /* We have found the bitmap font on the filesystem, so it should be in
     our internal list now.  We will overwrite it below.  */
  internal_font_ptr = find_internal_font (font_name);
  assert (internal_font_ptr != NULL);
  
  /* We've opened the files; now get the information we're supposed to
     return.  */

  /* The interword space is in the TFM file; we return it in pixels.  */
  FONT_SPACE (font) = tfm_get_interword_space () * dpi / POINTS_PER_INCH;

  /* To count the number of characters, we count the number of
     characters in the TFM file with nonzero widths.  This is much more
     efficient than reading the bitmap file, since TFM files are much
     smaller.  It is potentially wrong, however, since extensions of TeX
     can handle more than 256 characters per font, and the current
     Metafont has no objections to outputting such a font.  We just flub
     this one.  (Outside the TeX world, fonts with 500 to 1000
     characters are not uncommon, but such fonts are proprietary,
     anyway.)  */
  tfm_char = tfm_get_chars ();
  
  FONT_NCHARS (font) = 0;
  for (c = 0; c < TFM_SIZE; c++)
    FONT_NCHARS (font) += TFM_CHAR_EXISTS (tfm_char[c]);

  /* Let's check the checksums.  */
  tfm_checksum = tfm_get_checksum ();
  bitmap_checksum = BITMAP_FONT_CHECKSUM (FONT_BITMAP_FONT (font));

  if (tfm_checksum != 0 && bitmap_checksum != 0 
      && tfm_checksum != bitmap_checksum)
    WARNING1 ("%s: TFM and bitmap checksums don't match", font_name);

  INTERNAL_FONT_INFO (internal_f) = font;
  INTERNAL_BITMAP_ONLY (internal_f) = false;
  INTERNAL_BITMAP_FORMAT (internal_f)
    = INTERNAL_BITMAP_FORMAT (*internal_font_ptr);
  save_internal_font (font_name, internal_f);

  return font;
}



/* Close the files we have opened, for tidiness.  */

void
close_font (string font_name)
{
  internal_font_type *f = find_internal_font (font_name);

  if (f == NULL)
    FATAL1 ("close_font: Font `%s' not open", font_name);
    
  switch (INTERNAL_BITMAP_FORMAT (*f))
    {
    case pk_format:
      {
        string pk_name
          = BITMAP_FONT_FILENAME (FONT_BITMAP_FONT (INTERNAL_FONT_INFO (*f)));
        pk_close_input_file (pk_name);
      }
      break;
    
    case gf_format:
      gf_close_input_file ();
      break;
    
    default:
      FATAL1 ("Impossible bitmap format (%d)", INTERNAL_BITMAP_FORMAT (*f));
    }

  if (!INTERNAL_BITMAP_ONLY (*f))
    tfm_close_input_file ();
  
  delete_internal_font (font_name);
}



/* Look for the character numbered CODE in the font FONT_NAME.  If it
   doesn't exist, return NULL.  Otherwise, fill in a `char_info_type'
   structure and return a pointer to it. 
   
   This merely calls the get-a-character routine in the appropriate
   library.  */

char_info_type *
get_char (string font_name, one_byte code)
{
  char_info_type *c;
  internal_font_type *f = find_internal_font (font_name);
  
  if (f == NULL)
    FATAL1 ("get_char: Font `%s' not open", font_name);

  switch (INTERNAL_BITMAP_FORMAT (*f))
    {
    case pk_format:
      {
        string pk_name
          = BITMAP_FONT_FILENAME (FONT_BITMAP_FONT (INTERNAL_FONT_INFO (*f)));
        pk_char_type *pk_char = pk_get_char (code, pk_name);
        if (pk_char == NULL) return NULL;
        
        c = XTALLOC (1, char_info_type);
        CHARCODE (*c) = PK_CHARCODE (*pk_char);
        CHAR_SET_WIDTH (*c) = PK_H_ESCAPEMENT (*pk_char);
        CHAR_TFM_WIDTH (*c) = PK_TFM_WIDTH (*pk_char);
        CHAR_BB (*c) = PK_CHAR_BB (*pk_char);
        CHAR_BITMAP (*c) = PK_BITMAP (*pk_char);
      }
      break;
    
    case gf_format:
      {
        gf_char_type *gf_char = gf_get_char (code);
        if (gf_char == NULL) return NULL;

        c = XTALLOC (1, char_info_type);
        CHARCODE (*c) = GF_CHARCODE (*gf_char);
        CHAR_SET_WIDTH (*c) = GF_H_ESCAPEMENT (*gf_char);
        CHAR_TFM_WIDTH (*c) = GF_TFM_WIDTH (*gf_char);
        CHAR_BB (*c) = GF_CHAR_BB (*gf_char);
        CHAR_BITMAP (*c) = GF_BITMAP (*gf_char);
      }
      break;
    
    default:
      FATAL1 ("Impossible bitmap format (%d)", INTERNAL_BITMAP_FORMAT (*f));
    }
  
  return c;
}



/* Look for the character numbered CODE in the font FONT_NAME.  If it
   doesn't exist, return NULL.  Otherwise, fill in a `raw_char_type'
   structure and return a pointer to it. 
   
   This merely calls the corresponding routine in the appropriate
   library.  */

raw_char_type *
get_raw_char (string font_name, one_byte code)
{
  raw_char_type *c;
  internal_font_type *f = find_internal_font (font_name);
  
  if (f == NULL)
    FATAL1 ("get_raw_char: Font `%s' not open", font_name);

  switch (INTERNAL_BITMAP_FORMAT (*f))
    {
    case pk_format:
      c = NULL;
      break;
    
    case gf_format:
      c = gf_get_raw_char (code);
      break;
      
    default:
      FATAL1 ("Impossible bitmap format (%d)", INTERNAL_BITMAP_FORMAT (*f));
    }
  
  if (c != NULL)
    RAW_CHAR_BITMAP_FORMAT (*c) = INTERNAL_BITMAP_FORMAT (*f);
  return c;
}


/* Free the information in the raw character RAW_CHAR, including the
   character itself.  */

void
free_raw_char (raw_char_type *raw_char)
{
  free (RAW_CHAR_BYTES (*raw_char));
  free (raw_char);
}



/* Turn the C string TEXT into a bitmap, taking the characters from the
   font FONT_NAME at resolution DPI.  Any space characters in TEXT turn
   into the natural interword space for this font.  No escapes are
   recognized in TEXT.  We don't close FONT_NAME when we're done, since
   the most common case is to be called multiple times on a single font.  */

bitmap_type
string_to_bitmap (string text, string font_name, unsigned dpi)
{
  bitmap_type b;
  unsigned this_char;
  char_info_type *chars[strlen (text)];
  font_info_type font = get_font (font_name, dpi);
  int width = 0, height = 0, depth = 0;
  int x = 0, y = 0;  /* Our current position.  */

  for (this_char = 0; this_char < strlen (text); this_char++)
    {
      /* Turn any kind of space character into a normal interword space.  */
      if (isspace (text[this_char]))
        width += FONT_SPACE (font);
      else
        {
	  chars[this_char] = get_char (font_name, text[this_char]);
          
          /* If the character doesn't exist in this font, just keep
             going.  */
          if (chars[this_char] == NULL)
            continue;
            
          /* The set width should be equal to the left side bearing plus
             the bitmap width plus the right side bearing.  */
	  width += CHAR_SET_WIDTH (*chars[this_char]);
	  height = MAX (height, CHAR_HEIGHT (*chars[this_char]));
	  depth = MAX (depth, CHAR_DEPTH (*chars[this_char]));
        }
    }

  /* Unless the image is nonnegative both horizontally and vertically,
     it is invisible.  */
  if (width <= 0 || (height + depth <= 0))
    {
      BITMAP_WIDTH (b) = 0;
      BITMAP_HEIGHT (b) = 0;
      BITMAP_BITS (b) = NULL;
      return b;
    }

  /* The image is going to be visible.  */
  b = new_bitmap ((dimensions_type) { height + depth, width });
  
  for (this_char = 0; this_char < strlen (text); this_char++)
    {
      if (isspace (text[this_char]))
        x += FONT_SPACE (font);
      else
        {
          int char_x, char_y;
          char_info_type c;
          
          if (chars[this_char] == NULL)
            continue;
          
          c = *chars[this_char];

          x += CHAR_LSB (c);
          x = MAX (x, 0);  /* In case the lsb was negative.  */

          /* Copy the character image to the bitmap we are building, one
             column at a time, from top to bottom.  */
          for (char_x = 0; char_x < CHAR_BITMAP_WIDTH (c); char_x++, x++)
            for (char_y = 0, y = height - CHAR_HEIGHT (c);
                 char_y < CHAR_BITMAP_HEIGHT (c);
                 char_y++, y++)
              BITMAP_PIXEL (b, y, x)
                = BITMAP_PIXEL (CHAR_BITMAP (c), char_y, char_x);
          
          x += CHAR_RSB (c);
        }
    }

  return b;
}



/* Print the character C to the file F, using ordinary characters.  */

void
print_char (FILE *f, char_info_type c)
{
  unsigned this_row, this_col;
  bitmap_type b = CHAR_BITMAP (c);

  fprintf (f, "Character 0x%x=%u", CHARCODE (c), CHARCODE (c));
  if (isprint (CHARCODE (c)))
    fprintf (f, " (%c)", CHARCODE (c));
  
  fprintf (f, ":\n");
  
  fprintf (f, "   min/max col %d/%d, min/max row %d/%d, set width %d.\n", 
           CHAR_MIN_COL (c), CHAR_MAX_COL (c),
           CHAR_MIN_ROW (c), CHAR_MAX_ROW (c), CHAR_SET_WIDTH (c));
  
  /* We don't call `print_bitmap' because we want to print the Cartesian
     row number, as well as the bitmap row number.  */

  for (this_row = 0; this_row < BITMAP_HEIGHT (b); this_row++)
    {
      for (this_col = 0; this_col < BITMAP_WIDTH (b); this_col++)
        putc (BITMAP_PIXEL (b, this_row, this_col) ? '*' : ' ', f);
      
      fprintf (f, "%3d\t%u\n", CHAR_MAX_ROW (c) - this_row, this_row);
    }
}



/* We want to implement a typical key/value setup: here we are given the
   key (FONT_NAME) and the value (F).  We assign an index number to
   FONT_NAME, and store it and F in parallel lists.  If we are passed a
   FONT_NAME that is already in the list, we overwrite the old value.  */

static list_type font_name_list, internal_font_list;

static void
save_internal_font (string font_name, internal_font_type f)
{
  string *new_name;
  internal_font_type *new_font;
  static boolean first_call = true;
  
  if (first_call)
    { /* Have to construct our lists.  */
      font_name_list = new_list ();
      internal_font_list = new_list ();
      first_call = false;
    }

  new_font = find_internal_font (font_name);
  if (new_font == NULL)
    {
      /* Add another pointer to the end of the lists.  */
      new_name = append_element (&font_name_list, sizeof (string));
      new_font = append_element (&internal_font_list,
                                 sizeof (internal_font_type));

      /* Save the information.  */
      *new_name = xstrdup (font_name);
    }
  
  /* Either update the existing font, or save the new one.  */
  *new_font = f;
}


/* This routine returns the font information previously stored with
   FONT_NAME.  If FONT_NAME hasn't been saved, we return NULL.  */

static internal_font_type *
find_internal_font (string font_name)
{
  unsigned e;

  for (e = 0; e < LIST_SIZE (font_name_list); e++)
    if (STREQ (*(string *) LIST_ELT (font_name_list, e), font_name))
      return (internal_font_type *) LIST_ELT (internal_font_list, e);

  return NULL;
}


/* Remove FILENAME from `font_name_list', by setting the name in
   `font_name_list' to the empty string, so that the element is useless.
   (We don't have any actual memory to free.)  What a kludge.  */

static void
delete_internal_font (string filename)
{
  unsigned e;

  /* We can't use `find_internal_font', since we need to know the list
     element index, not just the internal font information.  */ 
  for (e = 0; e < LIST_SIZE (font_name_list); e++)
    if (STREQ (*(string *) LIST_ELT (font_name_list, e), filename))
      {
        string *name = LIST_ELT (font_name_list, e);
        internal_font_type *i
          = (internal_font_type *) LIST_ELT (internal_font_list, e);

        free (*name);
        *name = "";
        
        free (i);

        return;
      }

  FATAL1 ("The font `%s' hasn't been saved", filename);
}
