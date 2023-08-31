CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE

all: radar

radar: radar.c
	gcc $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f radar
