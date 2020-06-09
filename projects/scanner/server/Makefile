CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

all: scanner

scanner: scanner.c
	gcc $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f scanner
