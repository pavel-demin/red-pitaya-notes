CFLAGS = -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE

all: sdr-transceiver-hpsdr

sdr-transceiver-hpsdr: sdr-transceiver-hpsdr.c
	gcc $(CFLAGS) -o $@ $^ -lm -lpthread

clean:
	rm -f sdr-transceiver-hpsdr
