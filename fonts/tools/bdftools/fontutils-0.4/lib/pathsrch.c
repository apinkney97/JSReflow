/* pathsearch.c: look for files based on paths, i.e., colon-separated
   lists of directories.  
   
   Perhaps we should allow % specifiers in the paths for the resolution, etc.

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
#include "c-pathch.h"
#include "c-namemx.h"
#include "c-pathmx.h"
#include "paths.h"

#include <ctype.h>
#if !defined (DOS) && !defined (VMS) && !defined (VMCMS)
#include <pwd.h>
#endif
#include "dirio.h"
#include "pathsrch.h"

/* Access bit to test whether the filename F is readable.  */
#ifndef R_OK
#define R_OK 4
#endif


static boolean absolute_p P1H(string filename);
static void add_directory P3H(string **, unsigned *, string);
static void expand_subdir P3H(string **, unsigned *, string);
static void parse_envpath P4H(string **, unsigned *, string, string);
static string readable P1H(string);
static string truncate_pathname P1H(string);

/* If FILENAME is absolute or explicitly relative (i.e., starts with
   `/', `./', or `../'), or if DIR_LIST is null, we return whether
   FILENAME is readable as-is.  Otherwise, we test if FILENAME is in any of
   the directories listed in DIR_LIST.  (The last entry of DIR_LIST must
   be null.)  We return the complete path if found, NULL else.
   
   In the interests of doing minimal work here, we assume that each
   element of DIR_LIST already ends with a `/'.
   
   DIR_LIST is most conveniently made by calling `initialize_path_list'.
   This is a separate routine because we allow recursive searching, and
   it may take some time to discover the list of directories.  
   We do not want to incur that overhead every time we want to look for
   a file.
   
   (Actually, `/' is not hardwired into this routine; we use PATH_SEP,
   defined above.)  */

string
find_path_filename P2C(string, filename,  string *, dir_list)
{
  string found_name = NULL;
  
  /* Do this before testing for absolute-ness, as a leading ~ will be an
     absolute pathname.  */
  filename = expand_tilde (filename);
  
  /* If FILENAME is absolute or explicitly relative, or if DIR_LIST is
     null, only check if FILENAME is readable.  */
  if (absolute_p (filename) || dir_list == NULL)
    found_name = readable (filename);
  else
    { /* Test if FILENAME is in any of the directories in DIR_LIST.  */
      string save_filename = filename;
      
      while (*dir_list != NULL)
        {
          filename = concat (*dir_list, save_filename);
          found_name = readable (filename);
          if (found_name == NULL)
            {
              free (filename);
              dir_list++;
            }
          else
            {
              if (found_name != filename)
                free (filename);
              break;
            }
        }
    }
  
  return found_name;
}


/* If NAME is readable, return it.  If the error is ENAMETOOLONG,
   truncate any too-long path components and return the result (unless
   there were no too-long components, i.e., a overall too-long name
   caused the error, in which case return NULL).  On any other error,
   return NULL.
   
   POSIX invented this brain-damage of not necessarily truncating
   pathname components; the system's behavior is defined by the value of
   the symbol _POSIX_NO_TRUNC, but you can't change it dynamically!  */

static string
readable (name)
    string name;
{
  string ret;
  
  if (access (name, R_OK) == 0 && !dir_p (name))
    ret = name;
  else if (errno == ENAMETOOLONG)
    { 
      ret = truncate_pathname (name);

      /* Perhaps some other error will occur with the truncated name, so
         let's call access again.  */
      if (!(access (ret, R_OK) == 0 && !dir_p (ret)))
        { /* Failed.  */
          free (ret);
          ret = NULL;
        }
    }
  else
    ret = NULL; /* Some other error.  */
  
  return ret;
}



/* Truncate any too-long path components in NAME, returning the result.  */

static string
truncate_pathname (name)
    string name;
{
  unsigned c_len = 0;        /* Length of current component.  */
  unsigned ret_len = 0; /* Length of constructed result.  */
  string ret = xmalloc (PATH_MAX + 1);

  for (; *name; name++)
    {
      if (*name == PATH_SEP)
        { /* At a directory delimiter, reset component length.  */
          c_len = 0;
        }
      else if (c_len > NAME_MAX)
        { /* If past the max for a component, ignore this character.  */
          continue;
        }

      /* If we've already copied the max, give up.  */
      if (ret_len == PATH_MAX)
        {
          free (ret);
          return NULL;
        }

      /* Copy this character.  */
      ret[ret_len++] = *name;
      c_len++;
    }
  ret[ret_len] = 0;

  return ret;
}


