#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "bintobdf.h"
#include "patchlevel.h"

char *program;               /* This program's name.                    */
char *fontname = NULL;       /* BDF font name.                          */
int rev_bytes_out = FALSE;   /* Reverse the character's bytes.          */
int size = 16;               /* Default to 16x16 font.                  */
int bytes_per_char = 0;      /* Bytes needed for each char.             */
int bytes_skipped = 0;       /* Number of initial bytes to be skipped.  */
int encoding_start = 0xa1a1; /* Initial encoding start.                 */
int chars_per_row = 0x5e;    /* Number characters per row in font.      */
int gen_n_chars = 0;         /* Only generate n charaters.              */
int xoff = 0;                /* Character's x offset.                   */
int yoff = -2;               /* Character's y offset.                   */
int default_char = 0xa1a1;   /* Default char of BDF font.               */
int total_chars = 0;         /* Total number of chars in input file.    */

/*
 * Set the default architecture specific bit and byte orderings for bitmaps.
 */
int bitOrder  = DEFAULTBITORDER,
    byteOrder = DEFAULTBYTEORDER,
    scanUnit  = DEFAULTSCANUNIT;

/*
 * Next three functions for adjusting bitmaps for different architectures.
 */
static void
invertbits(buf, count)
unsigned char *buf;
int count;
{
    int i, n, m;
    unsigned char c, oc;

    for ( i = 0; i < count; i++ ) {
        c = buf[i];
        oc = 0;
        for (n=0, m=7; n < 8; n++, m--) {
            oc |= ((c >> n) & 1) << m;
        }
        buf[i] = oc;
    }
}

static void
invert2(buf, count)
unsigned char *buf;
int count;
{
    int i;
    unsigned char c;

    for ( i = 0; i < count; i += 2) {
        c = buf[i];
        buf[i] = buf[i+1];
        buf[i+1] = c;
    }
}

static void
invert4(buf, count)
unsigned char *buf;
int count;
{
    int i;
    unsigned char c;

    for (i = 0; i < count; i += 4) {
        c = buf[i];
        buf[i] = buf[i+3];
        buf[i+3] = c;
        c = buf[i+1];
        buf[i+1] = buf[i+2];
        buf[i+2] = c;
    }
}

#define my_abs(x) (((x) < 0) ? -(x) : (x))

