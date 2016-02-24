IDIR =.
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=.
LDIR =../lib

LIBS=-lm

_DEPS = terred.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = terred.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

terred: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 


