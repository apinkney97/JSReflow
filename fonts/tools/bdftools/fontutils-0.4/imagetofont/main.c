/* imagetofont -- convert a scanned image to a GF font.

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
#define CMDLINE_NO_DPI /* It's not in the input filename.  */
#include "cmdline.h"
#include "filename.h"
#include "getopt.h"
#include "gf.h"
#include "libfile.h"
#include "report.h"

#include "bb-outline.h"
#include "bitmap2.h"
#include "extract.h"
#include "image-char.h"
#include "image-header.h"
#include "input-img.h"
#include "input-pbm.h"
#include "main.h"
#include "strips.h"


/* A list of the image rows on which the baselines occur.  The end of
   the list is marked with an element -1.  (-baselines)  */
static int *baseline_list = NULL;

/* If a bounding box encloses more black (as a percentage) than this, it
   is cleared. (-clean-threshold)  */
real clean_threshold = .6;

/* If set, prints diagnostics about which boxes are and aren't cleaned. 
   (-print-clean-info)  */ 
boolean print_clean_info = false;

/* The design size of the font we're creating.  (-design-size)  */
real design_size = 10.0;

/* The name of the IFI file.  (-info-file)  */
string info_filename = NULL;

/* Pointers to functions based on input format.  (-input-format)  */
void (*image_open_input_file)(string filename) = NULL;
void (*image_close_input_file)(void) = NULL;
void (*image_get_header)(void) = NULL;
boolean (*image_get_scanline)(one_byte *p) = NULL;

/* Find about this many characters in the image, then stop.  This is
   useful only for testing, because converting the entire image takes a long
   time.  So we might in truth output a few more characters than
   specified here.  (-nchars)  */
unsigned nchars_wanted = MAX_CHARCODE + 1;

/* Says whether to print the row numbers of each character as we go, 
   so that the user can decide how to adjust jumping baselines. 
   (-print-guidelines)  */
boolean print_guidelines = false;

/* Says which characters we should process.  This is independent of the
   ordering in the font file.  (-range) */
int starting_char = 0;
int ending_char = MAX_CHARCODE;

/* Says whether to analyze the image for the actual characters, or just
   divide it up into strips.  (-strips) */
boolean do_strips = false;

/* Show every scanline on the terminal as we read it?  (-trace-scanlines) */
boolean trace_scanlines = false;

/* Says whether to output progress reports.  (-verbose) */
boolean verbose = false;

/* The image header.  */
image_header_type image_header = { 0, 0, 0, 0, 0, 0 };




/* Private variables.  */

/* The non-extension part of the filename for the output font.  */
static string font_basename;

/* The name of the file we're going to write.  (-output-file) */
static string output_name;

static string font_name_extension = ".pbm";

/* How many character reports fit on a line.  */
static unsigned nchars_per_line;

/* Where the baseline of the current row is in the image.  The first row
   is #1, and so on.  We start over at row 1 at each image row.*/
static unsigned row_baseline;

/* The mapping of which bounding boxes go with which characters.  */
static unsigned total_boxes_expected;


static gf_char_type bitmap_to_gf_char
  (bitmap_type, real, bounding_box_type, image_char_type);
static void clean_bitmap (bitmap_type, int, int);
static boolean do_image_line
  (bitmap_type, unsigned *, unsigned *, real, image_char_list_type);
static image_char_list_type get_image_info (unsigned *);
static unsigned output_chars
  (bounding_box_list_type, bitmap_type, real, image_char_list_type, unsigned);
static string read_command_line (int, string[]);



