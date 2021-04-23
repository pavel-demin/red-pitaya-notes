CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

all: impedance-controller

impedance-controller: impedance-controller.c
	gcc $(CFLAGS) -o $@ $^ -lm -lpthread

clean:
	rm -f impedance-controller
