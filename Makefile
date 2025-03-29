.PHONEY: all clean install uninstall

PROG=parrot
CC?=gcc
CFLAGS=-O2 -std=c11 -Wall
PREFIX?=/usr/local
BINDIR=$(PREFIX)/bin
SRC=main.c
OBJ=$(SRC:.c=.o)

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(OBJ) -o $(PROG) $(CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -f $(PROG)

install:
	mkdir -p $(BINDIR)
	cp $(PROG) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/$(PROG)
