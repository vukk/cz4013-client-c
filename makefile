IDIR=include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj
LDIR=lib
SRCDIR=src

LIBS=-Wall -W -pedantic -std=c11

_DEPS = current_utc_time.h marshall.h message.h command.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = current_utc_time.o marshall.o message.o command.o client.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

client: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
