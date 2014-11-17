/* commands.c: the operations we allow.  The routines here are called
   by the toolkit, as ``callbacks'', when certain events happen.

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

#include "xt-common.h"

#include "Bitmap.h"
#include "filename.h"
#include "font.h"
#include "gf.h"
#include "xmessage.h"

#include "char.h"
#include "commands.h"
#include "main.h"

/* The filename of the bitmap font we output.  (-output-file)  */
string output_name = NULL;

/* The names we construct for the output files.  */
static string gf_output_name, tfm_output_name;

/* If the `true' output_names are non-null, we are using a temporary
   output file, and should therefore do a copy when the user exits,
   because the input and output files are the same.  */
static string true_gf_output_name = NULL;
static string true_tfm_output_name = NULL;


static bitmap_type enlarge_bitmap (bitmap_type);




/* Exit the program.  We set the callback up to pass the font name as
   the CLIENT_DATA.  */

void
exit_command (Widget w, XtPointer client_data, XtPointer call_data)
{
  string font_name = client_data;
  
  close_font (font_name);

  if (true_gf_output_name != NULL)
    xrename (gf_output_name, true_gf_output_name);

  if (true_tfm_output_name != NULL)
    xrename (tfm_output_name, true_tfm_output_name);

  exit (0);
}



/* Move to the next character in the font.  CLIENT_DATA is a pointer to
   a `font_type' structure, and CALL_DATA is ignored.  */

void
next_command (Widget w, XtPointer client_data, XtPointer call_data)
{
  unsigned this_char;
  char_type *c = NULL;
  font_type *f = (font_type *) client_data;
    
  for (this_char = FONT_CURRENT_CHARCODE (*f) + 1;
       this_char <= MAX_CHARCODE && c == NULL;
       this_char++)
    c = read_char (*f, this_char);
  
  if (c == NULL)
    x_warning (XtParent (w), "You're at the last character");
  else
    show_char (XtParent (w), f, c);
}


/* Move to the previous character in the font.  The arguments are the
   same as for `next_command'.  */

void
prev_command (Widget w, XtPointer client_data, XtPointer call_data)
{
  int this_char;
  char_type *c = NULL;
  font_type *f = (font_type *) client_data;
    
  for (this_char = FONT_CURRENT_CHARCODE (*f) - 1;
       this_char >= 0 && c == NULL;
       this_char--)
    c = read_char (*f, this_char);
  
  if (c == NULL)
    x_warning (XtParent (w), "You're at the first character");
  else
    show_char (XtParent (w), f, c);
}


/* Go back to the image that is in the font file, killing all changes.
   Since we made a copy of the bitmap before making changes, we free the
   storage that the copy is occupying.  It would be nice if we could
   change the modified status of the font, but since we don't keep a
   record, we don't know whether this character is the only one that has
   changed.  */

void
revert_command (Widget w, XtPointer client_data, XtPointer call_data)
{
  char_type *c;
  font_type *f = (font_type *) client_data;
  charcode_type code = FONT_CURRENT_CHARCODE (*f);
  char_type *original = FONT_CHAR (*f, code);
  
  /* Set the pointer in the font structure to null, so `read_char' will
     have to reread the character.  */
  FONT_CHAR (*f, code) = NULL;
  
  /* Reread the character.  */
  c = read_char (*f, code);
  if (c == NULL)
    FATAL2 ("xbfe: Character %d has somehow been deleted from `%s'",
            FONT_CURRENT_CHARCODE (*f), FONT_NAME (*f));

  show_char (XtParent (w), f, c);

  /* Release the storage the current character is using.  We must do
     this after rereading the character from the font, so that the same
     memory is not used.  If that happens, `bitmap_set_values' thinks
     nothing has changed.  */
  free_bitmap (&BCHAR_BITMAP (*original));
  free (original);
}


