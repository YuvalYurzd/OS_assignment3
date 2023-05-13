CC = gcc
CFLAGS = -Wextra

all: stnc

stnc: stnc.c
	$(CC) $(CFLAGS) -o stnc stnc.c

clean:
	rm -f stnc