void
main (int argc, string argv[])
{
  string gf_name;
  bitmap_type *image_bitmap;
  string font_name = read_command_line (argc, argv);
  unsigned nchars_done = 0;
  string dpi = xmalloc (sizeof (unsigned) + 1);
  string design_size_str = xmalloc (sizeof (unsigned) + 1);
  string output_name_suffix = "";
  
  
  dpi[0] = design_size_str[0] = 0;

  /* Open the main input file.  */
  (*image_open_input_file) (concat (font_name, font_name_extension));
  
  /* We need the horizontal resolution before we can make the GF name,
     so for at least IMG input, have to read the header.  */
  (*image_get_header) ();

  font_basename = basename (font_name);

  if (output_name == NULL)
    {
      output_name = font_basename;

      if (!do_strips)
	 sprintf (design_size_str, "%u", (unsigned) design_size);
    }
   

  /* We have two completely different strategies for processing the image: 
     one to just take a constant number of scanlines as each character,
     the other to analyze the image and write out the true characters.  */


  /* The GF name has the form:
         <output_name><output_name_suffix><design_size>.<dpi>gf,
     where either one or the other of <output_name_suffix> and
     <design_size> is empty.  */

  sprintf (dpi, "%u", image_header.hres);

  if (do_strips)
    {
      output_name_suffix = "sp";

      /* The design size is irrelevant when we're creating strips,
         but it should be pretty large, lest the relative dimensions in
         the TFM file get too large.  */

      design_size = 100.0;
    }



  /* Only one of output_name_suffix and design_size should be nonempty.  */

  if (find_suffix (output_name) == NULL)
    output_name = make_stem_suffix (output_name,
				concat (output_name_suffix, design_size_str));

  gf_name = make_output_filename (output_name, concat (dpi, "gf"));

  if (!gf_open_output_file (gf_name))
    FATAL_PERROR (gf_name);

  /* Identify ourselves in the GF comment.  */
  gf_put_preamble (concat ("imagetofont output ", now () + 4));

  if (do_strips)
    write_chars_as_strips (image_header);
  else
    {
      /* Read the image information.  This tells us to which character
         each bounding box belongs, among other things.  */
      image_char_list_type image_char_list
        = get_image_info (&total_boxes_expected);

      /* Since we report more information when `print_guidelines' is true,
         we can fit fewer characters per line.  */
      nchars_per_line = print_guidelines ? 4 : 11;

      /* The main loop also (and more commonly) exits when we've read the
         entire image.  */ 
      while (nchars_done < nchars_wanted)
        {
          unsigned *transitions;

          /* Read one line of characters in the image.  After this,
             `image_bitmap' is, for example, `a...z', with blank columns at
             the left and right.  Whatever is in the original image.  */
          image_bitmap
            = some_black_to_all_white_row (image_header.width, &transitions);
          if (image_bitmap == NULL)
            break; /* We've read the whole image file.  */

          if (baseline_list == NULL || *baseline_list == -1)
            {
             if (baseline_list != NULL)
                WARNING ("imagetofont: Not enough baselines specified");
              row_baseline = BITMAP_HEIGHT (*image_bitmap);
            }
          else
            row_baseline = *baseline_list++;

          /* If `do_image_line' fails, we're supposed to read the next
             row in the image, and put it below the current line.  For
             example, if a line has only an `!' on it, we will only get
             the stem on the first call to `some_black_to_all_white_row'.  
             We want to get the dot in there, too.  */
          while (!do_image_line (*image_bitmap, transitions, &nchars_done,
                                 (real) image_header.hres, image_char_list))
            {
              bitmap_type *revised
                = append_next_image_row (*image_bitmap, image_header.width,
                                         &transitions);
              if (revised == NULL)
                FATAL ("imagetofont: Image ended in the midst of a character");
              
              free_bitmap (image_bitmap);
              image_bitmap = revised;
              if (baseline_list == NULL)
                row_baseline = BITMAP_HEIGHT (*image_bitmap);
            }

          free_bitmap (image_bitmap);
        }
    }

  if (verbose || print_guidelines || print_clean_info)
    REPORT ("\n");

  /* We've read all the characters we're supposed to (or else the whole
     image).  Finish up the font.  */
  gf_put_postamble (real_to_fix (design_size),
                    (real) image_header.hres, (real) image_header.vres);

  (*image_close_input_file) ();
  gf_close_output_file ();
  
  exit (0);
}


/* Analyze and output all of the bitmap IMAGE, which is one line of type
   in the original image.  The resolution of the image is H_RESOLUTION,
   and we've read NCHARS characters so far.  We use IMAGE_CHAR_LIST and
   the transition vector TRANSITIONS, which has in it how IMAGE breaks
   into characters.
   
   We return false if we need to be given another image row.  */