/* This is invoked when the user hits return (well, when the
   `AcceptValue' action is executed) in the Item widget that allows
   changing the current character.  CLIENT_DATA is a pointer to a
   `font_type' structure.  CALL_DATA is the value string in the item
   widget which the user typed; we hope it parses as a character code.  */

void
char_change_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
  charcode_type charcode; 
  string value_string = call_data;
  
  if (XTPARSE_CHARCODE (charcode, value_string, w))
    {
      font_type *f = (font_type *) client_data;
      char_type *c = read_char (*f, charcode);

      if (c == NULL)
        x_warning (XtParent (w), "No such character");
      else
        show_char (XtParent (w), f, c);
    }
}



/* Change the side bearing values.  */

/* We allow the user to give just simple decimal constants.  If we see
   something else, give a warning.  */
#define CHECK_DECIMAL(widget, str)					\
  if (strspn ((str) + (*(str) == '-'), "0123456789")			\
      != strlen (str) - (*(str) == '-'))				\
    {									\
      string s = concat3 ("`", str, "': invalid integer");		\
      x_warning (XtParent (widget), s);					\
      free (s);								\
      return;								\
    }

/* We have to do some things the same when either of the side bearings
   changes.  We change the character C according to the side-bearing
   change of OFFSET pixels.  Why did I make this a macro?  */
#define COMMON_CHANGE(f, c, offset)					\
  {									\
    char_info_type *char_info = &CHAR_BITMAP_INFO (*c);			\
    real ds = BITMAP_FONT_DESIGN_SIZE (FONT_BITMAP_FONT (FONT_INFO (*f)));\
    real pt_width = fix_to_real (CHAR_TFM_WIDTH (*char_info) * ds);	\
    real pt_offset = PIXELS_TO_POINTS (offset, FONT_DPI (*f));		\
    real new_tfm_width = pt_width + pt_offset;				\
    TFM_WIDTH (CHAR_TFM_INFO (*c)) = new_tfm_width;			\
    CHAR_TFM_WIDTH (*char_info) = real_to_fix (new_tfm_width / ds);	\
    CHAR_SET_WIDTH (*char_info) += offset;				\
    FONT_MODIFIED (*f) = true;						\
  }

void
lsb_change_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
  int new_lsb, old_lsb;
  string value_string = call_data;
  font_type *f = (font_type *) client_data;
  char_type *c = FONT_CURRENT_CHAR (*f);

  assert (c != NULL);
  
  CHECK_DECIMAL (w, value_string);

  old_lsb = CHAR_LSB (CHAR_BITMAP_INFO (*c));
  new_lsb = atoi (value_string);
  CHAR_MIN_COL (CHAR_BITMAP_INFO (*c)) = new_lsb;
  CHAR_MAX_COL (CHAR_BITMAP_INFO (*c)) += new_lsb - old_lsb;
  
  COMMON_CHANGE (f, c, new_lsb - old_lsb);
}


void
rsb_change_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
  int new_rsb, old_rsb;
  string value_string = call_data;
  font_type *f = (font_type *) client_data;
  char_type *c = FONT_CURRENT_CHAR (*f);

  assert (c != NULL);
  
  CHECK_DECIMAL (w, value_string);
  
  old_rsb = CHAR_RSB (CHAR_BITMAP_INFO (*c));
  new_rsb = atoi (value_string);

  COMMON_CHANGE (f, c, new_rsb - old_rsb);
}



/* Expand the bitmap by one pixel on all sides.  */

