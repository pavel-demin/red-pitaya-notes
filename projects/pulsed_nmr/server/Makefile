CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

all: pulsed-nmr

scanner: pulsed-nmr.c
	gcc $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f pulsed-nmr
