/* cmdline.h: macros to help process command-line arguments.

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

#ifndef CMDLINE_H
#define CMDLINE_H

#include "filename.h"
#include "getopt.h"
#include "global.h"


/* Test whether getopt found an option ``A''.
   Assumes the option index is in the variable `option_index', and the
   option table in a variable `long_options'.  */

#define ARGUMENT_IS(a) STREQ (long_options[option_index].name, a)


/* Read the string S as a percentage, i.e., a number between 0 and 100.  */

#define GET_PERCENT(s)							 \
  ({									 \
    unsigned temp = atou (s);						 \
    if (temp > 100)							 \
      FATAL1 ("GET_PERCENT: The argument %u should be at most 100, since \
it's a percentage", temp);						 \
    temp / 100.0;							 \
  })


/* Read the string S as two character codes separated by a hyphen.  Put
   the numeric values of the codes into START and END.  */

#define GET_RANGE(s, start, end)					\
  do									\
    {									\
      string str1 = strtok (s, "-");					\
      if (str1 == NULL)							\
        FATAL1 ("GET_RANGE: No character code in argument `%s'", s);	\
      start = xparse_charcode (str1);					\
      end = xparse_charcode (s + strlen (str1) + 1);			\
    }									\
  while (0)


/* In most programs, we want to deduce the resolution from the filename
   given, if possible.  But in some we don't.  Assumes lots of
   variables.  */

#ifdef CMDLINE_NO_DPI
#define FIND_CMDLINE_DPI() /* as nothing */
#else
#define FIND_CMDLINE_DPI()						\
  if (!explicit_dpi)							\
    {									\
      string test_dpi = find_dpi (argv[optind]);			\
      if (test_dpi != NULL)						\
        dpi = test_dpi;							\
    }
#endif


/* Perform common actions at the end of parsing the arguments.  Assumes
   lots of variables: `printed_version', a boolean for whether the
   version number has been printed, `optind', the current option index,
   `argc', `argv', and `explicit_dpi', for whether the resolution has
   been assigned already.  */

#define FINISH_COMMAND_LINE()						\
  do									\
    {									\
      /* Just wanted to know the version number?  */			\
      if (printed_version && optind == argc) exit (0);			\
                                                                        \
      /* Exactly one (non-empty) argument left?  */			\
      if (optind + 1 == argc && *argv[optind] != 0)			\
        {								\
          FIND_CMDLINE_DPI ();						\
          return remove_suffix (argv[optind]);				\
        }								\
      else								\
        {								\
          fprintf (stderr, "Usage: %s [options] <font_name>.\n", argv[0]);\
          fprintf (stderr, "(%s.)\n", optind == argc ? "Missing <font_name>"\
                                      : "Too many <font_name>s");	\
          fprintf (stderr, "For more information, use ``-help''.\n");	\
          exit (1);							\
        }								\
      return NULL; /* stop warnings */					\
    }									\
  while (0)


#define GETOPT_USAGE \
"  You can use `--' or `-' to start an option.
  You can use any unambiguous abbreviation for an option name.
  You can separate option names and values with `=' or ` '."

/* This reads a list of unsigned integers, separated by commas, and
   returns a vector of the integers, as numbers.  A -1 is appended to
   mark the end of the list, hence the return type.  */
int *scan_unsigned_list (string);

/* If S has the form <name>.<number><stuff>, as in `foo.1200gf', return
   <number>, as a string; otherwise, return NULL.  */
extern string find_dpi (string s);

#endif /* not CMDLINE_H */
