CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

all: playground

playground: playground.c
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -f playground
