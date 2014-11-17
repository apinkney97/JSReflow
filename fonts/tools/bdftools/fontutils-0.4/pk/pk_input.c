/* pk_input.c: read from any number of PK files.  The basic idea is to
   read the entire file into memory the first time the client asks for a
   character.  Since PK files tend to be quite small, this does not use
   an enormous amount of memory.  But we only bother to unpack a
   character's bitmap when the client asks for that particular
   character.

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
#include "list.h"
#include "pk.h"
#include "pk_opcodes.h"


/* The `packed_char_type' structure holds the character description,
   taken directly from the input file, before it is unpacked into a
   bitmap.  This type is only used as part of the `internal_font_type'
   structure, above.  (In contrast, the `pk_char_type' structure holds
   the unpacked information, in the form convenient for clients.)  */

typedef struct
{
  unsigned dyn_f : 4;
  unsigned black_first : 1;
  unsigned parameter_length : 3;
  unsigned packet_length;
  one_byte *data;
} packed_char_type;

/* This is the number of one-nybble run counts.  It is at most 14.  */
#define PACKED_DYN_F(c)  ((c).dyn_f)

/* This says whether the first run count is black or white.  */
#define PACKED_BLACK_FIRST(c) ((c).black_first)

/* This says how long the size parameters in the character preamble are.
   It is either 1, 2, or 4.  Either 1 or 2 means we're using a ``short
   format''; 4 means we're using the ``long format''.  */
#define PACKED_PARAM_LENGTH(c)  ((c).parameter_length)
#define LONG_FORMAT(c)  (PACKED_PARAM_LENGTH (c) == 4)

/* This is the length of the packed character data, in bytes.  */
#define PACKED_DATA_LENGTH(c)  ((c).packet_length)

/* And the packed data, as an array of `packet_length' bytes.  */
#define PACKED_DATA(c)  ((c).data)


/* We save an instance of this structure for each font we are asked to
   open.  */

typedef struct
{
  FILE *file;
  pk_char_type *chars[MAX_CHARCODE + 1];
  packed_char_type *packed_chars[MAX_CHARCODE + 1];
  boolean chars_read;
} internal_font_type;

/* The file pointer for the font F.  */
#define INTERNAL_FILE(f)  ((f).file)

/* This pointer is null if the character C in the internal font F has
   not yet been unpacked.  */
#define INTERNAL_PK_CHAR(f, c)  ((f).chars[c])

/* This pointer is null if the character C does not exist in F.  */
#define INTERNAL_PACKED_CHAR(f, c)  ((f).packed_chars[c])

/* This remembers whether we have read the character info from the file.  */
#define INTERNAL_CHARS_READ(f)  ((f).chars_read)


/* These macros get at the members of the packed character numbered C,
   in the internal font F.  */

#define INTERNAL_PACKED_DYN_F(f, c)					\
  PACKED_DYN_F (*INTERNAL_PACKED_CHAR (f, c))
#define INTERNAL_PACKED_BLACK_FIRST(f, c)				\
  PACKED_BLACK_FIRST (*INTERNAL_PACKED_CHAR (f, c))
#define INTERNAL_PACKED_PARAM_LENGTH(f, c)				\
  PACKED_PARAM_LENGTH (*INTERNAL_PACKED_CHAR (f, c))
#define INTERNAL_PACKED_DATA_LENGTH(f, c)				\
  PACKED_DATA_LENGTH (*INTERNAL_PACKED_CHAR (f, c))
#define INTERNAL_PACKED_DATA(f, c)					\
  PACKED_DATA (*INTERNAL_PACKED_CHAR (f, c))



/* File I/O.  */
static FILE *pk_input_file;      /* The file we're currently reading.  */
static string current_filename;  /* Its pathname.  */

/* Low-level input.  These macros call the corresponding routines in
   kbase, using the static variables for the input file and filename.  */

#define PK_MATCH_BYTE(expected) \
  match_byte (expected, pk_input_file, current_filename)
