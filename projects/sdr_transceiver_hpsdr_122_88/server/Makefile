CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE

all: sdr-transceiver-hpsdr sdr-transceiver-hpsdr-thetis

sdr-transceiver-hpsdr: sdr-transceiver-hpsdr.c
	gcc $(CFLAGS) -o $@ $^ -lm -lpthread

sdr-transceiver-hpsdr-thetis: sdr-transceiver-hpsdr.c
	gcc $(CFLAGS) -DTHETIS -o $@ $^ -lm -lpthread

clean:
	rm -f sdr-transceiver-hpsdr sdr-transceiver-hpsdr-thetis