/* Return true if FILENAME is absolute or explicitly relative, else
   false.  */

static boolean
absolute_p P1C(string, filename)
{
  boolean absolute = *filename == PATH_SEP
#ifdef DOS
                      || isalpha (*filename) && filename[1] == ':'
#endif
		      ;
  boolean explicit_relative
    = ((*filename == '.' && filename[1] == PATH_SEP)
       || (filename[1] == '.' && filename[2] == PATH_SEP));

  return absolute || explicit_relative;
}

/* Return a NULL-terminated array of directory names, each name ending
   with PATH_SEP, created by parsing the PATH_DELIMITER-separated list
   in the value of the environment variable ENV_NAME, or DEFAULT_PATH if
   the envvar is not set.
   
   If CWD_FIRST_P is set, make the first element in the returned list
   `./'.
   
   A leading or trailing colon in the value of ENV_NAME is replaced by
   DEFAULT_PATH.
   
   Any element of the path that ends with double PATH_SEP characters
   (e.g., `foo//') is replaced by all its subdirectories.  */

string *
initialize_path_list P3C(string, env_name, string, default_path,
                         boolean, cwd_first_p)
{
  unsigned dir_count = 0; /* Total number of directories returned.  */
  string *dir_list = NULL;
  
  if (cwd_first_p)
    add_directory (&dir_list, &dir_count, ".");
    
  /* This adds the list of all normal directories.  */
  parse_envpath (&dir_list, &dir_count, env_name, default_path);
  
  /* Add the terminating null entry to `dir_list'.  */
  dir_count++;
  XRETALLOC (dir_list, dir_count, string);
  dir_list[dir_count - 1] = NULL;
  
  return dir_list;
}

/* Subroutines for `initialize_path_list'.  */

/* Parse the value of the environment variable ENV_NAME and the default
   value DEFAULT_PATH as described at `initialize_path_list'.  Update
   the number of directories found in *DIR_COUNT_PTR, and the array of
   directory names, with `/' terminating each, in *DIR_LIST_PTR.
   
   If ENV_NAME is null, only parse DEFAULT_PATH.  If both are null, do
   nothing.  */

static void
parse_envpath (dir_list_ptr, dir_count_ptr, env_name, default_path)
    string **dir_list_ptr;
    unsigned *dir_count_ptr;
    string env_name;
    string default_path;
{
  string dir;
  string env_value = env_name ? getenv (env_name) : NULL;
  string path = expand_default (env_value, default_path);

  if (path == NULL)
    return;

  /* Be sure `path' is in writable memory.  */
  if (path == env_value || path == default_path)
    path = xstrdup (path);

  /* If `path' is nonempty... */
  if (path != NULL && *path != 0)
    { /* Find each element in the path in turn.  */
      for (dir = strtok (path, PATH_DELIMITER_STRING); dir != NULL;
           dir = strtok (NULL, PATH_DELIMITER_STRING))
        {
          int len = strlen (dir);
          
          /* If `dir' is the empty string, ignore it.  */
          if (len == 0)
            continue;
          
          /* If `dir' ends in double slashes, do subdirectories (and
             remove the second slash, so the pathnames we put in the list
             don't look like foo//bar).  */
          if (len > 2 && dir[len - 1] == PATH_SEP && dir[len - 2] == PATH_SEP)
            {
              dir[len - 1] = 0;
              expand_subdir (dir_list_ptr, dir_count_ptr, dir);
            }
          else
            add_directory (dir_list_ptr, dir_count_ptr, dir);
        }
    }
}


/* If DIR is not the pathname of a directory, do nothing.  Otherwise,
   Add a newly-allocated copy of DIR to the end of the array pointed to
   by DIR_LIST_PTR. Increment DIR_COUNT_PTR.  Append a `/' to DIR if
   necessary.  */

