.PHONEY: all clean

PROG=parrot
CC=gcc
CFLAGS=-O2 -std=c11 -Wall
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