#define PK_GET_BYTE() get_byte (pk_input_file, current_filename)
#define PK_GET_TWO() get_two (pk_input_file, current_filename)
#define PK_GET_FOUR() get_four (pk_input_file, current_filename)
#define PK_GET_N_BYTES(n) get_n_bytes (n, pk_input_file, current_filename)

static four_bytes pk_get_n_byte_value (unsigned);
static string pk_get_string (unsigned);

static void get_all_pk_chars (internal_font_type *);
static void get_one_pk_char (internal_font_type *, one_byte);

/* Memory ``I/O''.  */
static pk_char_type *unpack (packed_char_type);
static void get_bitmap (pk_char_type *);
static void get_packed_bitmap (pk_char_type *, packed_char_type);
static unsigned get_run_count (one_byte, unsigned *);

static one_byte *pk_input_data = NULL;  /* Where we are reading from.  */
static boolean do_upper_nybble = true;  /* Which nybble we're reading.  */

static one_byte data_get_bit (void);
static one_byte data_get_nybble (void);
static four_bytes data_get_three (void);
static four_bytes data_get_four (void);
static four_bytes data_get_n_byte_value (one_byte);
static signed_4_bytes data_get_signed_n_byte_value (one_byte);


/* Handle multiple fonts.  */
static internal_font_type *find_internal_font (string);
static void save_internal_font (string, FILE *);
static void delete_internal_font (string);



/* Prepare for reading FILENAME.  Return false if it can't be opened. 
   We allow the same file to be opened twice; I'm not sure if that's a
   good idea or not, but it doesn't seem to matter much.  */

boolean
pk_open_input_file (string filename)
{
  FILE *f = fopen (filename, "r");
  
  if (f == NULL) return false;
  
  save_internal_font (filename, f);
  return true;
}


/* Close the file associated with FILENAME, if we have it open.  If it's
   not open, the caller must have made a mistake, so
   `delete_internal_font' gives a fatal error.  */

void
pk_close_input_file (string filename)
{
  delete_internal_font (filename);
}



/* The PK preamble contains the fontwide information we are supposed to
   return.  If FONT_NAME isn't open for reading, we give a fatal error.  */

pk_preamble_type
pk_get_preamble (string filename)
{
  pk_preamble_type p;
  unsigned comment_length;
  internal_font_type *f = find_internal_font (filename);
  
  assert (INTERNAL_FILE (*f) != NULL);
  pk_input_file = INTERNAL_FILE (*f);
  current_filename = filename;

  /* The preamble is at the beginning of the file, so move there.  */
  xfseek (pk_input_file, 0, SEEK_SET, filename);
  
  PK_MATCH_BYTE (PK_PRE);
  PK_MATCH_BYTE (PK_ID);
  comment_length = PK_GET_BYTE ();
  PK_COMMENT (p) = pk_get_string (comment_length);
  PK_DESIGN_SIZE (p) = PK_GET_FOUR ();
  PK_CHECKSUM (p) = PK_GET_FOUR ();
  PK_H_RESOLUTION (p) = PK_GET_FOUR ();
  PK_V_RESOLUTION (p) = PK_GET_FOUR ();

  /* We read (but not unpack) the entire file.  */
  get_all_pk_chars (f);
  
  return p;
}



/* Return the information for the character CODE in the font FILENAME,
   or null if the character doesn't exist in that font.  */

pk_char_type *
pk_get_char (one_byte code, string filename)
{
  internal_font_type *f = find_internal_font (filename);
  
  assert (INTERNAL_FILE (*f) != NULL);
  pk_input_file = INTERNAL_FILE (*f);  
  current_filename = filename;

  if (!INTERNAL_CHARS_READ (*f))
    /* This routine sets the character info in F (among other things).  */
    get_all_pk_chars (f);

  /* The PK_CHAR is null until we unpack the bitmap.  If the character
     is not in the font at all, then the PACKED_CHAR is null.   */
  if (INTERNAL_PK_CHAR (*f, code) != NULL)
    /* We've already unpacked this character.  */
    return INTERNAL_PK_CHAR (*f, code);

  else if (INTERNAL_PACKED_CHAR (*f, code) == NULL)
    /* The character is not in the font.  */
    return NULL;

  else
    { /* We need to unpack the character.  */
      INTERNAL_PK_CHAR (*f, code) = unpack (*INTERNAL_PACKED_CHAR (*f, code));
      PK_CHARCODE (*INTERNAL_PK_CHAR (*f, code)) = code;
      return INTERNAL_PK_CHAR (*f, code);
    }
}


