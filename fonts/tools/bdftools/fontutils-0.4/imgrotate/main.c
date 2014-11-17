/* imgrotate -- rotate an image.

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

#include "cmdline.h"
#include "filename.h"
#include "getopt.h"

#include "img-input.h"
#include "img-output.h"

/* Says whether to rotate by 90 degrees clockwise.  (-rotate-clockwise)  */
boolean do_clockwise_rotation = false;

/* Says whether to flip the image end for end.  (-flip)  */
boolean do_head_over_heels = false;

/* The name of the output file.  (-output-file)  */
string output_name = NULL;

/* Says whether to output progress reports.  (-verbose)  */
boolean verbose = false;


static void flip (img_header_type, one_byte *);
static string read_command_line (int, string[]);
static one_byte reverse_bits (one_byte b);
static void rotate_clockwise (img_header_type, one_byte *);



void
main (int argc, string argv[])
{
  one_byte *image;
  img_header_type img_header_in;
  string input_name;
  string image_name = read_command_line (argc, argv);
    
  input_name = concat (image_name, ".img");
  if (!open_img_input_file (input_name))
    FATAL_PERROR (input_name);
  
  if (output_name == NULL)
    output_name = basename (input_name);
  else
    output_name = make_output_filename (output_name, "img");

  /* If the input file is coming from another directory, we don't have
     to name the output file something different.  */

  if (STREQ (output_name, input_name))
    output_name = make_prefix ("n", output_name);

  if (!open_img_output_file (output_name))
    FATAL_PERROR (output_name);
  
  /* Read the image header.  */
  img_header_in = get_img_header ();
  
  /* Read the entire image into memory.  */
  image = read_data ();
  
  if (do_clockwise_rotation)
    rotate_clockwise (img_header_in, image);

  else if (do_head_over_heels)
    flip (img_header_in, image);

  close_img_input_file ();
  close_img_output_file ();
  
  exit (0);
}



/* Flip the image end for end and left for right.  For example,
  *  *
  *  *
  *  *
   ***
     *
     *
     *   (that's an `h') turns upright.  */

static void
flip (img_header_type header, one_byte *image)
{
  int this_col, this_row;
  unsigned width_in_bits, width_in_bytes, width_in_shorts;
  unsigned count = 0;
  unsigned padding = 16 - header.width % 16;

  put_img_header (header);
  
  /* We have to get a byte at a time.  */
  width_in_bits = header.width;
  width_in_shorts = width_in_bits / 16 + (width_in_bits % 16 != 0);
  width_in_bytes = width_in_shorts * 2;
  
  for (this_row = header.height - 1; this_row >= 0; this_row--)
    {
      one_byte *row = image + this_row * width_in_bytes;

      /* Because we are reversing left and right, we must make sure that
         the padding bits don't get put into the image.  The scanner
         software pads bytes that are partially data with 1's (i.e.,
         white), so the only case we have to worry about is when the
         padding is more than one byte -- the byte that is entirely
         padding is output as zero.  */
      one_byte first_byte = (padding >= 8) ? 0xff : row[width_in_bytes - 1];
      
      img_put_byte (reverse_bits (first_byte));
      
      for (this_col = width_in_bytes - 2; this_col >= 0; this_col--)
        {
          one_byte byte = row[this_col];
          img_put_byte (reverse_bits (byte));
        }
      
      if (verbose)
        {
          putc ('.', stderr);
          if ((header.height - this_row) % 8 == 0)
            fprintf (stderr, "%2.0f%c",
                     100.0 * (header.height - this_row) / header.height,
                     (++count % 4 == 0) ? '\n' : ' ');
          fflush (stderr);
        }
    }
  
  if (verbose)
    putc ('\n', stderr);
}


/* Reverse the bits in the byte B; e.g., 0x88 becomes 0x11.  */

static one_byte
reverse_bits (one_byte b)
{
  one_byte mask;
  unsigned this_bit;
  one_byte answer = 0;
  
  if (b != 0 && b != 0xff)
    for (this_bit = 0, mask = 0x80; this_bit < 8; this_bit++, mask >>= 1)
      answer |= ((b & mask) != 0) << this_bit;
  else
    answer = b;
  
  return answer;
}



/* Rotate IMAGE by 90 degrees clockwise.  This turns, for example,
      ******
   ***  *
  *     *
   ***  *
      ******   upright.  */


