CC = gcc
CFLAGS = -ggdb -lm
OBJECTS = main.o Formatter.o parseAFM.o
OUTNAME = LineBreak

$(OUTNAME) : $(OBJECTS)
	$(CC) $(OBJECTS) -o $(OUTNAME) $(CFLAGS)

main.o : main.c parseAFM.h Formatter.h
	$(CC) $(CFLAGS) -c main.c

Formatter.o : Formatter.c Formatter.h
	$(CC) $(CFLAGS) -c Formatter.c

parseAFM.o : parseAFM.c parseAFM.h
	$(CC) $(CFLAGS) -c parseAFM.c


.PHONY: clean

clean:
	rm -f $(OBJECTS) $(OUTNAME)