/* This routine reads the contents of the entire `pk_input_file' (which
   should be set before calling it).  The results go into F.  */

static void
get_all_pk_chars (internal_font_type *f)
{
  while (true)  /* The loop exits when we see PK_POST.  */
    {
      one_byte command = PK_GET_BYTE ();
      
      if (command < PK_XXX1)
        get_one_pk_char (f, command);        

      else if (PK_XXX1 <= command && command <= PK_XXX4)
        /* The length of the string is given by the next 1--4 bytes,
           according to the opcode.  We just ignore all specials.  */
        (void) PK_GET_N_BYTES (pk_get_n_byte_value (command - PK_XXX1 + 1));
      
      else if (command == PK_YYY)
        (void) PK_GET_FOUR ();
      
      else if (command == PK_POST)
        break; 

      else if (command == PK_NO_OP)
        /* do nothing */;

      else
        FATAL1 ("Undefined command byte %u in PK file", command);
    }
  
  INTERNAL_CHARS_READ (*f) = true;
}


/* Read a single character, starting at the current position of
   `pk_input_file'.  The results go into F.  */

static void
get_one_pk_char (internal_font_type *f, one_byte flag)
{
  unsigned parameter_length, packet_length;
  int charcode;
  one_byte dyn_f = flag >> 4;

  if (dyn_f == 15)
    FATAL1 ("Unrecognized command %u", flag);

  if (!(flag & 4))
    { /* Short format.  */
      parameter_length = 1;
      packet_length = ((flag & 3) << 8) + PK_GET_BYTE ();
      charcode = PK_GET_BYTE ();
    }
  else if (!(flag & 1))
    { /* Extended short format.  */
      parameter_length = 2;
      packet_length = ((flag & 3) << 16) + PK_GET_TWO ();
      charcode = PK_GET_BYTE ();
    }
  else
    { /* Long format.  */
      parameter_length = 4;
      packet_length = PK_GET_FOUR ();
      charcode = PK_GET_FOUR ();
      if (charcode > MAX_CHARCODE || charcode < 0)
        /* Someone is trying to use a font with character codes that are
           out of our range.  */
        FATAL1 ("Character code %d out of range 0..255 in PK file (sorry)",
                charcode);
    }

  INTERNAL_PACKED_CHAR (*f, charcode) = xmalloc (sizeof (packed_char_type));
  INTERNAL_PACKED_DYN_F (*f, charcode) = dyn_f;
  INTERNAL_PACKED_BLACK_FIRST (*f, charcode) = (flag & 8) != 0;
  INTERNAL_PACKED_PARAM_LENGTH (*f, charcode) = parameter_length;
  INTERNAL_PACKED_DATA_LENGTH (*f, charcode) = packet_length;
  INTERNAL_PACKED_DATA (*f, charcode)
    = (one_byte *) PK_GET_N_BYTES (packet_length);
}


/* This routine unpacks a character C into the raster image.  All the
   fields but one in the `pk_char_type' structure are filled in.  The
   exception is the character code, which our caller does.  */

