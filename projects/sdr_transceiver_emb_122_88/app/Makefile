CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math

WDSP_DIR = ../wdsp

all: sdr-transceiver-emb

sdr-transceiver-emb: sdr-transceiver-emb.c $(WDSP_DIR)/libwdsp.a
	gcc $(CFLAGS) -o $@ $^ -I$(WDSP_DIR) -L$(WDSP_DIR) -lwdsp -lasound -lfftw3 -lm -lpthread

clean:
	rm -rf sdr-transceiver-emb
