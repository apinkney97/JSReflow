herps:		herps.o
		cc -o herps herps.o
herps.o:	prolog.h pagestart.h pagefinish.h trailer.h

.SUFFIXES:	.ps .h
.ps.h:
		sed -e 's/.*/"&",/' $*.ps >$*.h