static void
rotate_clockwise (img_header_type header_in, one_byte *image)
{
  one_byte byte;
  one_byte mask;
  unsigned this_col, this_row, this_bit;
  unsigned width_in_bits, width_in_bytes, width_in_shorts;

  /* Figure out how many bits of padding we are going to need.  */
  unsigned padding = 16 - header_in.height % 16;
  unsigned total_height = header_in.height + padding;
  one_byte *column = xmalloc (total_height);

  img_header_type header_out = header_in;
  
  /* Exchange the width and height.  */
  unsigned temp = header_out.width;
  header_out.width = header_out.height;
  header_out.height = temp;
  
  /* Put in the white pixels (which are 1 bits).  */
  for (this_row = 0; this_row < padding; this_row++)
    column[header_in.height + this_row] = 0xff;

  /* We have to get a byte at a time.  */
  width_in_bits = header_in.width;
  width_in_shorts = width_in_bits / 16 + (width_in_bits % 16 != 0);
  width_in_bytes = width_in_shorts * 2;
  
  /* That means that we are going to output the padding columns at the
     right of the original image as the last rows.  So we have to change
     the header to conform.  */
  header_out.height = width_in_bytes * 8;
  put_img_header (header_out);

  for (this_col = 0; this_col < width_in_bytes; this_col++)
    {
      for (this_row = 0; this_row < header_in.height; this_row++)
        /* The image starts down at the bottom, so adjust.  */
        column[header_in.height - 1 - this_row]
          = image[this_row * width_in_bytes + this_col];

      /* We can now output eight scanlines, one for each bit in the
         column array.  */
      for (byte = 0, mask = 0x80, this_bit = 0;
           this_bit < 8;
           mask >>= 1, this_bit++)
        {
          /* For each bit, go down the column, constructing the
             scanline.  */
          for (this_row = 0; this_row < total_height; this_row++)
            {
              one_byte new_bit
                = ((column[this_row] & mask) != 0) << (7 - this_row % 8);
              byte |= new_bit;
              if ((this_row + 1) % 8 == 0)
                {
                  img_put_byte (byte);
                  byte = 0;
                }
            }
        }
      
      if (verbose)
        {
          putc ('.', stderr);

          if ((this_col + 1) % 8 == 0)
            fprintf (stderr, "%2.0f%c", 100.0 * this_col / width_in_bytes,
                     (this_col + 1) % 56 == 0 ? '\n' : ' ');
          fflush (stderr);
        }
    }

  if (verbose)
    putc ('\n', stderr);
}



/* Reading the options.  */

/* This is defined in version.c.  */
extern string version_string;

#define USAGE "Options:
  <image_name> should be a base filename, e.g., `gmr'.  It is extended
    with `.img'."							\
  GETOPT_USAGE								\
"flip: rotate the image 180 degrees.
help: print this message.
output-file <filename>: write the output to <filename> if <filname> has
   a suffix, otherwise write to <filename>.img; the default is the input
   filename, if the input file directory is different from the execution
   directory, or the input filename prepended with `n', if the
   directories are the same.
rotate-clockwise: rotate the image 90 degrees clockwise.
verbose: output progress reports to stdout.
version: print the version number of this program.
"

static string
read_command_line (int argc, string argv[])
{
  int g;   /* `getopt' return code.  */
  int option_index;
  boolean printed_version = false;
  struct option long_options[]
    = { { "flip",		0, (int *) &do_head_over_heels, 1 },
        { "help",		0, 0, 0 },
        { "output-file",	1, 0, 0 },
        { "rotate-clockwise",	0, (int *) &do_clockwise_rotation, 1 },
        { "verbose",		0, (int *) &verbose, 1 },
        { "version",		0, (int *) &printed_version, 1 },
        { 0, 0, 0, 0 } };

  while (true)
    {
      g = getopt_long_only (argc, argv, "", long_options, &option_index);
      
      if (g == EOF)
        break;

      if (g == '?')
        exit (1);  /* Unknown option.  */
  
      assert (g == 0); /* We have no short option names.  */
      
      if (ARGUMENT_IS ("help"))
        {
          fprintf (stderr, "Usage: %s [options] <image_name>.\n", argv[0]);
          fprintf (stderr, USAGE);
          exit (0);
        }

      else if (ARGUMENT_IS ("output-file"))
        output_name = optarg;

      else if (ARGUMENT_IS ("version"))
        printf ("%s.\n", version_string);

      else
        ; /* It was a flag; getopt has already done the assignment.  */
    }
  
  /* Just wanted to know the version number?  */
  if (printed_version && optind == argc) exit (0);
  
  /* Check that they asked to do exactly one thing.  */
  if (!(do_head_over_heels || do_clockwise_rotation))
    {
      fprintf (stderr, "No operation specified.\n");
      fprintf (stderr, "For more information, use ``-help''.\n");
      exit (1);
    }
  else if (do_head_over_heels && do_clockwise_rotation)
    {
      fprintf (stderr, "Too many operations specified (sorry).\n");
      fprintf (stderr, "For more information, use ``-help''.\n");
      exit (1);
    }

  /* Exactly one (non-empty) argument left?  */
  if (optind + 1 == argc && *argv[optind] == 0)
    {
      fprintf (stderr, "Usage: %s [options] <image_name>.\n", argv[0]);
      fprintf (stderr, "(%s.)\n", optind == argc
                        ? "Missing <image_name>" : "Too many <image_name>s");
      fprintf (stderr, "For more information, use ``-help''.\n");
      exit (1);
    }

  return argv[optind];
}
