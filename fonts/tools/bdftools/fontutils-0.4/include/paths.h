/* Generated by make from paths.h.in (Tue Mar 31 10:05:38 BST 1992).  */
/* Paths.  */

/* If the environment variable `FONTUTIL_LIB' isn't set, use this
   path instead to search for auxiliary files.  */ 
#ifndef DEFAULT_LIB_PATH
#define DEFAULT_LIB_PATH "/ep/src/fonts/tools/bdftools/fontutils-0.4/data:/usr/local/lib/fontutil"
#endif

/* The meanings of these paths are described in `filename.h'.  They are
   exactly the same as those in the TeX distribution.  */

/* The directories listed in these paths are searched for the various
   font files.  The current directory is always searched first.  */
#ifndef DEFAULT_TFM_PATH
#define DEFAULT_TFM_PATH "/usr/local/lib/tex/fonts//"
#endif

#ifndef DEFAULT_PK_PATH
#define DEFAULT_PK_PATH "/usr/local/lib/tex/fonts//"
#endif

#ifndef DEFAULT_GF_PATH
#define DEFAULT_GF_PATH "/usr/local/lib/tex/fonts//"
#endif
