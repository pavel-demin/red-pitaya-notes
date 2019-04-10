CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

all: sdr-transceiver-wide

sdr-transceiver-wide: sdr-transceiver-wide.c
	gcc $(CFLAGS) -o $@ $^ -lm -lpthread

clean:
	rm -f sdr-transceiver-wide