static void
add_directory (dir_list_ptr, dir_count_ptr, dir)
    string **dir_list_ptr;
    unsigned *dir_count_ptr;
    string dir;
{
  /* If the path starts with ~ or ~user, expand it.  */
  dir = expand_tilde (dir);

  /* If DIR is not a directory, do nothing.  */
  if (!dir_p (dir))
    return;

  /* If `dir' does not end with a `/', add it.  We can't just
     write it in place, since that would overwrite the null that
     strtok may have put in.  So we always make a copy of DIR.  */
  if (dir[strlen (dir) - 1] != PATH_SEP)
    dir = concat (dir, PATH_SEP_STRING);
  else
    dir = xstrdup (dir);

  /* Add `dir' to the list of the directories.  */
  (*dir_count_ptr)++;
  XRETALLOC (*dir_list_ptr, *dir_count_ptr, string);
  (*dir_list_ptr)[*dir_count_ptr - 1] = dir;
}


/* If DIRNAME is a leaf directory, add it to DIR_LIST.  Otherwise, look for
   subdirectories, recursively.  We assume DIRNAME is non-null and
   non-empty.  */

static void
expand_subdir (dir_list_ptr, dir_count_ptr, dirname)
    string **dir_list_ptr;
    unsigned *dir_count_ptr;
    string dirname;
{
  DIR *dir;
  struct dirent *e;
  
  /* Add the directory (if it is a directory).  */
  add_directory (dir_list_ptr, dir_count_ptr, dirname);

  /* If this is a leaf directory, don't look at its contents.  */
  if (leaf_dir_p (dirname))
    return;

  /* Otherwise, we are going to have look at the contents.  */
  dir = opendir (dirname);
  if (dir == NULL)
    return;
  
  while ((e = readdir (dir)) != NULL)
    {
      if (!(e->d_name[0] == '.'
            && (e->d_name[1] == 0
                || (e->d_name[1] == '.' && e->d_name[2] == 0))))
        { /* If it's not . or .., check.  If it's not a directory,
             we will catch that when we add it to the list on the
             recursive call.  */
          string potential
            = concat3 (dirname, dirname[strlen (dirname) - 1] != PATH_SEP
                                ? PATH_SEP_STRING : "",
                       e->d_name);
          expand_subdir (dir_list_ptr, dir_count_ptr, potential);
          free (potential);
        }
    }
}

/* These routines, while not strictly needed to be exported, are
   plausibly useful to be called by outsiders.  */

/* Replace a leading or trailing `:' in ENV_PATH with DEFAULT_PATH.  If
   neither is present, return ENV_PATH if that is non-null, else
   DEFAULT_PATH.  */

string 
expand_default (env_path, default_path)
    string env_path;
   string default_path;
{
  string expansion;
  
  if (env_path == NULL)
    expansion = default_path;
  else if (*env_path == PATH_DELIMITER)
    expansion = concat (default_path, env_path);
  else if (env_path[strlen (env_path) - 1] == PATH_DELIMITER)
    expansion = concat (env_path, default_path);
  else
    expansion = env_path;
  
  return expansion;
}


/* Expand a leading ~ or ~user, Unix-style, unless we are some weirdo
   operating system.  */

string
expand_tilde P1C(string, name)
{
#if defined (DOS) || defined (VMS) || defined (VMCMS)
  return name;
#else
  string expansion;
  string home;
  
  /* If no leading tilde, do nothing.  */
  if (*name != '~')
    expansion = name;
  
  /* If `~' or `~/', use $HOME if it exists, or `.' if it doesn't.  */
  else if (name[1] == PATH_SEP || name[1] == 0)
    {
      home = getenv ("HOME");
      if (home == NULL)
        home = ".";
        
      expansion
        = name[1] == 0 ? home : concat3 (home, PATH_SEP_STRING, name + 2);
    }
  
  /* If `~user' or `~user/', look up user in the passwd database.  */
  else
    {
      struct passwd *p;
      string user;
      unsigned c = 2;
      while (name[c] != PATH_SEP && name[c] != 0)
        c++;
      
      user = xmalloc (c);
      strncpy (user, name + 1, c - 1);
      user[c - 1] = 0;
      
      p = getpwnam (user);
      /* If no such user, just use `.'.  */
      home = p == NULL ? "." : p->pw_dir;
      
      expansion = name[c] == 0 ? home : concat (home, name + c);
    }
  
  return expansion;
#endif /* not (DOS or VMS or VM/CMS) */
}