void
expand_command (Widget w, XtPointer client_data, XtPointer call_data)
{
  font_type *f = (font_type *) client_data;
  /* We must use `*bitmap' here, since the editable Bitmap is a child of
     the Viewport.  Incidentally, R3's XtNameToWidget won't work here.  */
  Widget bw = XtNameToWidget (XtParent (w), "*bitmap");
  bitmap_type *b = BitmapBits (bw);
  char_type *c = XTALLOC (1, char_type);
  
  /* Copy the current character, so the `*_set_values' commands can
     notice something is changed.  */
  *c = *FONT_CURRENT_CHAR (*f);

  /* Do the enlargement.  */
  BCHAR_BITMAP (*c) = enlarge_bitmap (*b);
  
  /* We have to increase the bounding box for the character.  The best
     way to do this is not obvious.  Right now, we arrange things so
     that the origin of the character does not move, i.e., the new row
     at the bottom is below the baseline, and the new column at the left
     is into the side bearing.  Perhaps we should have four separate
     expand commands (or something along those lines).  */
  BCHAR_SET_WIDTH (*c) += 2;
  BCHAR_MIN_COL (*c)--;
  BCHAR_MAX_COL (*c)++;
  BCHAR_MIN_ROW (*c)--;
  BCHAR_MAX_ROW (*c)++;

  /* Display the new values.  */
  show_char (XtParent (w), f, c);

  /* Free afterwards; see revert_command.  */
  free_bitmap (b);
}


/* Expand the bitmap B by one pixel on all four sides.  We return a new
   bitmap.  */

static bitmap_type
enlarge_bitmap (bitmap_type b)
{
  bitmap_type new;
  dimensions_type new_size;
  unsigned row;
  
  DIMENSIONS_WIDTH (new_size) = BITMAP_WIDTH (b) + 2;
  DIMENSIONS_HEIGHT (new_size) = BITMAP_HEIGHT (b) + 2;
  
  new = new_bitmap (new_size);
  
  /* `new_bitmap' initializes all the pixels to white, so we don't have
     to do anything to the new rows and columns.  */
  for (row = 1; row < DIMENSIONS_HEIGHT (new_size) - 1; row++)
    {
      one_byte *source = BITMAP_ROW (b, row - 1);
      one_byte *target = BITMAP_ROW (new, row) + 1;
      
      memcpy (target, source, BITMAP_WIDTH (b));
    }
  
  return new;
}



/* Save the font (if it's been modified).  The CLIENT_DATA is a pointer
   to a `font_type' structure, as defined in `main.h'.  */

/* Write C, a `char_info_type', to the output file.  */
#define OUTPUT_CHAR(c)							\
  do									\
    {									\
      gf_char_type gf_char;						\
									\
      GF_CHARCODE (gf_char) = this_char;				\
      GF_BITMAP (gf_char) = BCHAR_BITMAP (c);				\
      GF_CHAR_BB (gf_char) = BCHAR_BB (c);				\
      GF_H_ESCAPEMENT (gf_char) = BCHAR_SET_WIDTH (c);			\
      GF_TFM_WIDTH (gf_char) = BCHAR_TFM_WIDTH (c);			\
      gf_put_char (gf_char);						\
      									\
      tfm_put_char (CHAR_TFM_INFO (c));					\
    }									\
  while (0)


/* We put this in the font comment to identify ourselves.  */
#define OUR_PREFIX "Hand edited"

