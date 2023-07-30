CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

vpath %.o ../wsprd

all: gpio-output i2c-write led-output measure-corr measure-level sleep-rand sleep-to-59 temp0

gpio-output: gpio-output.c
	gcc $(CFLAGS) -o $@ $^

i2c-write: i2c-write.c
	gcc $(CFLAGS) -o $@ $^

led-output: led-output.c
	gcc $(CFLAGS) -o $@ $^

measure-corr: measure-corr.c
	gcc $(CFLAGS) -o $@ $^

measure-level: measure-level.c
	gcc $(CFLAGS) -o $@ $^ -lm

sleep-rand: sleep-rand.c
	gcc $(CFLAGS) -o $@ $^

sleep-to-59: sleep-to-59.c
	gcc $(CFLAGS) -o $@ $^

temp0: temp0.c
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -f gpio-output i2c-write led-output measure-corr measure-level sleep-rand sleep-to-59 temp0