#define NEXT_TRANSITION()  ({						\
  if (*transitions == BITMAP_WIDTH (image) + 1)				\
    {									\
      WARNING ("do_image_line: Expected more transitions");		\
      break;								\
    }									\
  *transitions++;							\
})

static boolean
do_image_line (bitmap_type image, unsigned *transitions, unsigned *nchars, 
               real h_resolution, image_char_list_type image_char_list)
{
  static unsigned box_count = 0;
  bounding_box_type bb;
  MIN_ROW (bb) = 0;
  MAX_ROW (bb) = BITMAP_HEIGHT (image) - 1;
  
  /* `nchars_wanted' is an option to the program, defined at the top.  */
  while (*nchars < nchars_wanted && *transitions != BITMAP_WIDTH (image) + 1)
    {
      int bb_offset = 0;
      unsigned original_box_count = box_count;
      bounding_box_list_type boxes = init_bounding_box_list ();
      bitmap_type *char_bitmap = xmalloc (sizeof (bitmap_type));
      bitmap_type *temp_bitmap = xmalloc (sizeof (bitmap_type));

      /* The first element of TRANSITIONS is white-to-black.
         Thereafter, they alternate.  */
      MIN_COL (bb) = NEXT_TRANSITION ();
      MAX_COL (bb) = 1 + NEXT_TRANSITION ();
      
      /* After this, `char_bitmap' might be just an `a' (with blank
         rows above and below, because `a' has neither a descender nor
         an ascender), then `b', and so on.  In many specimens, some
         characters overlap; for example, we might get `ij' as one
         bitmap, instead of `i' and then `j'.  */
      *char_bitmap = extract_subbitmap (image, bb);
      *temp_bitmap = copy_bitmap (*char_bitmap);

      while (true)
        {
          /* Find bounding boxes around all the shapes in `char_bitmap'.
             Continuing the `ij' example, this would result in four
             bounding boxes (one for each dot, one for the dotless `i',
             and one for the dotless `j').  */
          bounding_box_list_type temp_boxes
            = find_outline_bbs (*temp_bitmap, false, 0, 0);

          /* The subimages we've created all start at column zero.  But
             we don't want to just overlay the images; we put them side
             by side.  So we change the numbers in the bounding box.  */
          offset_bounding_box_list (&temp_boxes, bb_offset);

          box_count += BB_LIST_LENGTH (temp_boxes);
          append_bounding_box_list (&boxes, temp_boxes);

          /* If we've read all the bounding boxes we expected to, we're
             done.  */
          if (box_count == total_boxes_expected)
            break;

          /* If we've read more than we expected to, we're in
             trouble.  */
          if (box_count > total_boxes_expected)
            {
              WARNING2 ("do_image_line: Read box #%u but expected only %u",
                        box_count, total_boxes_expected);
              /* No point in giving this message more than once.  */
              total_boxes_expected = INT_MAX;
            }

          /* See if the white column was in the middle of a character.  */
          if (box_at_char_boundary_p (image_char_list, *nchars,
                                      box_count - original_box_count))
            break;

          /* It was.  We have to read more (horizontally) of the image,
             if we have it.  If we don't, return false, so our caller
             can read more (vertically). */
          free_bitmap (temp_bitmap);
          if (*transitions == BITMAP_WIDTH (image) + 1)
            {
              /* Forget that we've seen this before.  */
              box_count = original_box_count;
              return false;
            }
          MIN_COL (bb) = NEXT_TRANSITION ();
          MAX_COL (bb) = 1 + NEXT_TRANSITION ();
          *temp_bitmap = extract_subbitmap (image, bb);
          REPORT ("+");
          if (temp_bitmap == NULL)
            {
              WARNING1 ("do_image_line: Expected more bounding boxes for `%c'",
                        IMAGE_CHARCODE (IMAGE_CHAR (image_char_list, *nchars)));
              break;
            }

          /* The boxes `temp_bitmap' should be just to the right of the
             bitmap we've accumulated in `char_bitmap'.  */
          bb_offset = BITMAP_WIDTH (*char_bitmap);
          
          /* When this happens, it usually means that the IFI file
             didn't specify enough bounding boxes for some character,
             and so things are out of sync.  */
          if (BITMAP_HEIGHT (*char_bitmap) != BITMAP_HEIGHT (*temp_bitmap))
            {
              WARNING ("do_image_line: Line ended inside a character");
              break;
            }
          bitmap_concat (char_bitmap, *temp_bitmap);
        }
      free_bitmap (temp_bitmap);

      /* Convert the bits inside those bounding boxes into
         characters in the GF font.  */
      *nchars += output_chars (boxes, *char_bitmap, h_resolution,
                               image_char_list, *nchars);

      free_bitmap (char_bitmap);
      free_bounding_box_list (&boxes);
    }
  
  return true;
}



