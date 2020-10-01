CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -D_GNU_SOURCE

all: adc-recorder dac-player

adc-recorder: adc-recorder.c
	gcc $(CFLAGS) -o $@ $^

dac-player: dac-player.c
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -f adc-recorder dac-player
