.PHONEY: all clean install uninstall

PROG=parrot
CC?=gcc
CFLAGS?=-O2 -std=c11 -Wall -DGIT_DESC=\"$(shell git describe --tags --always --dirty)\"
PREFIX?=/usr/local
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man/man1/
SRC=parrot.c

all: $(PROG)

$(PROG): $(SRC)
	$(CC) $(SRC) -o $(PROG) $(CFLAGS)

clean:
	rm -f $(PROG)

install:
	mkdir -p $(MANDIR)
	cp parrot.1 $(MANDIR)
	mkdir -p $(BINDIR)
	cp $(PROG) $(BINDIR)

uninstall:
	rm -f $(MANDIR)/parrot.1
	rm -f $(BINDIR)/$(PROG)