/* This macro figures out how the previous and next bounding boxes
   overlap the current bounding box, which we assume is `BB_LIST_ELT
   (boxes, this_box)'.  The variables we assign to, `left_edge' and
   `right_edge', must be already declared.  We have to put the numbers
   in the coordinate system of the bitmap of the single character we're
   working on, instead of the coordinate system the bounding boxes
   numbers are in, which is the group of characters we were sent.  */

#define SET_EDGES(bb)							\
  left_edge								\
    = this_box == 0							\
      ? 0								\
      : MAX_COL (BB_LIST_ELT (boxes, this_box - 1)) - MIN_COL (bb);	\
  right_edge								\
    = this_box == BB_LIST_LENGTH (boxes) - 1				\
      ? BB_WIDTH (bb)							\
      : MIN_COL (BB_LIST_ELT (boxes, this_box + 1)) - MIN_COL (bb);


/* For each bounding box in the list BOXES, extract from IMAGE_BITMAP
   and turn the resulting bitmap into a single character in the font. 
   The ENCODING string maps each bounding box to a character code;
   consecutive bounding boxes may belong to the same character.  For
   example, `i' will appear twice, once for the dot and once for the
   stem.  We assume that all the bounding boxes for a given character
   will appear in IMAGE_BITMAP.
   
   We return the number of characters (not bounding boxes) found,
   including characters that were omitted.  */

static unsigned
output_chars (bounding_box_list_type boxes, bitmap_type image_bitmap, 
	      real h_resolution, image_char_list_type image_char_list,
              unsigned current_char)
{  
  static unsigned char_count = 0;
  int left_edge, right_edge;
  unsigned this_box;
  boolean done[BB_LIST_LENGTH (boxes)];
  unsigned number_written = 0;
  
  for (this_box = 0; this_box < BB_LIST_LENGTH (boxes); this_box++)
    done[this_box] = false;

  for (this_box = 0; this_box < BB_LIST_LENGTH (boxes); this_box++)
    {
      bounding_box_type bb;
      bitmap_type bitmap;
      image_char_type c;
      one_byte charcode;
      gf_char_type gf_char;
      
      /* `done[this_box]' will be set if we get to a bounding box that
         has already been combined with a previous one, because of
         an alternating combination.  Since we never go backwards, we
         don't bother to set `done' for every box we look at.  */
      if (done[this_box])
        continue;
        
      bb = BB_LIST_ELT (boxes, this_box);
      c = IMAGE_CHAR (image_char_list, current_char++);
      charcode = IMAGE_CHARCODE (c);
      
      /* We must read through the whole list of bounding boxes, since we
         don't know which characters we're going to ignore.  */
      if (charcode >= starting_char && charcode <= ending_char)
        {
          bitmap = extract_subbitmap (image_bitmap, bb);
          /* Remove extraneous blobs and such.  We have to do this when
             we first have the bitmap, before anything has been
             combined.  Otherwise, the dot on the `i', for example,
             would get cleaned away.  */
          SET_EDGES (bb);
          clean_bitmap (bitmap, left_edge, right_edge);
        }

      while (IMAGE_CHAR_BB_COUNT (c)-- > 1)
        {
          unsigned combine_box;
          
          if (IMAGE_CHAR_BB_ALTERNATING (c))
            {
              /* Don't increment `this_box', since it is incremented at
                 the end of the loop, and the next box is part of
                 another character.  */
              combine_box = this_box + 2;
              /* Don't look at the second box again in the outer loop:  */
              done[combine_box] = true;
            }
          else
            combine_box = ++this_box;

          if (combine_box >= BB_LIST_LENGTH (boxes))
            {
              WARNING1 ("output_chars: Not enough outlines for char %u",
                        (unsigned) charcode);
              break;
            }

          if (charcode >= starting_char && charcode <= ending_char)
            {
              /* Get the shape to combine with `bitmap'.  */
              bounding_box_type next_bb = BB_LIST_ELT (boxes, combine_box);
              bitmap_type next_bitmap
                = extract_subbitmap (image_bitmap, next_bb);

              SET_EDGES (next_bb);
              clean_bitmap (next_bitmap, left_edge, right_edge);
              combine_images (&bitmap, next_bitmap, &bb, next_bb);
              free_bitmap (&next_bitmap);
            }
        }
      
      if (charcode >= starting_char && charcode <= ending_char
          && !IMAGE_CHAR_OMIT (c))
        {
          REPORT ("[");
          gf_char = bitmap_to_gf_char (bitmap, h_resolution, bb, c);
          gf_put_char (gf_char);
          REPORT ("]");
          /* This and the GF character's bitmap are the same, so we only
             need to free one of the two.  */
          free_bitmap (&bitmap);
        }
      else
        REPORT ("x"); /* We're ignoring this character.  */

      REPORT1 ("%c", ++char_count % nchars_per_line ? ' ' : '\n');
      number_written++;
    }
  
  return number_written;
}              