static pk_char_type *
unpack (packed_char_type c)
{
  dimensions_type d;
  signed_4_bytes h_offset, v_offset;
  pk_char_type *pk_char = xmalloc (sizeof (pk_char_type));

  /* So it doesn't have to be a parameter to all the routines.  */
  pk_input_data = PACKED_DATA (c);
  do_upper_nybble = true; /* The data always starts on a byte boundary.  */

  /* The TFM width is three bytes in the short formats, four in the
     long format.  */
  PK_TFM_WIDTH (*pk_char)
    = LONG_FORMAT (c) ? data_get_four () : data_get_three ();

  /* The horizontal escapement is the next PARAMETER_LENGTH bytes.  In
     addition, if we're using the long format, it is in pixels
     multiplied by 2^16, and we want to unscale it.  */
  PK_H_ESCAPEMENT (*pk_char)
    = data_get_n_byte_value (PACKED_PARAM_LENGTH (c));
  if (LONG_FORMAT (c))
    {
      PK_H_ESCAPEMENT (*pk_char) >>= 16;/* Don't bother to round.  */
      (void) data_get_four ();		/* Ignore the vertical escapement.  */
    }

  /* Next comes the size of the bitmap.  */
  DIMENSIONS_WIDTH (d) = data_get_n_byte_value (PACKED_PARAM_LENGTH (c));
  DIMENSIONS_HEIGHT (d) = data_get_n_byte_value (PACKED_PARAM_LENGTH (c));
  PK_BITMAP (*pk_char) = new_bitmap (d);
  
  /* The last two values in the character preamble are the offsets from
     the upper left pixel to the reference pixel.  */
  h_offset = data_get_signed_n_byte_value (PACKED_PARAM_LENGTH (c));
  v_offset = data_get_signed_n_byte_value (PACKED_PARAM_LENGTH (c));
  MIN_COL (PK_CHAR_BB (*pk_char)) = -h_offset;
  MAX_COL (PK_CHAR_BB (*pk_char)) = -h_offset + DIMENSIONS_WIDTH (d);
  MIN_ROW (PK_CHAR_BB (*pk_char)) = v_offset + 1 - DIMENSIONS_HEIGHT (d);
  MAX_ROW (PK_CHAR_BB (*pk_char)) = v_offset;

  if (PACKED_DYN_F (c) == PK_BITMAP_FLAG)
    get_bitmap (pk_char);
  else
    get_packed_bitmap (pk_char, c);

  return pk_char;
}


/* Turn the packed bitmap in the character C into a real bitmap, in
   PK_CHAR.  The bitmap storage and sizes in PK_CHAR must already be
   set.  The packed data should encode exactly as many bits as are in
   the bitmap.  */

static void
get_packed_bitmap (pk_char_type *pk_char, packed_char_type c)
{
  bitmap_type b = PK_BITMAP (*pk_char);
  unsigned row = 0;
  unsigned col = 0;
  unsigned row_repeat_count = 0;
  one_byte color = PACKED_BLACK_FIRST (c);

  while (row < BITMAP_HEIGHT (b))
    {
      unsigned i;
      unsigned cmd_repeat_count;
      unsigned run_count = get_run_count (PACKED_DYN_F (c), &cmd_repeat_count);

      if (cmd_repeat_count != 0)
        {
          if (row_repeat_count != 0)
            FATAL2 ("Two repeat counts (%u and %u) in PK row",
                   row_repeat_count, cmd_repeat_count);
          row_repeat_count = cmd_repeat_count;
        }

      /* Start painting pixels.  */
      for (i = 0; i < run_count; i++)
        {
          BITMAP_PIXEL (b, row, col) = color;
          col++;
          
          /* It is inefficient to test this here.  Sorry.  */
          if (col == BITMAP_WIDTH (b))
            { /* We've finished a row.  If we saw a repeat count
                 somewhere within it, we should duplicate it.  
                 If the PK file is correct, `row' will not increase past
                 the height of the bitmap from repeat counts, but we may
                 as well be safe.  */
              for (row++;
                   row_repeat_count > 0 && row < BITMAP_HEIGHT (b); 
                   row_repeat_count--, row++)
                for (col = 0; col < BITMAP_WIDTH (b); col++)
                  BITMAP_PIXEL (b, row, col) = BITMAP_PIXEL (b, row - 1, col);

              /* Start painting pixels from the run count at the
                 beginning of the next row.  */
              col = 0;
            }
        }

      color = !color;
    }

    if (col != 0)
      FATAL ("The PK bitmap ended in a strange place");
}


/* This routine is called `pk_packed_num' in the PK documentation, but
   as far as the caller is concerned, it doesn't return a packed number,
   it returns a run count.  Due to the definition of packed data, it is
   impractical to return repeat counts in any other way except together
   with a run count.  */

