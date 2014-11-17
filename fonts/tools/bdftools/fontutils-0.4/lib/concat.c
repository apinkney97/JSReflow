/* concat.c: dynamic string concatenation.

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


/* Return the concatenation of S1 and S2.  This would be a prime place
   to use varargs.  */

string
concat P2C(string, s1, string, s2)
{
  string answer = xmalloc (strlen (s1) + strlen (s2) + 1);
  strcpy (answer, s1);
  strcat (answer, s2);

  return answer;
}


string
concat3 P3C(string, s1, string, s2, string, s3)
{
  string answer = xmalloc (strlen (s1) + strlen (s2) + strlen (s3) + 1);
  strcpy (answer, s1);
  strcat (answer, s2);
  strcat (answer, s3);

  return answer;
}


string
concat4 P4C(string, s1, string, s2, string, s3, string, s4)
{
  string answer = xmalloc (strlen (s1) + strlen (s2) + strlen (s3)
                            + strlen (s4) + 1);
  strcpy (answer, s1);
  strcat (answer, s2);
  strcat (answer, s3);
  strcat (answer, s4);

  return answer;
}


string
concat5 P5C(string, s1, string, s2, string, s3, string, s4, string, s5)
{
  string answer = xmalloc (strlen (s1) + strlen (s2) + strlen (s3)
                            + strlen (s4) + strlen (s5) + 1);
  strcpy (answer, s1);
  strcat (answer, s2);
  strcat (answer, s3);
  strcat (answer, s4);
  strcat (answer, s5);

  return answer;
}
