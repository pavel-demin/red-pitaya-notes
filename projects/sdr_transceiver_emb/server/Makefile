CFLAGS = -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

all: sdr-transceiver-emb

sdr-transceiver-emb: sdr-transceiver-emb.c
	gcc $(CFLAGS) -o $@ $^ -lm -lpthread

clean:
	rm -f sdr-transceiver-emb
