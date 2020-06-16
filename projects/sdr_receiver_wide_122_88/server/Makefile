CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE

all: sdr-receiver-wide gen

sdr-receiver-wide: sdr-receiver-wide.c
	gcc $(CFLAGS) -o $@ $^ -lm

gen: gen.c
	gcc $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f sdr-receiver-wide gen