/* Derive the information necessary to output the font character from
   the bitmap B, and return it.  The resolution of the bitmap is given in
   pixels per inch as H_RESOLUTION.  The bounding box BB encloses the
   character in the image coordinates.  We use BB, plus the global
   `baseline', to determine the positioning of the GF character.  */

static gf_char_type
bitmap_to_gf_char (bitmap_type b, real h_resolution,
                   bounding_box_type bb, image_char_type image_char)
{
  gf_char_type gf_char;
  real width_in_points;
  charcode_type charcode = IMAGE_CHARCODE (image_char);
  /* Have to subtract one from `row_baseline' because it is a one-origin
     variable; we need it zero-origin.  */
  int char_baseline
    = row_baseline - 1 + IMAGE_CHAR_BASELINE_ADJUST (image_char);
  
  if (verbose || print_guidelines)
    fprintf (stderr, "%u", charcode);
  
  GF_CHARCODE (gf_char) = charcode;
  GF_BITMAP (gf_char) = b;
  GF_CHAR_MIN_COL (gf_char) = IMAGE_CHAR_LSB (image_char);
  GF_CHAR_MAX_COL (gf_char) = GF_CHAR_MIN_COL (gf_char) + BITMAP_WIDTH (b);
  GF_CHAR_MIN_ROW (gf_char) = char_baseline - MAX_ROW (bb);
  GF_CHAR_MAX_ROW (gf_char) = char_baseline - MIN_ROW (bb);
  GF_H_ESCAPEMENT (gf_char) = (GF_CHAR_MAX_COL (gf_char)
                               + IMAGE_CHAR_RSB (image_char));
  width_in_points = GF_H_ESCAPEMENT (gf_char) * POINTS_PER_INCH / h_resolution;
  GF_TFM_WIDTH (gf_char) = real_to_fix (width_in_points / design_size);

  if (print_guidelines)
    {
      if (isprint (charcode))
        fprintf (stderr, " (%c)", charcode);
      fprintf (stderr, " %d/%d", 
 	       GF_CHAR_MIN_ROW (gf_char), GF_CHAR_MAX_ROW (gf_char));
    }


  return gf_char;
}