static unsigned
get_run_count (one_byte dyn_f, unsigned *repeat_count)
{
  unsigned n = data_get_nybble ();
  
  *repeat_count = 0;
  
  if (n == LONG_RUN_COUNT_FLAG)
    { /* Get a three nybble (or more) value, represented as some number
         of zeros, followed by that many hexadecimal values.  The
         largest such number that we'll be asked to find will fit in 
         32 bits.  */
      unsigned zero_count = 0;
      do
        {
          n = data_get_nybble ();
          zero_count++;
        }
      while (n == 0);
      
      for ( ; zero_count > 0; zero_count --)
        n = (n << 4) + data_get_nybble ();
      
      /* The smallest three-nybble value is one more than the largest
         two nybble value.  */
      return n - 15 + ((13 - dyn_f) << 4) + dyn_f;
    }

  else if (n <= dyn_f)
    /* The value we just read is itself the number.  */
    return n;

  else if (n < REPEAT_COUNT_FLAG)
    /* Get a two-nybble value.  */
    return ((n - dyn_f - 1) << 4) + data_get_nybble () + dyn_f + 1;
  
  else if (n == REPEAT_COUNT_FLAG)
    /* If the PK file is valid, this recursion will go only one level
       deep, because a repeat count should never be followed by another
       repeat count.  In other words, the next nybble must be a ``run
       count'' in some form.  (This is why this routine is called
       `pk_packed_num' in other programs.)  */
    *repeat_count = get_run_count (dyn_f, &n);

  else
    *repeat_count = 1;
  
  /* We only get here if we saw a repeat count.  Now we have to get the
     run count which should be next; there won't be any repeat count.  */
  return get_run_count (dyn_f, &n);
}


/* The PK format also allows for a straight bitmap representation in the
   file, one pixel per bit.  Here we put each pixel into a byte.  */

static void
get_bitmap (pk_char_type *pk_char)
{
  unsigned row;
  unsigned col;
  one_byte extra_bits;
  bitmap_type b = PK_BITMAP (*pk_char);
  
  for (row = 0; row < BITMAP_HEIGHT (b); row++)
    for (col = 0; col < BITMAP_WIDTH (b); col++)
      BITMAP_PIXEL (b, row, col) = data_get_bit ();
  
  /* If the bitmap did not completely fill the last byte, we should read
     and ignore the remaining bits.  */
  extra_bits = 8 - (row * col) % 8;
  while (extra_bits-- > 0)
    (void) data_get_bit ();
}



/* Low-level routines that read from the disk file.  */

static string
pk_get_string (unsigned length)
{
  string s;
  
  if (length == 0) return "";
  
  s = PK_GET_N_BYTES (length);
  s = xrealloc (s, length + 1);
  s[length] = 0;
  return s;
}


/* N must be <= 4.  */

static four_bytes
pk_get_n_byte_value (unsigned n)
{
  four_bytes v = 0;
  
  for ( ; n > 0; n--)
    v |= PK_GET_BYTE () << ((n - 1) * 8);

  return v;
}



/* These routines do not read from a disk file.  Instead, they read from
   memory (where we have put each character's definition, directly from
   the file.)  They use the semi-global `pk_input_data'.  */


/* This routine is used when the character is defined as a bitmap,
   instead of run-encoded.  */

static one_byte
data_get_bit ()
{
  static one_byte mask = 0;
  static one_byte n;
  one_byte value;
  
  if (mask == 0)
    {
      n = data_get_nybble ();
      mask = 0x8;
    }
  
  value = (n & mask) != 0;
  mask >>= 1;
  
  return value;
}


/* Since the PK format is based on nybbles, this routine is by far the
   most frequently called.  */

static one_byte
data_get_nybble ()
{
  do_upper_nybble = !do_upper_nybble;

  /* Double inversion gets us back the original value.  After we have
     done the lower nybble, we advance to the next byte.  */
  return !do_upper_nybble ? *pk_input_data >> 4 : *pk_input_data++ & 0xf;
}


/* We assume (correctly) that we are not in the middle of a byte
   when any of the following routines are called.  */

