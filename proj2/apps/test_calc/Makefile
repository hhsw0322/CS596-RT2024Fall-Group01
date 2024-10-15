CC=arm-linux-gnueabihf-gcc
CFLAGS=-Wall

all: test_calc

test_calc: test_calc.c
	$(CC) $(CFLAGS) -o test_calc test_calc.c

clean:
	rm -f test_calc