/* Find the bounding boxes of all outlines inside B.  For each of them
   that are sufficiently black, white them out.  In the type specimens,
   many characters overlap their neighbors by just a few pixels.  When
   we extract the pixels inside the bounding box of the `d', say, a few
   pixels of the `e' may creep in on the right.  We want to get rid of
   those.  Other junk might show up due to dark spots on the page, and
   we want to get rid of those, too.  However, when we find all the
   bounding boxes, we also find the ones enclosing the closed
   counterforms; we wouldn't want to white those out, since the curves
   would be lost!  Hence the test for ``sufficiently black''.  */

static void
clean_bitmap (bitmap_type b, int left, int right)
{
  unsigned this_box;
  bounding_box_list_type boxes = find_outline_bbs (b, true, left, right);
  
  if (print_clean_info)
    fprintf (stderr, "\nCleaning %ux%u:", BITMAP_WIDTH (b), BITMAP_HEIGHT (b));
    
  for (this_box = 0; this_box < BB_LIST_LENGTH (boxes); this_box++)
    {
      bounding_box_type bb = BB_LIST_ELT (boxes, this_box);
      
      /* If the bounding box encloses the entire character, we don't
          have to bother checking how black it is.  Furthermore, the
          spurious blobs we want to remove are always on an edge of the
          whole character.  */
      if (!(BB_WIDTH (bb) == BITMAP_WIDTH (b)
            && BB_HEIGHT (bb) == BITMAP_HEIGHT (b))
          && (MIN_COL (bb) == 0 || MAX_COL (bb) == BITMAP_WIDTH (b)
              || MAX_ROW (bb) == BITMAP_HEIGHT (b) - 1))
        {
          real average_gray;
          unsigned this_row, this_col;
          unsigned black_count = 0;
          unsigned total_pixels = BB_WIDTH (bb) * BB_HEIGHT (bb);
          
          for (this_row = MIN_ROW (bb); this_row <= MAX_ROW (bb); this_row++)
            for (this_col = MIN_COL (bb); this_col < MAX_COL (bb); this_col++)
              {
                if (BITMAP_PIXEL (b, this_row, this_col) == BLACK)
                  black_count++;
              }
          average_gray = (real) black_count / total_pixels;
          if (average_gray >= clean_threshold)
            {
              if (print_clean_info)
                fprintf (stderr, " clearing %.2f", average_gray);
              for (this_row = MIN_ROW (bb); this_row <= MAX_ROW (bb);
                   this_row++)
                for (this_col = MIN_COL (bb); this_col < MAX_COL (bb);
                     this_col++)
                  BITMAP_PIXEL (b, this_row, this_col) = WHITE;
            }
          else
            if (print_clean_info)
              fprintf (stderr, " omitting %.2f", average_gray);
        }
    }
  
  if (print_clean_info)
    fprintf (stderr, ". ");
}



/* Read an IFI file, which gives us some information about the
   image we are going to read.  If `info_filename' is set (which the
   user can do), use that for the name; otherwise, construct it from the
   font name.
   
   Each (non-blank, non-comment) line in this file represents a
   character in the image, and has one to five entries, separated by
   spaces and/or tabs.  Comments start with `%' and continue to the end
   of the line.
   
   The first entry represents the character code.  It is either 1) a
   single character, like `a' or `0'; 2) a number in decimal, such as
   `163' (you cannot give the character codes 0--9 in decimal, since
   those numerals will be taken as the character `0', i.e., ASCII
   character code 48); 3) a number in octal, such as `0243'; 4) a number
   in hex, such as `B9'; or 5) `-1'.  This last means the ``character''
   should not be output in the font.  This is useful when the image has
   blobs on it---we have to be told to not output them somehow.
   
   The second entry, if it exists, represents an adjustment to the
   baseline calculation.  It is a signed decimal number, such as
   `-3', and we move the baseline down (for negative numbers), or up
   (for positive) by this amount.  If it isn't given, we take it to be
   zero, naturally.
   
   The third entry, if it exists, is the number of bounding boxes that
   comprise this character.  For example, an `i' (well, in most
   typefaces, anyway) would have a `2' here, since the dot and the stem
   are not connected.  If this number is negative, the bounding boxes
   are not consecutive in the image; instead, they alternate with
   bounding boxes for some other character.  For example, an italic `j'
   might protrude to the left of the dot of the `i'.
   
   The fourth and fifth entries, if they exist, are the left and right
   side bearings, respectively.  */