void
save_command (Widget w, XtPointer client_data, XtPointer call_data)
{
  string font_comment, our_comment;
  string pl_output_name;
  unsigned this_char;
  font_type *f_ptr = (font_type *) client_data;
  font_type f = *f_ptr;
  bitmap_font_type bitmap_font = FONT_BITMAP_FONT (FONT_INFO (f));
  Widget bw = XtNameToWidget (XtParent (w), "*bitmap");

  if (!(FONT_MODIFIED (f) || BitmapModified (bw)))
    {
      x_warning (XtParent (w),
                 "You didn't change the font, so I won't write it out");
      return;
    }
  
  /* The output file is named the same thing as the input file (except
     we always output in GF format), unless the user has specified an
     output name.  */
  if (output_name == NULL)
    output_name = basename (FONT_NAME (f));

  gf_output_name = concat4 (output_name, ".", utoa ((unsigned) FONT_DPI (f)),
                            "gf");
  pl_output_name = concat (output_name, ".pl");
  tfm_output_name = concat (output_name, ".tfm");

  /* Since the user can go on editing after a save, however, we must
     make sure not to overwrite either of the input files.  If they do
     want to end up overwriting, we use a temporary file for writing.
     Then, when the user exits, we'll copy the temporary back to the
     real file.  */
  if (same_file_p (BITMAP_FONT_FILENAME (bitmap_font), gf_output_name))
    {
      string temp_dir = getenv ("TMPDIR") ? : "/tmp";
      true_gf_output_name = gf_output_name;
      gf_output_name = concat5 (temp_dir, "/", itoa (getpid ()),
                                basename (output_name), "gf"); 
    }
  if (same_file_p (FONT_TFM_FILENAME (FONT_INFO (f)), tfm_output_name))
    {
      string temp_dir = getenv ("TMPDIR") ? : "/tmp";
      true_tfm_output_name = tfm_output_name;
      tfm_output_name = concat4 (temp_dir, "/", itoa (getpid ()), ".tfm");
      pl_output_name = concat4 (temp_dir, "/", itoa (getpid ()), ".pl");
    }

  /* Open the output files we will write to.  */
  if (!gf_open_output_file (gf_output_name))
    FATAL_PERROR (gf_output_name);
  if (!tfm_open_pl_output_file (pl_output_name))
    FATAL_PERROR (pl_output_name);
    
  /* Don't keep tacking on our prefix to the beginning of the comment;
     instead, replace it if it's already there.  Also, don't bother to
     put the day of the week in the comment.  */
  our_comment = concat3 (OUR_PREFIX " ", now () + 4, " from ");
  font_comment = FONT_COMMENT (FONT_INFO (f));
  if (strncmp (font_comment, OUR_PREFIX, strlen (OUR_PREFIX)) == 0)
    font_comment += strlen (our_comment);

  /* Output the stuff at the beginning.  */
  gf_put_preamble (concat (our_comment, font_comment));
  tfm_put_global_info (FONT_TFM_FONT (FONT_INFO (f)));

  for (this_char = 0; this_char <= MAX_CHARCODE; this_char++)
    {
      char_type *c = FONT_CHAR (f, this_char);
      
      /* First see if we've read this character, i.e., if the user has
         edited (well, at least selected) it.  */
      if (c != NULL)
        OUTPUT_CHAR (*c);
      else
        { 
          tfm_char_type *tfm_char;
          
          /* The character hasn't been seen, so now we need to see if
             it's in the font.  If it is, though, there is no point in
             doing all the work of decoding and then re-encoding the
             bitmap, so we just read the raw bytes.  */
          raw_char_type *c = get_raw_char (FONT_NAME (f), this_char);
          if (c != NULL && RAW_CHAR_BITMAP_FORMAT (*c) == gf_format)
            {
              /* We found it in the font, although the user hasn't seen
                 it yet.  We just write out the same bytes that were
                 already present in the font file.  */
              gf_put_raw_char (*c);
              free_raw_char (c);
              
              /* Also output the TFM character, just as we read it.  */
              tfm_char = tfm_get_char (this_char);
              if (tfm_char != NULL)
                tfm_put_char (*tfm_char);
            }
          else
            { /* It's not available as a raw character in the right
                 format.  We still have to test if it's available at all.  */
              char_type *c = read_char (f, this_char);
              if (c != NULL)
                {
                  OUTPUT_CHAR (*c);
                  
                  /* We might as well save the character away, to avoid
                     reconstructing the bitmap if they either visit the
                     character or just save again.  */
                  FONT_CHAR (f, this_char) = c;
                }
            }
        }
    }

  gf_put_postamble (real_to_fix (FONT_DESIGN_SIZE (FONT_INFO (f))),
                    FONT_DPI (f), FONT_DPI (f));

  /* Make the TFM file from the PL file.  */
  tfm_convert_pl (false);
  
  /* Close the output files, since we will write them again from scratch
     next time.  */
  gf_close_output_file ();
  tfm_close_pl_output_file ();
}
