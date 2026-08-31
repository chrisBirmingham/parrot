.PHONEY: all clean install uninstall

PROG=parrot
CC?=gcc
CFLAGS?=-O2 -std=c2x -Wall -Wextra -pedantic -DVERSION=\"$(shell git describe --tags --always --dirty)\"
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
	install -D -C $(PROG).1 $(MANDIR)
	install -D -C $(PROG) $(BINDIR)

uninstall:
	rm -f $(MANDIR)/$(PROG).1
	rm -f $(BINDIR)/$(PROG)