static image_char_list_type
get_image_info (unsigned *total_count)
{
  string line;
  int bb_count;
  image_char_list_type image_char_list = new_image_char_list ();
  
  if (info_filename == NULL)
    libfile_start (font_basename, "ifi");
  else
    libfile_start (info_filename, "ifi");

  *total_count = 0;

  while ((line = libfile_line ()) != NULL)
    {
      image_char_type c;
      string baseline_adjust_str, bb_count_str, charcode_str;
      string lsb_str, rsb_str;
      string save_line = line; /* So we can free it.  */
      
      /* Remove the comment, if any.  */
      string comment_loc = strrchr (line, '%');
      if (comment_loc != NULL)
        *comment_loc = 0;
      
      /* The character code.  */
      charcode_str = strtok (line, " \t");
      if (charcode_str == NULL)
        continue;  /* The line was just a comment.  */
      
      if (STREQ (charcode_str, "-1"))
        IMAGE_CHAR_OMIT (c) = true;
      else
        {
          IMAGE_CHARCODE (c) = xparse_charcode (charcode_str);
          IMAGE_CHAR_OMIT (c) = false;
        }

      /* The baseline adjustment.  */
      baseline_adjust_str = strtok (NULL, " \t");
      IMAGE_CHAR_BASELINE_ADJUST (c)
        = baseline_adjust_str == NULL ? 0 : atoi (baseline_adjust_str);
      
      /* The bounding box count.  */
      bb_count_str = strtok (NULL, " \t");
      bb_count = bb_count_str == NULL ? 1 : atoi (bb_count_str);
      
      if (bb_count < 0)
        {
          IMAGE_CHAR_BB_COUNT (c) = -bb_count;
          IMAGE_CHAR_BB_ALTERNATING (c) = true;
        }
      else
        {
          IMAGE_CHAR_BB_COUNT (c) = bb_count;
          IMAGE_CHAR_BB_ALTERNATING (c) = false;
        }

      /* The left side bearing.  */
      lsb_str = strtok (NULL, " \t");
      IMAGE_CHAR_LSB (c) = lsb_str == NULL ? 0 : atoi (lsb_str);
      
      /* The right side bearing.  */
      rsb_str = strtok (NULL, " \t");
      IMAGE_CHAR_RSB (c) = rsb_str == NULL ? 0 : atoi (rsb_str);
      
      *total_count += IMAGE_CHAR_BB_COUNT (c);
      append_image_char (&image_char_list, c);
      free (save_line);
    }
  
  libfile_close ();
  return image_char_list;
}



/* Reading the options.  */

/* This is defined in version.c.  */
extern string version_string;

#define USAGE "Options:
<font_name> should be a base filename, e.g., `cmr10'." 			\
  GETOPT_USAGE								\
"baselines <row1>,<row2>,...: define the baselines for the lines of
  characters in the image.  The baseline of the first line is taken to
  be scanline <row1>, etc.
clean-threshold <percent>: if a bounding box encloses more black than
  this, it is cleared; default is 50.
design-size <real>: define the point size of the font; default is 10.0.
dpi <unsigned>: resolution (required for pbm input).
help: print this message.
info-file <filename>: use <filename>.ifi (if <filename doesn't have a
  suffix; otherwise use <filename>) from which to read the character codes;
  default is `<font_name>.ifi'.
input-format <string>: (required) format (img, pbm) of input file.
nchars <unsigned>: only read the first <unsigned> or so characters from
  the image; default is infinity.
output-file <filename>: write to <filename> if <filename> has a suffix.
  If <filename> doesn't have a suffix, then if writing strips, write to
  <filename>sp.<dpi>gf and to <filename>.<dpi>gf if not.  By default,
  write to <font_name>.<dpi>gf if writing strips and to
  <font_name><design_size>.<dpi>gf if not.
print-clean-info: print gray values for the bounding boxes that are and
  aren't cleaned.
print-guidelines: print the numbers of the top and bottom rows of each
  character.
