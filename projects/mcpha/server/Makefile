CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

all: mcpha-server pha-server

mcpha-server: mcpha-server.c
	gcc $(CFLAGS) -o $@ $^ -lm -lpthread

pha-server: pha-server.c
	gcc $(CFLAGS) -o $@ $^ -lpthread

clean:
	rm -f mcpha-server pha-server
