#include <stdio.h>

char *program, *fontname;
char buff[128];
int fontno = 1;

int w, h, psize, resX, resY;
double unit_width, unit_height;
double width_scale;
double bbx_llx, bbx_lly, bbx_urx, bbx_ury;

char *
get_fontname(str)
char *str;
{
    int len;
    char *ptr, *ptr1, *tmp_name;

    if (str == NULL) {
        tmp_name = (char *)malloc(20 * sizeof(char));
        sprintf(tmp_name, "BDF FONT %d", fontno++);
        return(tmp_name);
    }
    len = strlen(str);
    ptr = str + (len - 1);
    while(ptr > str && *ptr != '.')
      ptr--;
    if (ptr == str)
      return(str);
    ptr1 = ptr;
    while(ptr1 > str && *ptr1 != '/')
      ptr1--;
    if (*ptr1 == '/')
      ptr1++;

    tmp_name = (char *)malloc(((ptr - ptr1) + 1) * sizeof(char));
    strncpy(tmp_name, ptr1, (ptr - ptr1));
    tmp_name[(ptr - ptr1)] = '\0';
    return(tmp_name);
}

void
header(in)
FILE *in;
{
    int dx, dy;
    int i, count, ndx, ndy;

    printf("%%!PS-Adobe-2.0\n");
    printf("10 dict dup begin\n");
    printf("/FontType 3 def\n");
    printf("/FontMatrix [1 0 0 1 0 0] def\n");

    fscanf(in, "%[^\n]\n", buff);
    while(!feof(in) && strncmp(buff, "SIZE", 4))
      fscanf(in, "%[^\n]\n", buff);
    if (strncmp(buff, "SIZE", 4)) {
        fprintf(stderr, "%s: missing SIZE\n");
        exit(1);
    }
    sscanf(buff, "SIZE %d %d %d", &psize, &resX, &resY);

    width_scale = (((double)psize)/1000.0) * (((double)resX)/72.27);

    while(!feof(in) && strncmp(buff, "FONTBOUNDINGBOX", 15))
      fscanf(in, "%[^\n]\n", buff);
    if (strncmp(buff, "FONTBOUNDINGBOX", 15)) {
        fprintf(stderr, "%s: missing FONTBOUNDINGBOX\n");
        exit(1);
    }
    sscanf(buff, "FONTBOUNDINGBOX %d %d %d %d", &w, &h, &dx, &dy);

    ndx = (dx < 0) ? -(dx) : dx;
    ndy = (dy < 0) ? -(dy) : dy;

    unit_width = (double)(w + dx);
    unit_height = (double)(h + dy);

    bbx_llx = ((double)(w + ndx)) / unit_width;
    bbx_urx = ((double) dx) / unit_width;

    bbx_lly = ((double)(h + ndy)) / unit_height;
    bbx_ury = ((double) dy) / unit_height;

    printf("/FontBBox [%g %g %g %g] def\n", bbx_llx, bbx_lly, bbx_urx, bbx_ury);
    printf("/Encoding 256 array def\n");
    printf("0 1 255 {Encoding exch /.notdef put} for\n");
    printf("Encoding\n");
    count = 0;
    for (i = 0; i < 255; i++) {
        printf("dup %d /char%d put\t", i, i);
        count++;
        if (count % 3 == 0) {
            count = 0;
            printf("\n");
        }
    }
    printf("255 /char255 put\n");
    printf("/BuildChar\n{0 begin\n/char exch def\n/fontdict exch def\n");
    printf("/charname fontdict /Encoding get char get def\n");
    printf("/charinfo fontdict /CharData get charname get def\n");
    printf("/wx charinfo 0 get def\n");
    printf("/charbbox charinfo 1 4 getinterval def\n");
    printf("wx 0 charbbox aload pop setcachedevice\n");
    printf("charinfo 5 get charinfo 6 get true\n");
    printf("fontdict /imagemaskmatrix get\n");
    printf("dup 4 charinfo 7 get put\n");
    printf("dup 5 charinfo 8 get put\n");
    printf("charinfo 9 1 getinterval cvx imagemask\n");
    printf("end } def\n");
    printf("/BuildChar load 0 6 dict put\n");
    printf("/imagemaskmatrix [%d 0 0 -%d 0 0] def\n", w, h);
    printf("/CharData 256 dict def\nCharData begin\n");
}

void
gen_chars(in)
FILE *in;
{
    int cw, ch, dx, dy, udx, udy, encoding, i;
    int sw, dummy, count = 0;
    double wx, llx, lly, urx, ury, ndx, ndy, tdx;

    fscanf(in, "%[^\n]\n", buff);
    while(!feof(in)) {
        while(!feof(in) && strncmp(buff, "ENCODING", 8))
          fscanf(in, "%[^\n]\n", buff);
        if (feof(in))
          break;
        sscanf(buff, "ENCODING %d", &encoding);

        if (encoding < 0 || encoding > 255) {
            fscanf(in, "%[^\n]\n", buff);
            continue;
        }

        while(!feof(in) && strncmp(buff, "SWIDTH", 6))
          fscanf(in, "%[^\n]\n", buff);
        if (feof(in))
          break;
        sscanf(buff, "SWIDTH %d %d", &sw, &dummy);

        sw = (int)(((double)sw) * width_scale);
        sw++;

        if (!feof(in)) {
            while(!feof(in) && strncmp(buff, "BBX", 3))
              fscanf(in, "%[^\n]\n", buff);
            if (feof(in))
              break;
            sscanf(buff, "BBX %d %d %d %d", &cw, &ch, &dx, &dy);

            udx = (dx < 0) ? -(dx) : dx;
            udy = (dy < 0) ? -(dy) : dy;

            wx = ((double)sw) / unit_width;
            if (wx == 0)
              wx = ((double)w) * 0.01;

            llx = ((double) dx) / unit_width;
            urx = ((double)(sw + dx)) / unit_width;
            llx = ((wx - (llx+urx)) < 0.0) ? -(llx) : llx;

            lly = ((double) dy) / unit_height;
            ury = ((double)(ch + udy)) / unit_height;

            ury += bbx_lly - ury;

            ndx = ((double)dx) + 0.5;
            ndy = ((double)(ch + dy)) + 0.5;
            printf("/char%d [%g %g %g %g %g %d %d %g %g\n<", encoding, wx, llx,
                   lly, urx, ury, cw, ch, ndx, ndy);

            fscanf(in, "%[^\n]\n", buff);
            while(!feof(in) && strncmp(buff, "BITMAP", 6))
              fscanf(in, "%[^\n]\n", buff);
            if (feof(in))
              break;
            for (i = 0; i < ch; i++) {
                fscanf(in, "%[^\n]", buff);
                printf("%s", buff);
                getc(in);  /* kill the EOL */
            }
            printf(">] def\n");
        } else
          fscanf(in, "%[^\n]\n", buff);
        count++;
    }
    if (count < 256)
      printf("/.notdef [%g 0 0 0 0 1 0 0 <>] def\n", ((double) w) * .01);
    printf("end\nend\n/%s exch definefont pop\n", fontname);
}


main(argc, argv)
int argc;
char **argv;
{
    FILE *in = stdin;

    program = argv[0];

    argc--;
    *argv++;
    while(argc) {
        if (!(in = fopen(argv[0], "r")))
          fprintf(stderr, "%s: problem with %s\n", program, argv[0]);
        else {
            fontname = get_fontname(argv[0]);
            header(in);
            gen_chars(in);
            fclose(in);
            in = NULL;
        }
        argc--;
        *argv++;
    }
    if (in != NULL) {
        header(in);
        gen_chars(in);
    }
}