range <char1>-<char2>: only process characters between <char1> and
  <char2>, inclusive. 
strips: take a constant number of scanlines as each ``character'',
  instead of analyzing the image.
trace-scanlines: show every scanline as we read it.
verbose: output progress reports to stderr.
version: print the version number of this program.
"

static string
read_command_line (int argc, string argv[])
{
  int g;  /* `getopt' return code.  */
  int option_index;
  boolean printed_version = false;
  struct option long_options[]
    = { { "baselines",		1, 0, 0 },
        { "clean-threshold",	1, 0, 0 },
        { "design-size",	1, 0, 0 },
        { "help",               0, 0, 0 },
	{ "dpi",		1, 0, 0 },
        { "info-file",		1, 0, 0 },
        { "nchars",		1, 0, 0 },
        { "input-format",	1, 0, 0 },
        { "output-file",	1, 0, 0 },
        { "print-clean-info",	0, (int *) &print_clean_info, 1 },
        { "print-guidelines",	0, (int *) &print_guidelines, 1 },
        { "range",              1, 0, 0 },
        { "strips",		0, (int *) &do_strips, 1 },
        { "trace-scanlines",	0, (int *) &trace_scanlines, 1 },
        { "verbose",		0, (int *) &verbose, 1 },
        { "version",            0, (int *) &printed_version, 1 },
        { 0, 0, 0, 0 } };

  while (true)
    {
      g = getopt_long_only (argc, argv, "", long_options, &option_index);
      
      if (g == EOF)
        break;

      if (g == '?')
        exit (1);  /* Unknown option.  */
  
      assert (g == 0); /* We have no short option names.  */
      
      if (ARGUMENT_IS ("baselines"))
        baseline_list = scan_unsigned_list (optarg);
        
      else if (ARGUMENT_IS ("clean-threshold"))
        clean_threshold = GET_PERCENT (optarg);
      
      else if (ARGUMENT_IS ("design-size"))
        design_size = atof (optarg);
      
      else if (ARGUMENT_IS ("dpi"))
	{
          image_header.hres = (two_bytes) atou (optarg);
          image_header.vres = image_header.hres;
        }

      else if (ARGUMENT_IS ("help"))
        {
          fprintf (stderr, "Usage: %s [options] <font_name>.\n", argv[0]);
          fprintf (stderr, USAGE);
          exit (0);
        }

      else if (ARGUMENT_IS ("info-file"))
        info_filename = optarg;

      else if (ARGUMENT_IS ("input-format"))
	{
	  if (STREQ ("pbm", optarg))
            {
              image_open_input_file = pbm_open_input_file;
              image_close_input_file = pbm_close_input_file;
              image_get_header = pbm_get_header;
              image_get_scanline = pbm_get_scanline;
              font_name_extension = ".pbm";
            }
	  else if (STREQ ("img", optarg))
            {
              image_open_input_file = img_open_input_file;
              image_close_input_file = img_close_input_file;
              image_get_header = img_get_header;
              image_get_scanline = img_get_scanline;
              font_name_extension = ".img";
            }
        }

      else if (ARGUMENT_IS ("nchars"))
        nchars_wanted = atou (optarg);

      else if (ARGUMENT_IS ("output-file"))
        output_name = optarg;

      else if (ARGUMENT_IS ("range"))
        GET_RANGE (optarg, starting_char, ending_char);
      
      else if (ARGUMENT_IS ("version"))
        printf ("%s.\n", version_string);

      else
        ; /* It was a flag; getopt has already done the assignment.  */
    }
  
  if (image_open_input_file == NULL)
    {
      fprintf (stderr, "You must supply an input format.\n");
      fprintf (stderr, "For more information, use ``-help''.\n");
      exit (1);
    }

  /* If the input format is PBM, then the dpi must be set.  */
  if (image_open_input_file == pbm_open_input_file 
      && image_header.hres == 0)
    {
      fprintf (stderr, "If you input a PBM format, you must supply a dpi.\n");
      fprintf (stderr, "For more information, use ``-help''.\n");
      exit (1);
    }

  FINISH_COMMAND_LINE ();
}
