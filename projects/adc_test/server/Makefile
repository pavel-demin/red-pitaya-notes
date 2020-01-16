CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE

all: adc-test-server gen

adc-test-server: adc-test-server.c
	gcc $(CFLAGS) -o $@ $^ -lm

gen: gen.c
	gcc $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f adc-test-server gen