void
genbdf(infile, outfile)
FILE *infile, *outfile;
{
    time_t clock;
    struct stat fsize;

    int i, row, column, row_start, column_start;
    int start, end, inc;
    int swidth, dwidth;
    long int fsz, nfsz;
    int done;
    int bpcl, count, char_count;
    double resval;

    unsigned char *in_buf;

    time(&clock);
    fprintf(outfile, "STARTFONT 2.1\n");
    fprintf(outfile, "COMMENT GENERATED BY \"bin2bdf %d.%d\"\n",
            BINTOBDFVERSION, patchlevel);
    fprintf(outfile, "COMMENT AUTHOR %s\n", AUTHOR_STRING);
    fprintf(outfile, "COMMENT DATE %s", ctime(&clock));
    fprintf(outfile, "COMMENT\n");
    fprintf(outfile, "FONT %s\n", fontname);
    fprintf(outfile, "SIZE %d %d %d\n", size, DEFAULTXRES, DEFAULTYRES);
    fprintf(outfile, "FONTBOUNDINGBOX %d %d %d %d\n", size, size, xoff, yoff);
    fprintf(outfile, "STARTPROPERTIES 4\n");
    fprintf(outfile, "COPYRIGHT \"Public Domain\"\n");
    fprintf(outfile, "DEFAULT_CHAR %d\n", default_char);
    fprintf(outfile, "FONT_ASCENT %d\n", size + yoff);
    fprintf(outfile, "FONT_DESCENT %d\n", my_abs(yoff));
    fprintf(outfile, "ENDPROPERTIES\n");

    if (fstat(fileno(infile), &fsize) < 0) {
        fprintf(stderr, "%s: stat error on input file\n", program);
        fclose(infile);
        exit(-1);
    }

    fsz = nfsz = ((long int)fsize.st_size) - bytes_skipped;

    if (bytes_skipped) {
        if (fseek(infile, bytes_skipped, 0L) < 0) {
            fprintf(stderr, "%s: fseek problem on input file.\n", program);
            fclose(infile);
            if (outfile != stdout)
              fclose(outfile);
        }
    }

    while(fsz % size) fsz--;
    if ((nfsz - fsz) != 0) {
        fprintf(stderr,
                "%s: found odd number of bytes - skipping %d bytes forward.\n",
                program, (nfsz - fsz));
        if (fseek(infile, (nfsz - fsz), 1L) < 0) {
            fprintf(stderr, "%s: fseek problem on input file.\n", program);
            fclose(infile);
            if (outfile != stdout)
              fclose(outfile);
        }
    }

    total_chars = fsz / bytes_per_char;
    total_chars++;

    if ((gen_n_chars == 0) || (gen_n_chars > total_chars))
      gen_n_chars = total_chars;

    resval = (((double)size)/1000.0) * (((double)DEFAULTXRES)/72.27);
    swidth = (int)(((double)DEFAULTXRES)/resval) / 10;
    dwidth = size;

    fprintf(outfile, "CHARS %d\n", gen_n_chars);

    row_start = (encoding_start >> 8) & 0xff;
    column_start = encoding_start & 0xff;

    row = row_start;
    column = column_start;
    done = FALSE;
    bpcl = (size + 7) / 8;
    in_buf = (unsigned char *)malloc(bytes_per_char * sizeof(unsigned char));

    char_count = 0;
    while(char_count < gen_n_chars && !feof(infile)) {
        if ((column - column_start) == chars_per_row) {
            column = column_start;
            row++;
        }

        fprintf(outfile, "STARTCHAR %02x%02x\n", row & 0xff, column & 0xff);
        fprintf(outfile, "ENCODING %d\n", ((row << 8) | column));
        fprintf(outfile, "SWIDTH %d 0\n", swidth);
        fprintf(outfile, "DWIDTH %d 0\n", size);
        fprintf(outfile, "BBX %d %d %d %d\n", size, size, xoff, yoff);
        fprintf(outfile, "BITMAP\n");


        if (fread(in_buf, 1, bytes_per_char, infile) != bytes_per_char) {
            if (!feof(infile)) {
                fprintf(stderr,
                        "%s: missing some bytes on the end of input file\n",
                        program);
                fclose(infile);
                if (outfile != stdout)
                  fclose(outfile);
                exit(-1);
            }
        }

        if (bitOrder == LSBFirst)
          invertbits(in_buf, bytes_per_char);
        if (bitOrder != byteOrder) {
            if (scanUnit == 2)
              invert2(in_buf, bytes_per_char);
            else if (scanUnit == 4)
              invert4(in_buf, bytes_per_char);
        }

        if (rev_bytes_out) {
            start = bytes_per_char - 1;
            end = inc = -1;
        } else {
            start = 0;
            end = bytes_per_char;
            inc = 1;
        }

        count = 0;
        while (start != end) {
            if (count == bpcl) {
                putc('\n', outfile);
                count = 0;
            }
            count++;
            fprintf(outfile, "%02x", in_buf[start]);
            start += inc;
        }
        fprintf(outfile, "\nENDCHAR\n");
        column++;
        char_count++;
    }
    fprintf(outfile, "ENDFONT\n");
}