static four_bytes
data_get_three ()
{
  return data_get_n_byte_value (3);
}


static four_bytes
data_get_four ()
{
  return data_get_n_byte_value (4);
}


static four_bytes
data_get_n_byte_value (one_byte n)
{
  four_bytes v = 0;
  
  for ( ; n > 0; n--)
    v |= *pk_input_data++ << ((n - 1) * 8);

  return v;
}


static signed_4_bytes
data_get_signed_n_byte_value (one_byte passed_n)
{
  signed_4_bytes v = 0;
  unsigned n = passed_n;
  
  assert (n <= 4);

  for ( ; n > 0; n--)
    v |= (*pk_input_data++ << ((n - 1) * 8));

  if (passed_n == 1)
    return (signed_byte) (v & 0xff);
  else if (passed_n == 2)
    return (signed_2_bytes) (v & 0xffff);
  else if (passed_n == 3)
    return (signed_4_bytes) (v & 0xffffff);
  else
    return (signed_4_bytes) v;

  return v;
}



/* We assign an index number to NAME, and store both NAME and the
   `internal_font_type' that we construct in parallel lists.  The file F
   goes into the `internal_font_list'.  */

static list_type font_name_list, internal_font_list;

static void
save_internal_font (string name, FILE *f)
{
  unsigned c;
  string *new_name;
  internal_font_type *new_font;
  static boolean first_call = true;
  
  if (first_call)
    { /* We have to construct our lists.  */
      font_name_list = new_list ();
      internal_font_list = new_list ();
      first_call = false;
    }

  new_name = (string *) append_element (&font_name_list, sizeof (string));
  new_font = (internal_font_type *)
             append_element (&internal_font_list, sizeof (internal_font_type));

  /* Save the information.  */
  *new_name = xstrdup (name);
  INTERNAL_FILE (*new_font) = f;
  
  /* We also have to initialize other parts of `new_font'.  */
  for (c = 0; c <= MAX_CHARCODE; c++)
    {
      INTERNAL_PK_CHAR (*new_font, c) = NULL;
      INTERNAL_PACKED_CHAR (*new_font, c) = NULL;
    }
  INTERNAL_CHARS_READ (*new_font) = false;
}


/* Here we return the `internal_font_type' previously stored along with
   NAME.  If NAME hasn't been saved, we give a fatal error.  */

static internal_font_type *
find_internal_font (string name)
{
  unsigned e;

  for (e = 0; e < LIST_SIZE (font_name_list); e++)
    if (STREQ (*(string *) LIST_ELT (font_name_list, e), name))
      return (internal_font_type *) LIST_ELT (internal_font_list, e);

  FATAL1 ("The PK font `%s' hasn't been saved", name);
  return NULL; /* NOTREACHED */
}


/* Free the memory and file pointer used by the font FILENAME.  We don't
   delete the list element for now, since removing elements from the
   middle of an array is painful.  Instead, we simply set the name in
   `font_name_list' to the empty string, so that the element is useless.
   Yes, this is disgusting.  */

static void
delete_internal_font (string filename)
{
  unsigned e;

  /* We can't use `find_internal_font', since we need to know the list
     element index, not just the internal font information.  */ 
  for (e = 0; e < LIST_SIZE (font_name_list); e++)
    if (STREQ (*(string *) LIST_ELT (font_name_list, e), filename))
      {
        unsigned c;
        internal_font_type *f = LIST_ELT (internal_font_list, e);

	assert (INTERNAL_FILE (*f) != NULL);
	xfclose (INTERNAL_FILE (*f), filename);
	INTERNAL_FILE (*f) = NULL;

	if (INTERNAL_CHARS_READ (*f))
	  for (c = 0; c <= MAX_CHARCODE; c++)
	    if (INTERNAL_PK_CHAR (*f, c) != NULL) 
	      free (INTERNAL_PK_CHAR (*f, c));
        
        *(string *) LIST_ELT (font_name_list, e) = "";
        return;
      }
  FATAL1 ("The PK font `%s' hasn't been saved", filename);
}
