/* input-pbm.c: read PBM files.

Copyright (C) 1990, 1991, 19 19ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "config.h"

#include "file-input.h"

#include "main.h"
#include "image-header.h"
#include "input-pbm.h"

#include "pbmplus.h"
#include "pbm.h"


/* Where the input comes from.  */
static FILE *pbm_input_file;
static string pbm_input_filename;



/* Only one file can be open at a time.  We do no path searching.  If
   FILENAME can't be opened, we quit.  */

void
pbm_open_input_file (string filename)
{
  assert (pbm_input_file == NULL);
  
  pbm_input_file = xfopen (filename, "r");
  pbm_input_filename = filename;
}


/* Close the input file.  If it hasn't been opened, we quit.  */

void
pbm_close_input_file ()
{
  assert (pbm_input_file != NULL);
  
  xfclose (pbm_input_file, pbm_input_filename);
  pbm_input_file = NULL;
}



/* Read the header information.
   Modifies the global image_header in main.c.  */

void
pbm_get_header ()
{
  int width, height, format;

  pbm_readpbminit (pbm_input_file, &width, &height, &format);
  image_header.width = (two_bytes) width;
  image_header.height = (two_bytes) height;
  image_header.depth = 0;
  image_header.format = (two_bytes) format;
}



/* Read one scanline of the image.  */

boolean
pbm_get_scanline (one_byte *line_in_bits)
{
  static unsigned scanline_count = 0;
  int c = getc (pbm_input_file);
  
  if (c == EOF)
    return false;
    
  ungetc (c, pbm_input_file);
  pbm_readpbmrow (pbm_input_file, line_in_bits, image_header.width, 
		  image_header.format);
  if (trace_scanlines)
    {
      printf ("%4d:", ++scanline_count);
      for (c = 0; c < image_header.width; c++)
        putchar (line_in_bits[c] ? '*' : ' ');
      putchar ('\n');
    }
  return true;
}