char *
make_fontname(name)
char *name;
{
    char *tmp_name, *ptr, *ptr1;
    int len;

    len = strlen(name) - 1;
    ptr = ptr1 = name + len;
    for (;*ptr && *ptr != DIR_SEP && ptr != name; ptr--) {}

    if (!*ptr) {
        fprintf(stderr, "%s: unexpected NULL in filename\n", program);
        exit(-1);
    }

    for (;*ptr1 && *ptr1 != '.' && ptr1 != name; ptr1--) {}

    if (ptr == name && ptr1 == name) {
        tmp_name = (char *)malloc((len + 4) * sizeof(char));
        sprintf(tmp_name, "%s%d", name, size);
        return(tmp_name);
    }

    if (*ptr == DIR_SEP)
      ptr++;

    tmp_name = (char *)malloc((ptr1 - ptr + 5) * sizeof(char));
    sprintf(tmp_name, "%.*s%d", (ptr1 - ptr), ptr, size);

    return(tmp_name);
}

void
usage()
{
    fprintf(stderr, "usage:  %s [-size #] [-skip #] [-cdefglmLMoruxy] ",
            program);
    fprintf(stderr, "infile\n");
    exit(-1);
}

int
getint(str)
char *str;
{
    if (!str)
      usage();
    return(atoi(str));
}

main(argc, argv)
int argc;
char **argv;
{
    FILE *in = stdin, *out = stdout;
    char next;

    program = argv[0];
    argc--;
    *argv++;

    if (!argc)
      usage();

    while(argc) {
        if (argv[0][0] == '-') {
            switch(argv[0][1]) {
              case 'c':
              case 'C':
                argc--;
                *argv++;
                chars_per_row = getint(argv[0]);
                break;
              case 'd':
              case 'D':
                argc--;
                *argv++;
                default_char = getint(argv[0]);
                break;
              case 'e':
              case 'E':
                argc--;
                *argv++;
                encoding_start = getint(argv[0]);
                break;
              case 'f':
              case 'F':
                argc--;
                *argv++;
                if (argv[0])
                  fontname = argv[0];
                else
                  usage();
                break;
              case 'g':
              case 'G':
                argc--;
                *argv++;
                gen_n_chars = getint(argv[0]);
                break;
              case 'L':
                byteOrder = LSBFirst;
                break;
              case 'l':
                bitOrder = LSBFirst;
                break;
              case 'M':
                byteOrder = MSBFirst;
                break;
              case 'm':
                bitOrder = MSBFirst;
                break;
              case 'o':
              case 'O':
                argc--;
                *argv++;
                if (argv[0]) {
                    if (!(out = fopen(argv[0], "w+b"))) {
                        fprintf(stderr, "%s: problem with output file %s\n",
                                argv[0]);
                        exit(-1);
                    }
                } else
                  usage();
                break;
              case 'r':
              case 'R':
                rev_bytes_out = TRUE;
                break;
              case 's':
              case 'S':
                next = '\0';
                switch(argv[0][2]) {
                    /* Set char width to size by size bits. */
                  case 'i':
                  case 'I':
                    argc--;
                    *argv++;
                    size = getint(argv[0]);
                    break;
                    /* Skip n bytes at beginning of file. */
                  case 'k':
                  case 'K':
                    argc--;
                    *argv++;
                    bytes_skipped = getint(argv[0]);
                    break;
                  default:
                    usage();
                    break;
                }
                break;
              case 'u':
              case 'U':
                argc--;
                *argv++;
                scanUnit = getint(argv[0]);
                break;
              case 'x':
              case 'X':
                argc--;
                *argv++;
                xoff = getint(argv[0]);
                break;
              case 'y':
              case 'Y':
                argc--;
                *argv++;
                yoff = atoi(argv[0]);
                break;
              default:
                usage();
                break;
            }
        } else {
            if (fontname == NULL)
              fontname = make_fontname(argv[0]);
            
            if (!(in = fopen(argv[0], "r+b"))) {
                fprintf(stderr, "%s: problem with input file %s\n",
                        program, argv[0]);
                exit(-1);
            }
            bytes_per_char = ((size + 7) / 8) * size;
            genbdf(in, out);
            if (in != stdin)
              fclose(in);
            if (out != stdout)
              fclose(out);
        }
        if (argc) {
            argc--;
            *argv++;
        }
    }
}