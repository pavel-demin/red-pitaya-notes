CFLAGS = -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE

all: sdr-transceiver-hpsdr sdr-transceiver-hpsdr-ananxd

sdr-transceiver-hpsdr: sdr-transceiver-hpsdr.c
	gcc $(CFLAGS) -o $@ $^ -lm -lpthread

sdr-transceiver-hpsdr-ananxd: sdr-transceiver-hpsdr.c
	gcc $(CFLAGS) -DANANXD -o $@ $^ -lm -lpthread

clean:
	rm -f sdr-transceiver-hpsdr sdr-transceiver-hpsdr-ananxd
